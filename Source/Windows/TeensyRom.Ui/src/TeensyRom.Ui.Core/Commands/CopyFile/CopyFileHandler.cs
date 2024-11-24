﻿using MediatR;
using System.Reactive.Linq;
using TeensyRom.Core.Common;
using TeensyRom.Ui.Core.Serial;
using TeensyRom.Ui.Core.Serial.State;
using TeensyRom.Ui.Core.Settings;
using TeensyRom.Core.Storage.Entities;

namespace TeensyRom.Ui.Core.Commands
{
    public class CopyFileHandler : IRequestHandler<CopyFileCommand, CopyFileResult>
    {
        private readonly ISerialStateContext _serialState;

        public CopyFileHandler(ISerialStateContext serialState)
        {
            _serialState = serialState;
        }

        public Task<CopyFileResult> Handle(CopyFileCommand request, CancellationToken cancellationToken)
        {
            _serialState.SendIntBytes(TeensyToken.CopyFile, 2);
            _serialState.SendIntBytes(request.StorageType.GetStorageToken(), 1);
            _serialState.Write($"{request.SourcePath}\0");
            _serialState.Write($"{request.DestPath}\0");
            _serialState.HandleAck();
            return Task.FromResult(new CopyFileResult());
        }
    }
}