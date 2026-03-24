using CodexEditor.CoreAPI;
using CodexEditor.Util;
using CodexEngine;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
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

namespace CodexEditor.View.Shared
{
	/// <summary>
	/// Interaction logic for EngineHostView.xaml
	/// </summary>
	public partial class EngineHostView : UserControl
	{
		public static EngineHost EngineInstance;

		public EngineHostView()
		{
			InitializeComponent();
			DefaultStyleKey = typeof(Border);
			EngineInstance = new EngineHost(hostBorder);
			hostBorder.Child = EngineInstance;
		}
	}
}
