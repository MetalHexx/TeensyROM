﻿using TeensyRom.Cli.Core.Storage.Entities;

namespace TeensyRom.Cli.Core.Games
{
    public interface IGameMetadataService
    {
        GameItem EnrichGame(GameItem game);
    }
}