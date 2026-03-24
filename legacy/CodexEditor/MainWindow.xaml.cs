using CodexEditor.ViewModel.GameProject;
using CodexEditor.Views.ProjectBrowser;
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
using System.Windows.Shapes;

namespace CodexEditor
{
	/// <summary>
	/// Interaction logic for MainWindow.xaml
	/// </summary>
	public partial class MainWindow : Window
	{
		public MainWindow()
		{
			InitializeComponent();
		}

		private void Window_Loaded(object sender, RoutedEventArgs e)
		{
			this.Hide();
			var projectBrowser = new ProjectBrowser();
			var dialogResult = projectBrowser.ShowDialog();
			if (dialogResult == false || projectBrowser.DataContext == null)
				Environment.Exit(-1);
			//Project.Current?.Unload();
			this.DataContext = projectBrowser.DataContext;
			this.Show();
		}
	}
}
