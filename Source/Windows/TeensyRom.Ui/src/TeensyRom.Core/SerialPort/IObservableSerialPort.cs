﻿using System.Reactive;

namespace TeensyRom.Core.Serial
{
    /// <summary>
    /// Provides an observable interface to a serial port that can be interacted with
    /// </summary>
    public interface IObservableSerialPort
    {
        /// <summary>
        /// Connections can be dropped, thus ports can change, so we want to observe those changes.
        /// </summary>
        IObservable<string[]> Ports { get; }

        /// <summary>
        /// The current connection state
        /// </summary>
        IObservable<bool> IsConnected { get; }

        /// <summary>
        /// All the log data from serial port communications
        /// </summary>
        IObservable<string> Logs { get; }

        /// <summary>
        /// Sets the port to connect to
        /// </summary>
        void SetPort(string port);

        /// <summary>
        /// Opens the port with the current set port
        /// </summary>
        Unit OpenPort();

        //TODO: Add a method to close the port
        //TODO: Add a method to send file data
    }
}