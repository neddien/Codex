using CodexEditor.Util;
using CodexEditor.ViewModel.GameProject;
using System;
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

namespace CodexEditor.View.GameProject
{
	/// <summary>
	/// Interaction logic for OpenProject.xaml
	/// </summary>
	public partial class OpenProject : UserControl
	{
		public OpenProject()
		{
			InitializeComponent();
		}

		private void openBtn_Click(object sender, RoutedEventArgs e)
		{
			var projetData = projectListBox.SelectedItem as ProjectData;
			var project = ViewModel.GameProject.OpenProject.Open(projetData);
			var win = Window.GetWindow(this);
			bool dialogResult = false;
			if (project != null)
			{
				dialogResult = true;
				win.DataContext = project;
			}
			win.DialogResult = dialogResult;
			win.Close();
		}

		private void quitBtn_Click(object sender, RoutedEventArgs e)
		{
			EngineHost.SignalTermination();
			Environment.Exit(0);
        }

		public void UserControl_Loaded(object sender, RoutedEventArgs e)
		{
			if (projectListBox.SelectedItem != null)
			{
				projectListBox.ScrollIntoView(projectListBox.SelectedItem);
				projectListBox.Focus();
			}
		}
	}
}
