﻿using MediatR;
using TeensyRom.Cli.Core.Storage.Entities;

namespace TeensyRom.Cli.Core.Commands
{
    public class FavoriteFileCommand(TeensyStorageType storageType, string sourcePath, string targetPath) : IRequest<FavoriteFileResult>
    {
        public TeensyStorageType StorageType { get; } = storageType;
        public string SourcePath { get; } = sourcePath;
        public string TargetPath { get; } = targetPath;
    }
}