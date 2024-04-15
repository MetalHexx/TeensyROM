﻿using MediatR;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;
using System;
using System.Collections.Generic;
using System.Reactive;
using System.Reactive.Linq;
using System.Threading.Tasks;
using System.Transactions;
using System.Windows;
using TeensyRom.Core.Commands.File.LaunchFile;
using TeensyRom.Core.Common;
using TeensyRom.Core.Logging;
using TeensyRom.Core.Storage.Entities;
using TeensyRom.Ui.Features.Common.Models;
using TeensyRom.Ui.Features.Discover.State;
using TeensyRom.Ui.Features.Discover.State.Progress;

namespace TeensyRom.Ui.Controls.PlayToolbar
{
    public class PlayToolbarViewModel : ReactiveObject
    {   
        [Reactive] public bool PlayButtonEnabled { get; set; }
        [Reactive] public bool PauseButtonEnabled { get; set; }
        [Reactive] public bool StopButtonEnabled { get; set; }
        [Reactive] public bool TimedPlayEnabled { get; set; }
        [Reactive] public bool TimedPlayButtonEnabled { get; set; }
        [Reactive] public bool TimedPlayComboBoxEnabled { get; set; }              
        [Reactive] public bool ProgressEnabled { get; set; }
        [Reactive] public string TimerSeconds { get; set; } = "3m";
        [Reactive] public List<string> TimerOptions { get; set; } = ["5s", "10s", "15s", "30s", "1m", "3m", "5m", "10m", "15m", "30m"];
        [ObservableAsProperty] public bool ShowTitleOnly { get; }
        
        [ObservableAsProperty] public ILaunchableItem? File { get; }
        [ObservableAsProperty] public TimeProgressViewModel? Progress { get; } = null;
        [ObservableAsProperty] public bool ShuffleModeEnabled { get; }
        [ObservableAsProperty] public bool ShareVisible { get; }        
        [ObservableAsProperty] public bool ShowCreator { get; }
        [ObservableAsProperty] public bool ShowReleaseInfo { get; }
        [ObservableAsProperty] public bool ShowReleaseCreatorSeperator { get; }

        public ReactiveCommand<Unit, Unit> TogglePlayCommand { get; set; }
        public ReactiveCommand<Unit, Unit> PreviousCommand { get; set; }
        public ReactiveCommand<Unit, Unit> NextCommand { get; set; }
        public ReactiveCommand<Unit, Unit> ToggleShuffleCommand { get; set; }
        public ReactiveCommand<Unit, Unit> ToggleTimedPlay { get; set; }
        public ReactiveCommand<Unit, Unit> FavoriteCommand { get; set; }
        public ReactiveCommand<Unit, Unit> RemoveFavoriteCommand { get; set; }
        public ReactiveCommand<Unit, Unit> ShareCommand { get; set; }
        public ReactiveCommand<Unit, Unit> NavigateToFileDirCommand { get; set; }

        private readonly IAlertService _alert;
        private IProgressTimer? _timer;
        private IDisposable? _timerCompleteSubscription;
        private IDisposable? _currentTimeSubscription;

        public PlayToolbarViewModel(
            IObservable<ILaunchableItem> file, 
            IObservable<LaunchItemState> playState,
            IObservable<bool> timedPlayEnabled,
            IProgressTimer? timer,
            Func<Unit> toggleMode, 
            Func<Task> togglePlay,
            Func<Task> playPrevious, 
            Func<Task> playNext, 
            Func<ILaunchableItem, Task> saveFav,
            Func<ILaunchableItem, Task> removeFav,
            Func<string, Task> loadDirectory,            
            IAlertService alert)
        {
            _timer = timer;
            _alert = alert;

            var currentFile = file.Where(item => item is not null);

            var showReleaseInfo = currentFile
                .Select(item => !string.IsNullOrWhiteSpace(item.ReleaseInfo));

            var showCreatorInfo = currentFile
                .Select(item => !string.IsNullOrWhiteSpace(item.Creator));

            currentFile.ToPropertyEx(this, f => f.File);

            showReleaseInfo.ToPropertyEx(this, vm => vm.ShowReleaseInfo);

            showCreatorInfo.ToPropertyEx(this, vm => vm.ShowCreator);

            showReleaseInfo.CombineLatest(showCreatorInfo, (release, creator) => release && creator)
                .ToPropertyEx(this, vm => vm.ShowReleaseCreatorSeperator);

            showReleaseInfo.CombineLatest(showCreatorInfo, (release, creator) => !(release || creator))
                .Select(x => x)
                .ToPropertyEx(this, vm => vm.ShowTitleOnly);

            currentFile
                .Select(f => !string.IsNullOrWhiteSpace(f.ShareUrl))
                .ToPropertyEx(this, vm => vm.ShareVisible);

            playState
                .Select(state => state.PlayMode == PlayMode.Shuffle)
                .ToPropertyEx(this, vm => vm.ShuffleModeEnabled);

            playState
                .Where(state => state.PlayState != PlayState.Playing)
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(_ =>
                {
                    PauseButtonEnabled = false;
                    StopButtonEnabled = false;
                    PlayButtonEnabled = true;
                    _timer?.PauseTimer();
                });

            var playToggle = playState
                .ObserveOn(RxApp.MainThreadScheduler)
                .Where(state => state.PlayState == PlayState.Playing)                
                .Do(_ => PlayButtonEnabled = false);

            playToggle
                .Where(_ => File is GameItem or HexItem or ImageItem)
                .Subscribe(item => 
                {   
                    StopButtonEnabled = true;
                    PauseButtonEnabled = false;
                });

            playToggle
                .Where(_ => File is SongItem)
                .Subscribe(_ =>
                {
                    StopButtonEnabled = false;
                    PauseButtonEnabled = true;
                });

            var song = currentFile.OfType<SongItem>();
            var hexItem = currentFile.OfType<HexItem>();
            var gameOrImage = currentFile.Where(f => f is GameItem or ImageItem);

            song.Subscribe(item => 
            {
                TimedPlayButtonEnabled = false;
                TimedPlayComboBoxEnabled = false;
                ProgressEnabled = true;
                InitializeProgress(playNext, item);
            });

            gameOrImage
                .Where(_ => TimedPlayEnabled)
                .Subscribe(item =>
                {
                    TimedPlayButtonEnabled = true;
                    TimedPlayComboBoxEnabled = true;                                        
                    ProgressEnabled = true;
                    InitializeProgress(playNext, item);
                });

            gameOrImage
                .Where(_ => !TimedPlayEnabled)
                .Subscribe(item =>
                {
                    TimedPlayButtonEnabled = true;
                    TimedPlayComboBoxEnabled = false;                                       
                    ProgressEnabled = false;
                    _timer?.PauseTimer();
                });

            hexItem.Subscribe(item =>
            {
                TimedPlayButtonEnabled = false;
                TimedPlayComboBoxEnabled = false;
                ProgressEnabled = false;
                _timer?.PauseTimer();
            });

            this.WhenAnyValue(x => x.TimerSeconds)
                .Where(_ => TimedPlayEnabled)
                .Subscribe(timeSpan =>
                {
                    InitializeProgress(playNext, File);
                });

            timedPlayEnabled
                .Take(2)
                .Where(pt => pt is true).Subscribe(enabled =>
            {
                TimedPlayEnabled = true;
                TimedPlayComboBoxEnabled = true;
                ProgressEnabled = true;
                InitializeProgress(playNext, (File));
            });

            TogglePlayCommand = ReactiveCommand.CreateFromTask(_ => 
            {
                if (PlayButtonEnabled) _timer?.ResumeTimer();
                if (PauseButtonEnabled) _timer?.PauseTimer();
                return togglePlay();
            });

            ToggleTimedPlay = ReactiveCommand.Create<Unit>(_ => 
            {
                TimedPlayEnabled = !TimedPlayEnabled;

                if (TimedPlayEnabled)
                {
                    TimedPlayComboBoxEnabled = true;                    
                    ProgressEnabled = true;
                    InitializeProgress(playNext, (File));
                    return;
                }
                TimedPlayComboBoxEnabled = false;
                ProgressEnabled = false;                
                _timer?.PauseTimer();
            });

            NextCommand = ReactiveCommand.CreateFromTask(playNext);
            PreviousCommand = ReactiveCommand.CreateFromTask(playPrevious);
            ToggleShuffleCommand = ReactiveCommand.Create(toggleMode);
            FavoriteCommand = ReactiveCommand.CreateFromTask(_ => saveFav(File!));
            RemoveFavoriteCommand = ReactiveCommand.CreateFromTask(_ => removeFav(File!));
            ShareCommand = ReactiveCommand.Create<Unit, Unit>(_ => HandleShareCommand());
            NavigateToFileDirCommand = ReactiveCommand.CreateFromTask(_ => loadDirectory(File!.Path.GetUnixParentPath()!));
        }

        private void InitializeProgress(Func<Task> playNext, ILaunchableItem? item)
        {
            if (item == null) return;
            if(_timer == null) return;

            TimeSpan playLength;

            if(item is SongItem songItem)
            {
                playLength = songItem.PlayLength;
            }
            else
            {
                playLength = ConvertToTimeSpan(TimerSeconds);
            }

            _timerCompleteSubscription?.Dispose();
            _currentTimeSubscription?.Dispose();

            _timer?.StartNewTimer(playLength);

            _currentTimeSubscription = _timer?.CurrentTime
                .Select(t => new TimeProgressViewModel(playLength, t))
                .ObserveOn(RxApp.MainThreadScheduler)
                .ToPropertyEx(this, vm => vm.Progress);

            _timerCompleteSubscription = _timer?.TimerComplete
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(_ =>
                {
                    playNext();
                });
        }

        private Unit HandleShareCommand() 
        {   
            Clipboard.SetDataObject(File!.ShareUrl);
            _alert.Publish("Share URL copied to clipboard.");
            return Unit.Default;
        }

        private TimeSpan ConvertToTimeSpan(string timeString)
        {
            if (string.IsNullOrWhiteSpace(timeString))
            {
                return TimeSpan.Zero;
            }

            char timeUnit = timeString[^1];

            if(timeUnit != 's' && timeUnit != 'm')
            {
                return TimeSpan.Zero;
            }

            if (int.TryParse(timeString[..^1], out int timeValue))
            {
                return timeUnit switch
                {
                    's' => TimeSpan.FromSeconds(timeValue),
                    'm' => TimeSpan.FromMinutes(timeValue),
                    _ => TimeSpan.Zero
                };
            }
            return TimeSpan.Zero;
        }
    }
}
