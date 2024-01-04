﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using TeensyRom.Core.Storage.Entities;
using TeensyRom.Ui.Features.Music.SongList;

namespace TeensyRom.Ui.Features.Files.DirectoryContent
{
    /// <summary>
    /// Interaction logic for DirectoryContentView.xaml
    /// </summary>
    public partial class DirectoryContentView : UserControl
    {
        public DirectoryContentView()
        {
            InitializeComponent();
        }

        private void OnListViewDoubleClicked(object sender, MouseButtonEventArgs e)
        {
            ListView? listView = sender as ListView;

            if (TryOpenDirectory(listView)) return;
        }

        private bool TryOpenDirectory(ListView? listView)
        {
            if (listView?.SelectedItem is DirectoryItem directoryItem)
            {
                var viewModel = (DirectoryContentViewModel)DataContext;
                viewModel.LoadDirectoryCommand.Execute(directoryItem).Subscribe();
                return true;
            }
            return false;
        }

        private void OnListViewPreviewMouseWheel(object sender, MouseWheelEventArgs e)
        {
            if (!e.Handled)
            {
                e.Handled = true;
                var eventArg = new MouseWheelEventArgs(e.MouseDevice, e.Timestamp, e.Delta)
                {
                    RoutedEvent = MouseWheelEvent,
                    Source = sender
                };

                var parent = ((Control)sender).Parent as UIElement;
                parent?.RaiseEvent(eventArg);
            }
        }
    }
}