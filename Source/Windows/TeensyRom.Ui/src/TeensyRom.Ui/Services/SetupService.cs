﻿using ReactiveUI;
using System;
using System.Collections.Generic;
using System.DirectoryServices.ActiveDirectory;
using System.Linq;
using System.Reactive.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using TeensyRom.Core.Serial.State;
using TeensyRom.Core.Settings;
using TeensyRom.Core.Storage.Entities;
using TeensyRom.Core.Storage.Services;
using TeensyRom.Ui.Features.Common.State.Player;
using TeensyRom.Ui.Features.Discover;
using TeensyRom.Ui.Features.Discover.State;
using TeensyRom.Ui.Features.NavigationHost;

namespace TeensyRom.Ui.Services
{
    public interface ISetupService
    {
        void ResetSetup();
        Task StartSetup();

    }
    public class SetupService : ISetupService
    {
        private readonly ISettingsService _settingsService;
        private readonly INavigationService _navigation;
        private readonly ISerialStateContext _serial;
        private readonly IDialogService _dialog;
        private readonly ICachedStorageService _storage;
        private readonly IDiscoverContext _discoverContext;
        private readonly DiscoverViewModel _discoverView;
        private TeensySettings _settings = null!;
        private bool _sdSuccess = false;
        private bool _usbSuccess = false;

        public SetupService(ISettingsService settingsService, INavigationService navigation, ISerialStateContext serial, IDialogService dialog, ICachedStorageService storage, IDiscoverContext discoverContext, DiscoverViewModel discoverView)
        {
            _settingsService = settingsService;
            _navigation = navigation;
            _serial = serial;
            _dialog = dialog;
            _storage = storage;
            _discoverContext = discoverContext;
            _discoverView = discoverView;
            _settingsService.Settings.Subscribe(settings => _settings = settings);
        }
        public void ResetSetup()
        {
            _settings = _settingsService.GetSettings();
            _settings.FirstTimeSetup = true;
            _settingsService.SaveSettings(_settings);
        }
        public async Task StartSetup()
        {
            _settings = _settingsService.GetSettings();

            if (!_settings.FirstTimeSetup) return;


            var currentView = await _navigation.SelectedNavigationView.FirstOrDefaultAsync();

            if (currentView.Type is not NavigationLocation.Connect)
            {
                _navigation.NavigateTo(NavigationLocation.Connect);
            }

            var result = await _dialog.ShowConfirmation("Welcome to TeensyROM!", "This start up guide will help you get set up and learn about a few basic features of the app. \r\rAt any point, feel free to press cancel to finish up on your own.");

            if (!result)
            {
                await Complete();
                return;
            }
            await OnConnectable();
        }

        public async Task OnConnectable()
        {
            _serial.CurrentState
                .Where(s => s is SerialConnectableState)
                .Take(1)
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(async _ =>
                {
                    var result = await _dialog.ShowConfirmation("Connect to the TR", "Click on the Connect button.  If things go ok, we'll continue to the next step. \r\rNote, your TR will reset once you connect.  This is normal.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    OnConnected();
                });

            var currentSerialState = await _serial.CurrentState.FirstAsync();

            var result = false;

            if (currentSerialState is not SerialConnectableState)
            {
                result = await _dialog.ShowConfirmation("No COM Ports Detected", "Things to try before we proceed: \r\r· Make sure your USB cable is seated properly \r· Try a different USB cable \r· Make sure the C64 is turned on or try restarting it \r\rOnce I detect COM ports, we'll go to the next step.");
            }
        }

        public void OnConnected()
        {
            _serial.CurrentState
                .Where(s => s is SerialConnectedState)
                .Take(1)
                .ObserveOn(RxApp.MainThreadScheduler).Subscribe(async _ =>
                {
                    var result = await _dialog.ShowConfirmation("Successful Connection!", "You have connected to the TR successfully.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    result = await _dialog.ShowConfirmation("Connection View", "On this screen, you will find a few utility features that might come in handy.\r\r· Ping: This is useful for troubleshooting connectivity. \r· Reset: Will reset the C64. \r· Logs: Useful for troubleshooting or debugging.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    result = await _dialog.ShowConfirmation("Automatic Reconnect", "Useful to note that if you decide to turn off your C64 or lose your connection to the TR for some other reason, the application will automatically attempt to re-connect.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    result = await _dialog.ShowConfirmation("Settings", "Let's head over to the settings view and get your preferences configured.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }

                    _navigation.NavigateTo(NavigationLocation.Settings);

                    result = await _dialog.ShowConfirmation("Settings", "There are a few settings you should be aware of.  Let's review and configure them.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }

                    result = await _dialog.ShowConfirmation("Automatic File Transfer", "The \"Watch Directory\" can be configured to make copying files your TR very easy.\r\rWhen new .SID, .CRT, .PRG or .HEX firmware files copied to the watch directory, they will be detected and automatically be uploaded to the /auto-transfer directory on your TR. \r\rYou may find the download directory a good choice if you like the roam the web for your C64 content.  Your download directory is set by default.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }

                    result = await _dialog.ShowConfirmation("Automatic File Transfer", "Using the download directory also makes it easy to update your TR by downloading a new Firmware (hex file) using your browser.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }

                    result = await _dialog.ShowConfirmation("Auto-Launch on File Transfer", "Note, when you copy files to the TR, you can optionally have them automatically launch.  This behavior is on by default.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    result = await _dialog.ShowConfirmation("Auto-Launch on File Transfer", "If you're a SID musician or a programmer, the watch directory combined with auto-launch can turn your TR into a realtime testing environment!");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }

                    result = await _dialog.ShowConfirmation("Save Your Settings", "Once you're done configuring your settings, click the \"Save Settings\" button and we'll go to the next step.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }

                    OnSettingsSaved();
                });
        }

        public void OnSettingsSaved()
        {
            _settingsService.Settings
                .Skip(1)
                .Take(1)
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(async settings =>
            {
                var result = await _dialog.ShowConfirmation("Let's Index", "Next, we're going to index all the file locations on your selected storage device.\r\rThis will increase performance, stability, and unlock some fun search and randomization features.");

                if (!result)
                {
                    await Complete();
                    return;
                }

                result = await _dialog.ShowConfirmation("Copy Some Files!", $"Copy some files now before we start the indexing process.  \r\rTotally optional, but strongly recommended, consider copying HVSC and OneLoad64 onto your {_settings.StorageType} storage. You can always do this later if you want.\r\rClicking \"OK\" will start the indexing process.");

                if (!result)
                {
                    await Complete();
                    return;
                }

                await OnCacheSD();
            });
        }

        public async Task OnCacheSD()
        {   
            var result = await _dialog.ShowConfirmation("Let's navigate back to the connection screen.  You can watch as all the files are read from the TR.\r\rHang tight as this will likely take several minutes if you have thousands of files.");
            
            _navigation.NavigateTo(NavigationLocation.Connect);

            result = await _dialog.ShowConfirmation("SD Index", "First we'll check your SD card for files to index.");
            if (!result)
            {
                await Complete();
                return;
            }

            _settings.StorageType = TeensyStorageType.SD;
            _settingsService.SaveSettings(_settings);
            await _storage.CacheAll(StorageConstants.Remote_Path_Root);

            await OnCacheUSB();
        }

        public async Task OnCacheUSB()
        {
            var result = await _dialog.ShowConfirmation("USB Index", "Next we'll check your USB storage for files to index.");
            if (!result)
            {
                await Complete();
                return;
            }
            _settings.StorageType = TeensyStorageType.USB;
            _settingsService.SaveSettings(_settings);
            await _storage.CacheAll(StorageConstants.Remote_Path_Root);

            result = await _dialog.ShowConfirmation("File Indexing Completed", $"Now that your file information has been indexed, lets head over to the Discover view and do some exploring.");

            if (!result)
            {
                await Complete();
                return;
            }

            await OnDiscover();
        }

        public async Task OnDiscover() 
        {
            _navigation.NavigateTo(NavigationLocation.Discover);

            _discoverView.StorageSelector.SelectedStorage = TeensyStorageType.SD;

            var result = await _dialog.ShowConfirmation("Indexing Files", "If you make any changes to your storage outside of this application, you can always re-index all your files by clicking on the download button in the upper right.  \r\rNote, if you avoid indexing all your files, your random play and search capabilities will be limited to the folders you have visited.");

            if (!result)
            {
                await Complete();
                return;
            }

            result = await _dialog.ShowConfirmation("Discovery View", $"In the \"Discover\" view, you can navigate and launch music and games (or other programs).  \r\rIn the first 2 sections, you should see the root directory structure and file listing of your {_settings.StorageType} storage.\r\rOn the right you will find some file information for the currently selected file.  If you copy the HVSC and OneLoad64 collection to your SD, you will be treated with some extra content here.");

            if (!result)
            {
                await Complete();
                return;
            }

            result = await _dialog.ShowConfirmation("Storage", $"{_settings.StorageType} storage is currently selected.  You can switch between SD and USB storage using the dropdown next to the \"Discover\" title.\r\rYour last selected storage device will be remembered next time you load the app.");

            if (!result)
            {
                await Complete();
                return;
            }

            result = await _dialog.ShowConfirmation("Transfer Files", "You can drag and drop files or folders onto the file listing of the current directory to transfer files to the TR.  Optionally, copied files can be auto-launched. \r\rWarning, if you drag a folder that has other nested folders, they will all be copied!");

            if (!result)
            {
                await Complete();
                return;
            }

            result = await _dialog.ShowConfirmation("Feeling Lucky?", "Let's try discovering something to play. \r\rClick on the die button near the lower left of the screen next to the \"All\", \"Games\", and \"Music\" filters.");

            if (!result)
            {
                await Complete();
                return;
            }
        }

        public void OnLaunch()
        {
            _discoverContext.LaunchedFile
                .OfType<ILaunchableItem>()
                .Where(file => file.IsCompatible is true)
                .Take(1)
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(async file =>
                {
                    var itemType = file is SongItem ? "SID" : "Game";

                    var result = await _dialog.ShowConfirmation("Random Launch", $"I see you discoverd a {itemType} called \"{file.Name}\", nice! \r\rNotice the \"All\" filter is selected in the lower left.  Currently, either Games or Music will be launched randomly.  \r\rTry selecting the \"Games\" or \"Music\" filter.  After, try clicking the \"Next\" button on the play toolbar.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }

                    OnLaunchGame();
                });
        }
        public void OnLaunchGame()
        {
            _discoverContext.LaunchedFile
                .OfType<ILaunchableItem>()
                .Skip(1)
                .Take(1)
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(async file =>
                {
                    var itemType = string.Empty;

                    var result = false;

                    if (file is GameItem)
                    {
                        result = await _dialog.ShowConfirmation("Random Game Launched", $"Nice, you found a game located at {file.Path}.  \r\rNotice how the directory listing has also changed to location of the launched file.");
                    }
                    else if (file is SongItem)
                    {
                        result = await _dialog.ShowConfirmation("Random Music Launched", $"Nice, you found a SID located at {file.Path}.  \r\rNotice how the directory listing has also changed to location of the launched file.");
                    }

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    result = await _dialog.ShowConfirmation("Play Toolbar", "As you may have noticed, the \"Discover\" view functions like a media player.  Depending on the mode you're in and the filter you have selected, clicking \"Previous\" or \"Next\" will have a different behavior.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    result = await _dialog.ShowConfirmation("Play Toolbar", "Currently we're in \"Shuffle Mode\" as indicated by the blue crossed arrows on the right.  As you saw, the \"Next\" button played the next random file.  \r\rNote, in this application, shuffle mode works across your entire collection for the selected storage device, not just the current directory.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    result = await _dialog.ShowConfirmation("Play Toolbar", "Try clicking the \"Shuffle Mode\" button to turn it off.  Then click the \"Next\" button.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    OnNormalPlay();
                });
        }

        public void OnNormalPlay()
        {
            _discoverContext.LaunchedFile
                .OfType<ILaunchableItem>()
                .Skip(1)
                .Take(1)
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(async file =>
                {
                    var itemType = string.Empty;

                    var result = await _dialog.ShowConfirmation("Normal Play Mode", $"The next file in the current directory should have launched if you turned off shuffle mode.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    result = await _dialog.ShowConfirmation("Music Only", $"Music Launching has some special behaviors that games do not.  \r\r· When a SID ends, the next one will be played automatically. \r· The previous button will restart the current SID.  \r· Clicking previous a second time will go to the previous SID.  \r· A share button will copy a shareable DeepSID link to your clipboard.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    result = await _dialog.ShowConfirmation("Games Only", $"Games also have some unique behaviors.  \r\r· Games will have a \"Stop\" button instead of \"Pause\".  \r· Stop will reset the TR\r· Tagging a favorite while a game is launched will re-launch the game.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    result = await _dialog.ShowConfirmation("Favorite Your Favorites!", $"Whenever you see a \"Heart\" button throughout the application, when you click it, the file will be saved to the /favorites folder.  \r\rThis will physically copy the file there, so you can find the favorites while using TR C64 UI directly as well.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }
                    result = await _dialog.ShowConfirmation("Search", $"In the upper right, you can type multiple keywords to search for games or SIDs.  As you would expect, \"All\", \"Games\" and \"Music\" will filter your search results.\r\rPlaying Next or Previous will move through the files in your search results.");

                    if (!result)
                    {
                        await Complete();
                        return;
                    }

                    result = await _dialog.ShowConfirmation("", $"I hope these tools help you have fun finding new content!  \r\rLet's head over to the \"Help\" section now.");

                    await Complete();
                });
        }

        public Task Complete()
        {
            var settings = _settingsService.GetSettings();
            settings.FirstTimeSetup = false;
            _settingsService.SaveSettings(settings);

            _navigation.NavigateTo(NavigationLocation.Help);

            return _dialog.ShowConfirmation("Setup Wizard Complete", "The help section is a great place to find out more about the project.\r\rVisit the GitHub project page for more detailed documentation on the application features or troubleshooting tips.\r\rIf you'd like to re-run this tutorial, you can do so from here.\r\rEnjoy! ;)");
        }
    }
}
