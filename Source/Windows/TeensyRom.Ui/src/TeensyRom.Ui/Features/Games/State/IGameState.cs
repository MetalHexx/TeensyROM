﻿using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using TeensyRom.Core.Storage.Entities;
using TeensyRom.Ui.Controls.DirectoryTree;

namespace TeensyRom.Ui.Features.Games.State
{
    public interface IGameState
    {
        IObservable<DirectoryNodeViewModel> DirectoryTree { get; }
        IObservable<ObservableCollection<StorageItem>> DirectoryContent { get; }
        IObservable<GameItem> RunningGame { get; }
        IObservable<GameMode> CurrentGameMode { get; }
        IObservable<GameStateType> CurrentPlayState { get; }
        IObservable<GameItem> GameLaunched { get; }
        IObservable<GameItem> SelectedGame { get; }

        Task LoadDirectory(string path);
        Task RefreshDirectory(bool bustCache = true);
        Task<bool> LoadGame(GameItem game, bool clearHistory = true);
        Task<bool> SaveFavorite(GameItem game);
        Task DeleteFile(GameItem file);
        Task PlayNext();
        Task PlayPrevious();
        Task<GameStateType> ToggleGame();
        Task<GameItem?> PlayRandom();
        Unit SearchGames(string keyword);
        Task ClearSearch();
        Unit ToggleShuffleMode();
        Task CacheAll();
        Unit SetSelectedGame(GameItem game);
    }
}