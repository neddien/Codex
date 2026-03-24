using CodexEditor.ViewModel.GameProject;
using System;
using System.Windows;
using System.Windows.Controls;

namespace CodexEditor.View.GameProject
{
    /// <summary>
    /// Interaction logic for NewProject.xaml
    /// </summary>
    public partial class NewProjectView : UserControl
    {
        public NewProjectView()
        {
            InitializeComponent();
        }

		private void createBtn_Click(object sender, RoutedEventArgs e)
		{
            var da = DataContext as NewProject;
            var projectTemplate = templateListBox.SelectedItem as ProjectTemplate;
			var project = da.CreateProject(projectTemplate);
            var win = Window.GetWindow(this);
            bool dialogResult = false;
            if (!string.IsNullOrEmpty(project.ProjectPath))
			{
				dialogResult = true;
				var projectData = new ProjectData()
                {
					ProjectName = project.ProjectName,
                    ProjectPath = project.ProjectPath,
                    PreviewImage = projectTemplate.PreviewImage,
                    Icon = projectTemplate.Icon
                };
                win.DataContext = project;
                ViewModel.GameProject.OpenProject.Open(projectData);
            }
			win.DialogResult = dialogResult;
			win.Close();
		}

		private void quitBtn_Click(object sender, RoutedEventArgs e)
		{
            Environment.Exit(0);
		}

		public void UserControl_Loaded(object sender, RoutedEventArgs e)
		{
            if (templateListBox.SelectedItem != null)
			{
				templateListBox.ScrollIntoView(templateListBox.SelectedItem);
				templateListBox.Focus();
			}    
		}
    }
}
