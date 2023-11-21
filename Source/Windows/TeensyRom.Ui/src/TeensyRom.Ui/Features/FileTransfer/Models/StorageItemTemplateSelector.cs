﻿using System.Windows.Controls;
using System.Windows;
using TeensyRom.Ui.Features.Common.Models;

namespace TeensyRom.Ui.Features.FileTransfer.Models
{
    public class StorageItemTemplateSelector : DataTemplateSelector
    {
        public DataTemplate DirectoryTemplate { get; set; } = new();
        public DataTemplate FileTemplate { get; set; } = new();

        public override DataTemplate SelectTemplate(object item, DependencyObject container)
        {
            return item switch
            {
                DirectoryItemVm _ => DirectoryTemplate,
                FileItemVm _ => FileTemplate,
                _ => base.SelectTemplate(item, container)
            };
        }
    }
}
