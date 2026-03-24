using CodexEditor.CoreAPI;
using CodexEditor.Util;
using CodexEditor.ViewModel.GameProject;
using System;
using System.Collections.Specialized;
using System.Reflection;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace CodexEditor.View.Editors
{
    /// <summary>
    /// Interaction logic for SceneEditorView.xaml
    /// </summary>
    public partial class SceneEditorView : UserControl
	{
		public static SceneEditorView Instance { get; private set; }

		public enum Tabs
		{
			TilePallete
		}

		public SceneEditorView()
		{
			InitializeComponent();
			Instance = this;
		}

		private void UserControl_Loaded(object sender, RoutedEventArgs e)
		{
			this.Focus();
		}

		public static void FocusAtTab(Tabs tab)
		{
			switch (tab)
			{
				case Tabs.TilePallete:
					{
						Instance.lowerRightTabCtrl.SelectedIndex = 1;
						break;
					}
			}
		}
	}
}