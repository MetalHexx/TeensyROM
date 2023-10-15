﻿using FluentAssertions;
using Newtonsoft.Json;
using System.IO.Ports;
using TeensyRom.Core.Files;
using TeensyRom.Core.Files.Abstractions;
using TeensyRom.Core.Serial;
using TeensyRom.Core.Serial.Abstractions;
using TeensyRom.Core.Settings;
using TeensyRom.Ui.Features.FileTransfer;

namespace TeensyRom.Tests
{
    [Collection("SerialPortTests")]
    public class FileTransferTests: IDisposable
    {
        private FileTransferViewModel _viewModel;
        private ITeensyObservableSerialPort _teensyPort;
        private IFileWatcher _fileWatcher;
        private ISettingsService _settingsService;
        private ITeensyFileService _fileService;

        private readonly string _settingsFileName = "Settings.json";        
        private readonly string _testFileName = $"{Guid.NewGuid().ToString().Substring(0, 7)}-test";
        private readonly string _fullSourceTestPath = string.Empty;

        private readonly string _serialPortName = SerialPort.GetPortNames().First();

        private readonly TeensySettings _settings = new()
        {
            SidTargetPath = "/integration-test-files/sid/",
            PrgTargetPath = "/integration-test-files/prg/",
            CrtTargetPath = "/integration-test-files/crt/",
            TargetType = TeensyStorageType.SD,
            WatchDirectoryLocation = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), "Downloads")
        };

        public FileTransferTests()
        {   
            _fullSourceTestPath = @$"{_settings.WatchDirectoryLocation}\{_testFileName}";
        }

        [Fact]
        public void Given_WatcherDetectsNewFile_When_SidSaved_ToSD_Then_ReturnsSuccess()
        {
            //Arrange
            var fileDetectedText = @$"File detected: {_settings.WatchDirectoryLocation}\{_testFileName}.sid";
            var initiatedText = $"Initiating file transfer handshake";
            var savedText = $"File transfer complete!";
            var expectedType = $"Type: Sid";
            var storageType = $"Storage Type: SD";

            _settings.TargetType = TeensyStorageType.SD;
            InitializeViewModel();

            //Act
            File.WriteAllText($"{ _fullSourceTestPath}.sid", "Test sid");
            Thread.Sleep(1000);

            //Assert
            _viewModel.Logs.Should().Contain(fileDetectedText);
            _viewModel.Logs.Should().Contain(initiatedText);
            _viewModel.Logs.Should().Contain(savedText);
            _viewModel.Logs.Should().Contain(expectedType);
            _viewModel.Logs.Should().Contain(storageType);
        }

        [Fact]
        public void Given_WatcherDetectsNewFile_When_PrgSaved_ToSD_Then_ReturnsSuccess()
        {
            //Arrange
            var fileDetectedText = @$"File detected: {_settings.WatchDirectoryLocation}\{_testFileName}.prg";
            var initiatedText = $"Initiating file transfer handshake";
            var savedText = $"File transfer complete!";
            var expectedType = $"Type: Prg";
            var storageType = $"Storage Type: SD";

            _settings.TargetType = TeensyStorageType.SD;
            InitializeViewModel();

            //Act
            File.WriteAllText($"{_fullSourceTestPath}.prg", "Test prg");
            Thread.Sleep(1000);

            //Assert
            _viewModel.Logs.Should().Contain(fileDetectedText);
            _viewModel.Logs.Should().Contain(initiatedText);
            _viewModel.Logs.Should().Contain(savedText);
            _viewModel.Logs.Should().Contain(expectedType);
            _viewModel.Logs.Should().Contain(storageType);
        }

        [Fact]
        public void Given_WatcherDetectsNewFile_When_CrtSaved_ToSD_Then_ReturnsSuccess()
        {
            //Arrange
            var fileDetectedText = @$"File detected: {_settings.WatchDirectoryLocation}\{_testFileName}.crt";
            var initiatedText = $"Initiating file transfer handshake";
            var savedText = $"File transfer complete!";
            var expectedType = $"Type: Crt";
            var storageType = $"Storage Type: SD";

            _settings.TargetType = TeensyStorageType.SD;
            InitializeViewModel();

            //Act
            File.WriteAllText($"{_fullSourceTestPath}.crt", "Test crt");
            Thread.Sleep(1000);

            //Assert
            _viewModel.Logs.Should().Contain(fileDetectedText);
            _viewModel.Logs.Should().Contain(initiatedText);
            _viewModel.Logs.Should().Contain(savedText);
            _viewModel.Logs.Should().Contain(expectedType);
            _viewModel.Logs.Should().Contain(storageType);
        }

        [Fact]
        public void Given_WatcherDetectsNewFile_When_SidSaved_ToUSB_Then_ReturnsSuccess()
        {
            //Arrange
            var fileDetectedText = @$"File detected: {_settings.WatchDirectoryLocation}\{_testFileName}.sid";
            var initiatedText = $"Initiating file transfer handshake";
            var savedText = $"File transfer complete!";
            var expectedType = $"Type: Sid";
            var storageType = $"Storage Type: USB";

            _settings.TargetType = TeensyStorageType.USB;
            InitializeViewModel();

            //Act
            File.WriteAllText($"{_fullSourceTestPath}.sid", "Test sid");
            Thread.Sleep(1000);

            //Assert
            _viewModel.Logs.Should().Contain(fileDetectedText);
            _viewModel.Logs.Should().Contain(initiatedText);
            _viewModel.Logs.Should().Contain(savedText);
            _viewModel.Logs.Should().Contain(expectedType);
            _viewModel.Logs.Should().Contain(storageType);
        }

        [Fact]
        public void Given_WatcherDetectsNewFile_When_PrgSaved_ToUSB_Then_ReturnsSuccess()
        {
            //Arrange
            var fileDetectedText = @$"File detected: {_settings.WatchDirectoryLocation}\{_testFileName}.prg";
            var initiatedText = $"Initiating file transfer handshake";
            var savedText = $"File transfer complete!";
            var expectedType = $"Type: Prg";
            var storageType = $"Storage Type: USB";

            _settings.TargetType = TeensyStorageType.USB;
            InitializeViewModel();

            //Act
            File.WriteAllText($"{_fullSourceTestPath}.prg", "Test prg");
            Thread.Sleep(1000);

            //Assert
            _viewModel.Logs.Should().Contain(fileDetectedText);
            _viewModel.Logs.Should().Contain(initiatedText);
            _viewModel.Logs.Should().Contain(savedText);
            _viewModel.Logs.Should().Contain(expectedType);
            _viewModel.Logs.Should().Contain(storageType);
        }

        [Fact]
        public void Given_WatcherDetectsNewFile_When_CrtSaved_ToUSB_Then_ReturnsSuccess()
        {
            //Arrange
            var fileDetectedText = @$"File detected: {_settings.WatchDirectoryLocation}\{_testFileName}.crt";
            var initiatedText = $"Initiating file transfer handshake";
            var savedText = $"File transfer complete!";
            var expectedType = $"Type: Crt";
            var storageType = $"Storage Type: USB";

            _settings.TargetType = TeensyStorageType.USB;
            InitializeViewModel();

            //Act
            File.WriteAllText($"{_fullSourceTestPath}.crt", "Test crt");
            Thread.Sleep(1000);

            //Assert
            _viewModel.Logs.Should().Contain(fileDetectedText);
            _viewModel.Logs.Should().Contain(initiatedText);
            _viewModel.Logs.Should().Contain(savedText);
            _viewModel.Logs.Should().Contain(expectedType);
            _viewModel.Logs.Should().Contain(storageType);
        }

        private void InitializeViewModel()
        {
            var json = JsonConvert.SerializeObject(_settings);
            File.WriteAllText(_settingsFileName, json);

            _teensyPort = new TeensyObservableSerialPort();
            _fileWatcher = new FileWatcher();
            _settingsService = new SettingsService();
            _fileService = new TeensyFileService(_settingsService, _fileWatcher, _teensyPort);
            _viewModel = new FileTransferViewModel(_fileService);
            _teensyPort.SetPort(_serialPortName);
            _teensyPort.OpenPort();
        }

        public void Dispose()
        {
            _teensyPort?.Dispose();
            _fileWatcher.Dispose();
            _fileService.Dispose();

            if (File.Exists(_fullSourceTestPath))
            {
                File.Delete(_fullSourceTestPath);
            }

            if (File.Exists(_settingsFileName))
            {
                File.Delete(_settingsFileName);
            }
        }
    }
}
