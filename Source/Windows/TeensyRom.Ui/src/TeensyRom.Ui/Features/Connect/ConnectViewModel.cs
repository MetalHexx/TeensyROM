﻿using DynamicData;
using DynamicData.Kernel;
using MediatR;
using ReactiveUI;
using ReactiveUI.Fody.Helpers;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Data;
using System.Linq;
using System.Reactive.Concurrency;
using System.Reactive.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Controls;
using TeensyRom.Core.Commands;
using TeensyRom.Core.Logging;
using TeensyRom.Core.Serial;
using TeensyRom.Core.Serial.State;
using TeensyRom.Ui.Controls.FeatureTitle;
using TeensyRom.Ui.Features.Global;
using TeensyRom.Ui.Helpers.ViewModel;

namespace TeensyRom.Ui.Features.Connect
{
    public class ConnectViewModel: ReactiveObject
    {
        [ObservableAsProperty] public List<string> Ports { get; }
        [ObservableAsProperty] public bool IsConnected { get; set; }
        [ObservableAsProperty] public bool IsConnectable { get; set; }

        [Reactive] public FeatureTitleViewModel Title { get; set; }
        [Reactive] public string SelectedPort { get; set; } = string.Empty;

        public ReactiveCommand<Unit, Unit> ConnectCommand { get; set; }
        public ReactiveCommand<Unit, Unit> DisconnectCommand { get; set; }
        public ReactiveCommand<Unit, PingResult> PingCommand { get; set; }
        public ReactiveCommand<Unit, ResetResult> ResetCommand { get; set; }
        public ReactiveCommand<Unit, Unit> ClearLogsCommand { get; set; }
        public ObservableCollection<string> Logs { get; } = [];

        private readonly IMediator _mediator;
        private readonly ISerialStateContext _serial;

        public ConnectViewModel(IMediator mediator, ISerialStateContext serial, ILoggingService log, IAlertService alertService)
        {
            Title = new FeatureTitleViewModel("Connection");

            serial.Ports
                .Where(p => p is not null && p.Length > 0)
                .Select(p => p.ToList())
                .Select(p => 
                {
                    if (p.Count > 0) 
                    {
                        p.Insert(0, "Auto");
                    }   
                    return p;
                })
                .ToPropertyEx(this, vm => vm.Ports);

            this.WhenAnyValue(x => x.Ports)
                .Where(port => port != null)
                .Subscribe(ports => SelectedPort = ports.First());

            this.WhenAnyValue(x => x.SelectedPort)
                .Where(port => port is not null && !port!.Contains("Auto"))
                .Subscribe(port => serial.SetPort(port));

            serial.CurrentState
                .ObserveOn(RxApp.MainThreadScheduler)
                .Select(state => state is SerialConnectedState)
                .ToPropertyEx(this, vm => vm.IsConnected);

            serial.CurrentState
                .ObserveOn(RxApp.MainThreadScheduler)
                .Select(state => state is SerialConnectableState)                
                .ToPropertyEx(this, vm => vm.IsConnectable);

            ConnectCommand = ReactiveCommand.CreateFromTask<Unit, Unit>(
                execute: async n =>
                {
                    var success = SelectedPort.Contains("Auto") 
                        ? await TryAutoConnect()
                        : await TrySingleConnect();

                    if (!success)
                    {
                        alertService.Publish("Failed to connect to device.  Check to make sure you're using the correct com port.");
                    }
                        
                    return Unit.Default;
                },
                canExecute: this.WhenAnyValue(x => x.IsConnectable),
                outputScheduler: RxApp.MainThreadScheduler);

            DisconnectCommand = ReactiveCommand.Create<Unit, Unit>(
                execute: n => serial.ClosePort(),
                canExecute: this.WhenAnyValue(x => x.IsConnected),
                outputScheduler: RxApp.MainThreadScheduler);

            PingCommand = ReactiveCommand.CreateFromTask(
                execute: () => mediator.Send(new PingCommand()),
                canExecute: this.WhenAnyValue(x => x.IsConnected),
                outputScheduler: RxApp.MainThreadScheduler);

            ResetCommand = ReactiveCommand.CreateFromTask(
                execute: () => mediator.Send(new ResetCommand()), 
                canExecute: this.WhenAnyValue(x => x.IsConnected),
                outputScheduler: RxApp.MainThreadScheduler);

            ClearLogsCommand = ReactiveCommand.Create<Unit, Unit>(
                execute: _ =>
                {
                    Logs.Clear();
                    return Unit.Default;
                },
                canExecute: this.WhenAnyValue(x => x.Logs.Count).Select(count => count > 0),
                outputScheduler: RxApp.MainThreadScheduler);

            log.Logs
                .ObserveOn(RxApp.MainThreadScheduler)
                .Subscribe(logMessage =>
                {
                    Logs.Add(logMessage);

                    if (Logs.Count > 1000)
                    {
                        Logs.RemoveAt(0);
                    }
                });
            _mediator = mediator;
            _serial = serial;
        }

        private async Task<bool> TrySingleConnect()
        {
            _serial.OpenPort();
            var result = await _mediator.Send(new ResetCommand());

            if (!result.IsSuccess)
            {
                _serial.ClosePort();
                return false;
            }
            return true;
        }

        private async Task<bool> TryAutoConnect()
        {
            var legitPorts = Ports!
                .Where(p => p != "Auto")
                .OrderBy(p => p);

            foreach (var port in legitPorts)
            {
                _serial.SetPort(port);
                _serial.OpenPort();
                var result = await _mediator.Send(new ResetCommand());

                if (!result.IsSuccess)
                {
                    _serial.ClosePort();
                }
                else
                {
                    return true;
                }
            }
            return false;
        }
    }
}
