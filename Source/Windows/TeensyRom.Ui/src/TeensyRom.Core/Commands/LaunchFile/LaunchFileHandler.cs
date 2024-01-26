﻿using MediatR;
using System.Reactive.Linq;
using TeensyRom.Core.Common;
using TeensyRom.Core.Logging;
using TeensyRom.Core.Serial;
using TeensyRom.Core.Serial.State;
using TeensyRom.Core.Settings;
using TeensyRom.Core.Storage.Entities;

namespace TeensyRom.Core.Commands.File.LaunchFile
{
    public class LaunchFileHandler: IRequestHandler<LaunchFileCommand, LaunchFileResult>
    {
        private TeensySettings _settings;
        private readonly ISerialStateContext _serialState;
        private readonly ILoggingService _log;

        public LaunchFileHandler(ISerialStateContext serialState, ISettingsService settings, ILoggingService log)
        {
            settings.Settings.Take(1).Subscribe(s => _settings = s);
            _serialState = serialState;
            _log = log;
        }

        public Task<LaunchFileResult> Handle(LaunchFileCommand request, CancellationToken cancellationToken)
        {
            _serialState.SendIntBytes(TeensyToken.LaunchFile, 2);
            _serialState.HandleAck();
            _serialState.SendIntBytes(_settings.TargetType.GetStorageToken(), 1);
            _serialState.Write($"{request.Path}\0");
            _serialState.HandleAck();
            var response = _serialState.ReadSerialAsString(100);
            var resultType = ParseResponse(response);

            return Task.FromResult(new LaunchFileResult
            {
                LaunchResult = resultType
            });
        }

        private LaunchFileResultType ParseResponse(string response)
        {
            var sidError = new[] { "PSID not found", "Mem conflict w/ TR app" };

            if (sidError.Any(response.Contains))
            {
                _log.ExternalError($"Failed to launch file: \r\n{response}");
                return LaunchFileResultType.SidError;
            }
            _log.External(response);
            return LaunchFileResultType.Success;
        }
    }
}