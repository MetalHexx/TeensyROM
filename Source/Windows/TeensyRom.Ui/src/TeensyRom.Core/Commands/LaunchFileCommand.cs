﻿using MediatR;
using System.IO;
using System.Reactive;
using TeensyRom.Core.Common;
using TeensyRom.Core.Logging;
using TeensyRom.Core.Serial;
using TeensyRom.Core.Settings;
using TeensyRom.Core.Storage.Entities;

namespace TeensyRom.Core.Commands.File.LaunchFile
{
    public class LaunchFileCommand : IRequest<LaunchFileResponse>
    {
        public string Path { get; set; } = string.Empty;
    }
    public class LaunchFileResponse: CommandResult { }
    public class LaunchFileHandler: TeensyCommand, IRequestHandler<LaunchFileCommand, LaunchFileResponse>
    {
        public LaunchFileHandler(ISettingsService settingsService, IObservableSerialPort serialPort, ILoggingService logService) 
            : base(settingsService, serialPort, logService) { }

        public Task<LaunchFileResponse> Handle(LaunchFileCommand request, CancellationToken cancellationToken)
        {
            _serialPort.SendIntBytes(TeensyConstants.Launch_File_Token, 2);

            if (!GetAck())
            {
                ReadSerialAsString();
                throw new TeensyException("Error getting acknowledgement when Launch File Token sent");
            }
            _serialPort.SendIntBytes(GetStorageToken(_settings.TargetType), 1);
            _serialPort.Write($"{request.Path}\0");

            if (!GetAck())
            {
                ReadSerialAsString(msToWait: 100);
                throw new TeensyException("Error getting acknowledgement when launch path sent");
            }
            return Task.FromResult(new LaunchFileResponse());
        }
    }
}