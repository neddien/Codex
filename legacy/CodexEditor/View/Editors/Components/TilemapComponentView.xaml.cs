using CodexEditor.ViewModel.ECS;
using CodexEditor.ViewModel.Editors;
using CodexEngine;
using CodexEngine.Renderer;
using CodexEngine.Scene;
using Microsoft.Win32;
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

namespace CodexEditor.View.Editors.Components
{
    /// <summary>
    /// Interaction logic for TilemapComponentView.xaml
    /// </summary>
    public partial class TilemapComponentView : UserControl
    {
        public TilemapComponentView()
        {
            InitializeComponent();
        }

		private void loadBtn_Click(object sender, RoutedEventArgs e)
		{
			var da = DataContext as MSTilemapComponent;
			var fileDialog = new OpenFileDialog();
			fileDialog.Filter = "Image files (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg|All files (*.*)|*.*";
			fileDialog.Multiselect = false;
			fileDialog.Title = "Open an image.";
			if (fileDialog.ShowDialog() == true)
			{
				da.Texture = new Texture2D(fileDialog.FileName);
			}
		}

		private void palleteBtn_Click(object sender, RoutedEventArgs e)
		{
			var da = DataContext as MSTilemapComponent;
			var editor = TileEditor.Instance;
			editor.ActiveEntity = da.SelectedComponents.FirstOrDefault().Parent;
			TileEditorView.SetCurrentActiveAction(CodexEngine.Enums.EditorAction.TilemapBrush);
			SceneEditorView.FocusAtTab(SceneEditorView.Tabs.TilePallete);
        }
    }
}
