using CodexEditor.CoreAPI;
using CodexEditor.ViewModel.ECS;
using Microsoft.Win32;
using System.Windows;
using System.Windows.Controls;
using CodexEngine.Scene;
using CodexEngine.Renderer;

namespace CodexEditor.View.Editors.Components
{
	/// <summary>
	/// Interaction logic for SpriteRendererComponentView.xaml
	/// </summary>
	public partial class SpriteRendererComponentView : UserControl
	{
		private bool _propertyChanged = false;

		public SpriteRendererComponentView()
		{
			InitializeComponent();
		}

		private void UserControl_Loaded(object sender, RoutedEventArgs e)
		{
			(DataContext as MSSpriteRendererComponent).PropertyChanged += (s, e) => _propertyChanged = true;
		}

		private void loadBtn_Click(object sender, RoutedEventArgs e)
		{
			var da = DataContext as MSSpriteRendererComponent;
			var fileDialog = new OpenFileDialog();
			fileDialog.Filter = "Image files (*.png;*.jpg;*.jpeg)|*.png;*.jpg;*.jpeg|All files (*.*)|*.*";
			fileDialog.Multiselect = false;
			fileDialog.Title = "Open an image.";
			if (fileDialog.ShowDialog() == true)
			{
				da.Sprite = new Sprite(new Texture2D(fileDialog.FileName));
			}
		}
	}
}
