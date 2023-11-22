﻿using ReactiveUI;
using ReactiveUI.Fody.Helpers;
using System;
using System.Reactive;
using System.Reactive.Linq;
using TeensyRom.Core.Commands.File.LaunchFile;
using TeensyRom.Ui.Features.Common.Models;
using TeensyRom.Ui.Features.Music.State;

namespace TeensyRom.Ui.Features.Music.PlayToolbar
{

    public class PlayToolbarViewModel: ReactiveObject
    {
        [ObservableAsProperty] public SongItemVm Song { get; }
        [ObservableAsProperty] public string TotalTime { get; }
        [ObservableAsProperty] public string CurrentTime { get; }
        [ObservableAsProperty] public double CurrentProgress { get; }
        [ObservableAsProperty] public bool ShuffleModeEnabled { get; }
        [ObservableAsProperty] public bool IsPlaying { get; }

        public  ReactiveCommand<Unit, Unit>  TogglePlayCommand { get; set; }
        public  ReactiveCommand<Unit, Unit>  PreviousCommand { get; set; }
        public  ReactiveCommand<Unit, Unit>  NextCommand { get; set; }

        private readonly ILaunchFileCommand _launchFileCommand;
        private readonly IMusicState _musicState;

        public PlayToolbarViewModel(ILaunchFileCommand launchFileCommand, IMusicState musicState)
        {
            _launchFileCommand = launchFileCommand;
            _musicState = musicState;

            _musicState.CurrentSong
                .ToPropertyEx(this, s => s.Song);

            _musicState.CurrentSongTime
                .Select(t => t.ToString(@"mm\:ss"))
                .ToPropertyEx(this, vm => vm.CurrentTime);

            _musicState.CurrentSongMode
                .Select(mode => mode == SongMode.Shuffle)
                .ToPropertyEx(this, vm => vm.ShuffleModeEnabled);

            _musicState.CurrentPlayState
                .Select(playState => playState == PlayState.Playing)
                .ToPropertyEx(this, vm => vm.IsPlaying);

            _musicState.CurrentSong
                .CombineLatest(_musicState.CurrentSongTime,
                    (song, currentTime) => (double)currentTime.TotalSeconds / (double)song.SongLength.TotalSeconds)
                .ToPropertyEx(this, x => x.CurrentProgress);

            _musicState.CurrentSong
                .Select(s => s.SongLength.ToString(@"mm\:ss"))
                .ToPropertyEx(this, x => x.TotalTime);


            TogglePlayCommand = ReactiveCommand.Create<Unit, Unit>(_ => HandleTogglePlayCommand());
            NextCommand = ReactiveCommand.Create<Unit, Unit>(_ => HandleNextCommand());
            PreviousCommand = ReactiveCommand.Create<Unit, Unit>(_ => HandlePreviousCommand());
        }

        private Unit HandlePreviousCommand()
        {
            _musicState.PlayPrevious();
            return Unit.Default;
        }

        private Unit HandleNextCommand()
        {
            _musicState.PlayNext();
            return Unit.Default;
        }

        private Unit HandleTogglePlayCommand()
        {
            _musicState.ToggleMusic();
            return Unit.Default;
        }
    }
}