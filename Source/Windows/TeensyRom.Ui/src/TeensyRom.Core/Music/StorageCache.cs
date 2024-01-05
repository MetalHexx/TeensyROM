﻿using System.Collections.Generic;
using System.IO;
using System.Reactive.Concurrency;
using TeensyRom.Core.Common;
using TeensyRom.Core.Storage.Entities;

namespace TeensyRom.Core.Music
{
    public class StorageCache : Dictionary<string, StorageCacheItem>
    {
        public void UpsertDirectory(string path, StorageCacheItem directory)
        {
            Delete(path);
            Insert(path, directory);
        }
        
        public void UpsertFile(FileItem fileItem)
        {
            var fileParentDir = EnsureParents(fileItem.Path);            

            fileParentDir!.UpsertFile(fileItem);
            UpsertDirectory(fileParentDir.Path, fileParentDir);
        }

        public StorageCacheItem EnsureParents(string path)
        {
            var parentPath = CleanPath(path.GetUnixParentPath());
            var fileParentDir = Get(parentPath);

            if (fileParentDir is null)
            {
                fileParentDir = new StorageCacheItem
                {
                    Path = parentPath,
                    Directories = [],
                    Files = []
                };
                Insert(fileParentDir.Path, fileParentDir);
            }
            if(string.IsNullOrWhiteSpace(parentPath)) return fileParentDir;

            var grandParent = EnsureParents(parentPath);

            grandParent.InsertSubdirectory(new DirectoryItem
            {
                Name = fileParentDir.Path.GetLastDirectoryFromPath(),
                Path = fileParentDir.Path,
            });
            return fileParentDir;
        }

        private void Insert(string path, StorageCacheItem cacheItem)
        {
            var cleanPath = CleanPath(path);
            TryAdd(cleanPath, cacheItem);
        }

        public void Delete(string path)
        {
            var cleanPath = CleanPath(path);
            var dir = Get(cleanPath);

            if (dir is null) return;

            Remove(cleanPath);
        }

        public void DeleteWithChildren(string path)
        {
            var currentDir = Get(path);

            if (currentDir is null) return;

            foreach (var directory in currentDir.Directories)
            {
                DeleteWithChildren(directory.Path);
            }
            Delete(currentDir.Path);
        }

        public StorageCacheItem? Get(string path)
        {
            var cleanPath = CleanPath(path);

            if (!TryGetValue(cleanPath, out var item)) return null;

            return item;
        }

        private static string CleanPath(string path) => path
            .RemoveLeadingAndTrailingSlash();
    }
}