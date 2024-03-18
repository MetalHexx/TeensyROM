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
using System.Windows.Media.Animation;
using System.Windows.Media.Imaging;
using System.Windows.Media.Media3D;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace TeensyRom.Ui.Controls.LibraryFilter
{
    /// <summary>
    /// Interaction logic for LibraryFilterView.xaml
    /// </summary>
    public partial class LibraryFilterView : UserControl
    {
        public LibraryFilterView()
        {
            InitializeComponent();
        }

        private void RadioButton_Loaded(object sender, RoutedEventArgs e)
        {
            var radioButton = sender as RadioButton;

            if (radioButton == null) return;

            var parentDataContext = this.DataContext;
            var viewModel = parentDataContext as LibraryFilterViewModel;

            if (viewModel == null) return;

            if (viewModel.SelectedLibrary is not null && radioButton.DataContext == viewModel.SelectedLibrary)
            {
                radioButton.IsChecked = true;
                return;
            }
            if (viewModel.SelectedLibrary is null && radioButton.DataContext == viewModel.Libraries.FirstOrDefault())
            {
                radioButton.IsChecked = true;
            }
        }

        private void Random_Click(object sender, RoutedEventArgs e)
        {
            var rotateAnimation = new DoubleAnimation
            {
                From = 0,
                To = 360,
                Duration = TimeSpan.FromSeconds(0.5),
                AccelerationRatio = 0.5,
                DecelerationRatio = 0.5
            };

            IconRotateTransform.BeginAnimation(RotateTransform.AngleProperty, rotateAnimation);
        }
    }
}
