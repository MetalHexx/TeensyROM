﻿using DynamicData;
using MediatR;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.IO;
using System.Linq;
using System.Reactive;
using System.Reactive.Linq;
using System.Reactive.Subjects;
using System.Runtime;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using TeensyRom.Core.Commands.File.LaunchFile;
using TeensyRom.Core.Common;
using TeensyRom.Core.Music;
using TeensyRom.Core.Settings;
using TeensyRom.Core.Storage.Entities;
using TeensyRom.Core.Storage.Services;
using TeensyRom.Ui.Controls.DirectoryTree;
using TeensyRom.Ui.Features.Files.DirectoryContent;

namespace TeensyRom.Ui.Features.Files.State
{
    public class FileState : IFileState, IDisposable
    {
        public IObservable<DirectoryNodeViewModel> DirectoryTree => _directoryTree.AsObservable();
        public IObservable<ObservableCollection<StorageItem>> DirectoryContent => _directoryContent.AsObservable();
        public IObservable<bool> DirectoryLoading => _directoryLoading.AsObservable();

        private readonly BehaviorSubject<DirectoryNodeViewModel> _directoryTree = new(new());
        private readonly Subject<ObservableCollection<StorageItem>> _directoryContent = new();
        private readonly Subject<bool> _directoryLoading = new();
        private readonly BehaviorSubject<StorageCacheItem?> _currentDirectory = new(null);

        private readonly ICachedStorageService _storageService;
        private readonly ISettingsService _settingsService;
        private readonly IMediator _mediator;
        private TeensySettings _settings = new();
        private IDisposable _settingsSubscription;

        public FileState(ICachedStorageService storageService, ISettingsService settingsService, IMediator mediator)
        {
            _storageService = storageService;
            _settingsService = settingsService;
            _mediator = mediator;
            _settingsSubscription = _settingsService.Settings.Subscribe(settings => OnSettingsChanged(settings));

        }
        private void OnSettingsChanged(TeensySettings settings)
        {
            _settings = settings;
            _directoryContent.OnNext(new ObservableCollection<StorageItem>());
            ResetDirectoryTree();
        }

        private void ResetDirectoryTree()
        {
            var musicLibraryPath = _settings.GetLibraryPath(TeensyLibraryType.Music);

            var dirItem = new DirectoryNodeViewModel
            {
                Name = "Fake Root",  //TODO: Fake root required since UI view binds to enumerable -- design could use improvement
                Path = "Fake Root",
                Directories =
                [
                    new DirectoryNodeViewModel
                    {
                        Name = "/",
                        Directories = []
                    }
                ]
            };
            _directoryTree.OnNext(dirItem);
        }


        public async Task LoadDirectory(string path)
        {
            _directoryLoading.OnNext(true);
            var directoryResult = await _storageService.GetDirectory(path);

            if (directoryResult is null)
            {
                _directoryLoading.OnNext(false);
                return;
            }

            Application.Current.Dispatcher.Invoke(() =>
            {
                _directoryTree.Value.Insert(directoryResult.Directories);
                _directoryTree.Value.SelectDirectory(path);
            });

            _directoryTree.OnNext(_directoryTree.Value);

            var directoryItems = new ObservableCollection<StorageItem>();
            directoryItems.AddRange(directoryResult.Directories);
            directoryItems.AddRange(directoryResult.Files);

            _directoryContent.OnNext(directoryItems);
            _currentDirectory.OnNext(directoryResult);
            _directoryTree.OnNext(_directoryTree.Value);
            _directoryLoading.OnNext(false);

            return;
        }

        public async Task RefreshDirectory(bool bustCache = true)
        {
            if (_currentDirectory.Value is null) return;

            if (bustCache) _storageService.ClearCache(_currentDirectory.Value.Path);

            await LoadDirectory(_currentDirectory.Value.Path);
        }

        public async Task StoreFiles(IEnumerable<FileCopyItem> files)
        {
            var commonParent = GetCommonBasePath(files.Select(i => i.Path));
            var directoryOnly = files.All(i => i.InSubdirectory);

            foreach (var file in files)
            {
                var fileInfo = new TeensyFileInfo(file.Path);

                var finalPath = directoryOnly
                    ? Directory.GetParent(commonParent)?.FullName
                    : commonParent;

                var relativePath = fileInfo.FullPath
                    .Replace(finalPath!, string.Empty)
                    .ToUnixPath()
                    .GetUnixParentPath();
                
                fileInfo.TargetPath = _currentDirectory.Value!.Path.UnixPathCombine(relativePath);
                await _storageService.SaveFile(fileInfo);
            }
            await RefreshDirectory(bustCache: false);
        }

        private static string GetCommonBasePath(IEnumerable<string> directories)
        {
            if (!directories.Any()) return string.Empty;

            string commonPath = directories.First();

            foreach (string path in directories)
            {
                while (!path.StartsWith(commonPath, StringComparison.OrdinalIgnoreCase))
                {
                    commonPath = commonPath.Substring(0, commonPath.LastIndexOf(Path.DirectorySeparatorChar));
                }
            }
            return commonPath;
        }

        public void Dispose()
        {
            _settingsSubscription?.Dispose();
        }

        public Task LaunchFile(FileItem file) => _mediator.Send(new LaunchFileCommand { Path = file.Path });

        public async Task SaveFavorite(FileItem file)
        {
            var favFile = await _storageService.SaveFavorite(file);
            var parentDir = favFile?.Path.GetUnixParentPath();

            if (parentDir is null) return;

            var directoryResult = await _storageService.GetDirectory(parentDir);

            if (directoryResult is null) return;

            Application.Current.Dispatcher.Invoke(() =>
            {
                _directoryTree.Value.Insert(directoryResult.Directories);
            });

            _directoryTree.OnNext(_directoryTree.Value);

            var favParentPath = favFile?.Path.GetUnixParentPath().GetUnixParentPath().RemoveLeadingAndTrailingSlash();
            var currentPath = _currentDirectory.Value?.Path.RemoveLeadingAndTrailingSlash();

            if (favParentPath == currentPath) 
            {
                await RefreshDirectory(bustCache: false);
            }
        }
    }
}
