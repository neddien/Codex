using CodexEditor.View.GameProject;
using Microsoft.VisualBasic;
using System;
using System.Linq;
using System.Windows;

namespace CodexEditor.Views.ProjectBrowser
{
	/// <summary>
	/// Interaction logic for ProjectBrowser.xaml
	/// </summary>
	public partial class ProjectBrowser : Window
	{
		public ProjectBrowser()
		{
			InitializeComponent();
		}

		private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
		{

        }

		private void ToggleButton_Click(object sender, RoutedEventArgs e)
		{
			if (sender == openProjectBtn)
			{
				openProjectBtn.IsChecked = true;
				newProjectBtn.IsChecked = false;
				this.browserContent.Margin = new Thickness(0);
				this.openProjectView.UserControl_Loaded(this, new RoutedEventArgs());
			}
			else
			{
				newProjectBtn.IsChecked = true;
				openProjectBtn.IsChecked = false;
				this.browserContent.Margin = new Thickness(-800,0,0,0);
				this.newProjectView.UserControl_Loaded(this, new RoutedEventArgs());
			}
        }

		private void Window_Loaded(object sender, RoutedEventArgs e)
		{
			this.openProjectView.Focus();
			this.openProjectView.UserControl_Loaded(this, new RoutedEventArgs());

			if (!ViewModel.GameProject.OpenProject.Projects.Any())
			{
				openProjectBtn.IsEnabled = false;
				openProjectView.Visibility = Visibility.Hidden;
				ToggleButton_Click(newProjectBtn, null);
			}
		}
	}
}
