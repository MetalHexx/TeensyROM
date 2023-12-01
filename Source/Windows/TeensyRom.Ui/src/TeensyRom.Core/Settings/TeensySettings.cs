﻿using Newtonsoft.Json;
using System.Runtime;
using TeensyRom.Core.Common;
using TeensyRom.Core.Storage.Entities;

namespace TeensyRom.Core.Settings
{
    /// <summary>
    /// Used to persist and retrieve user preference from disk.  See: Settings.json in the bin folder
    /// </summary>
    public record TeensySettings
    {
        public TeensyStorageType TargetType { get; set; } = TeensyStorageType.SD;
        public string WatchDirectoryLocation { get; set; } = string.Empty;
        public string TargetRootPath { get; set; } = @"/sync";
        public List<TeensyTarget> FileTargets { get; set; } = new List<TeensyTarget>();
        public bool AutoFileCopyEnabled { get; set; }
        public bool SaveMusicCacheEnabled { get; set; }        

        public TeensySettings()
        {
            GetDefaultBrowserDownloadPath();
        }

        public void InitializeDefaults()
        {
            FileTargets.AddRange(new List<TeensyTarget>
            {
                new TeensyTarget
                {
                    Type = TeensyFileType.Sid,
                    DisplayName = "SID",
                    Extension = ".sid",
                    TargetPath = "sid"
                },
                new TeensyTarget
                {
                    Type = TeensyFileType.Prg,
                    DisplayName = "PRG",
                    Extension = ".prg",
                    TargetPath = "prg"
                },
                new TeensyTarget
                {
                    Type = TeensyFileType.Crt,
                    DisplayName = "CRT",
                    Extension = ".crt",
                    TargetPath = "crt"
                },
                new TeensyTarget
                {
                    Type = TeensyFileType.Hex,
                    DisplayName = "HEX",
                    Extension = ".hex",
                    TargetPath = "hex"
                }
            });
        }

        public string GetFileTypePath(TeensyFileType type)
        {
            var fileTarget = FileTargets.First(t => t.Type == type);

            return TargetRootPath.UnixPathCombine(fileTarget.TargetPath);
        }

        /// <summary>
        /// If the user has no settings file saved yet, we'll default to the 
        /// environmentally defined location for the user profile download directory
        /// </summary>
        private void GetDefaultBrowserDownloadPath()
        {
            string userProfile = Environment.GetFolderPath(Environment.SpecialFolder.UserProfile);
            WatchDirectoryLocation = Path.Combine(userProfile, "Downloads");
        }
    }
}