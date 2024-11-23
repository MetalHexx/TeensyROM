﻿namespace TeensyRom.Cli.Core.Storage.Entities
{
    public class ExtractionResult
    {
        public string OriginalFileName { get; }
        public List<FileInfo> ExtractedFiles { get; }


        public ExtractionResult(string fileName, List<FileInfo> extractedFiles)
        {
            OriginalFileName = fileName;
            ExtractedFiles = extractedFiles;

        }
    }
}