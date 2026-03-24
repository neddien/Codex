using CodexEditor.Util;
using CodexEditor.ViewModel.ECS;
using CodexEditor.ViewModel.GameProject;
using CodexEngine;
using System.Linq;
using System.Windows;
using System.Windows.Controls;

namespace CodexEditor.View.Editors
{
    /// <summary>
    /// Interaction logic for SceneHierarchy.xaml
    /// </summary>
    public partial class SceneHierarchy : UserControl
    {
        public SceneHierarchy()
        {
            InitializeComponent();
        }

		private void createEntityBtn_Click(object sender, RoutedEventArgs e)
		{
            var btn = sender as Button;
            var da = btn.DataContext as Scene;
            da.AddEntityCmd.Execute(new Entity(da) { Name = $"Empty entity {da.Entities.Count}" });
        }

		private void ListBox_SelectionChanged(object sender, SelectionChangedEventArgs e)
		{
            var listBox = sender as ListBox;
            var newSelection = listBox.SelectedItems.Cast<Entity>().ToList();
            var previousSelection = newSelection.Except(e.AddedItems.Cast<Entity>()).Concat(e.RemovedItems.Cast<Entity>()).ToList();

            Project.Unredoable.Add(new UnredoableAction(
                () =>
                {
                    listBox.UnselectAll();
                    previousSelection.ForEach(x =>
					{
                        if ((listBox.ItemContainerGenerator.ContainerFromItem(x) as ListBoxItem) is ListBoxItem lbox)
                            lbox.IsSelected = true;
				    });
                },
                () =>
                {
                    listBox.UnselectAll();
                    newSelection.ForEach(x =>
                    {
                        if ((listBox.ItemContainerGenerator.ContainerFromItem(x) as ListBoxItem) is ListBoxItem lbox) 
                            lbox.IsSelected = true;
                    });
                },
                "Selection changed"
                ));
            MSEntity msEntity = null;
            if (newSelection.Any())
                msEntity = new MSEntity(newSelection);
            // TODO: Move this to the Entity class.
            CodexAPI.SetSelectedEntityID(msEntity.SelectedEntities.FirstOrDefault().EntityID);
            EntityEditorView.Instance.DataContext = msEntity;
        }

		private void UserControl_Loaded(object sender, RoutedEventArgs e)
		{

		}
	}
}
