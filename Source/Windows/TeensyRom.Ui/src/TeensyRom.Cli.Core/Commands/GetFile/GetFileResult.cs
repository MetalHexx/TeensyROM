﻿namespace TeensyRom.Cli.Core.Commands.GetFile
{
    public class GetFileResult : TeensyCommandResult
    {
        public byte[] FileData { get; set; } = default!;
    }
}