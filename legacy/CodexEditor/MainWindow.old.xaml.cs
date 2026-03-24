using CodexEditor.ViewModel;
using CodexEditor.CoreAPI;
using System;
using System.ComponentModel;
using System.Windows;
using System.Windows.Interop;
using CodexEditor.Views.ProjectBrowser;
using CodexEditor.ViewModel.GameProject;

namespace CL.CodexEditor
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindowOld : Window
	{
		private EngineHostControl _control;

		public MainWindowOld()
		{
			//this.Hide();
			//InitializeComponent();
			//WindowState = WindowState.Maximized;
		}

		private void Window_OnLoaded(object sender, RoutedEventArgs e)
		{
			var projectBrowser = new ProjectBrowser();
			var dialogResult = projectBrowser.ShowDialog();
			if (dialogResult == false || projectBrowser.DataContext == null)
				Environment.Exit(-1);
			Project.Current?.Unload();
			this.DataContext = projectBrowser.DataContext;
			this.Show();
		}

		private void Window_OnClosing(object sender, CancelEventArgs e)
		{
			Project.Current?.Unload();
		}

		/*private void Window_OnLoaded(object sender, RoutedEventArgs e)
		{
			_control = new EngineHostControl(
				this.EngineBorderHost, 
				(int)(this.EngineBorderHost.ActualWidth),
				(int)(this.EngineBorderHost.ActualHeight)
			);
			this.EngineBorderHost.Child = _control;
			_control.Focusable = true;
			this.EngineBorderHost.Focusable = true;
			_control.Focus();
		}

		private void Window_OnClosing(object sender, CancelEventArgs e)
		{
			_control.Dispose();
		}*/
	}
}
