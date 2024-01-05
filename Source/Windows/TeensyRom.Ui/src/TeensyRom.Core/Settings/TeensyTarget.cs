﻿using TeensyRom.Core.Storage.Entities;

namespace TeensyRom.Core.Settings
{
    public class TeensyTarget
    {        
        public TeensyFileType Type { get; set; }
        public TeensyLibraryType LibraryType { get; set; }
        public string DisplayName { get; set; } = string.Empty;
        public string Extension { get; set; } = string.Empty;
    }
}