using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Markup;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace CodexEditor.View.Editors.Components
{
	/// <summary>
	/// Interaction logic for ComponentView.xaml
	/// </summary>
	[ContentProperty("ComponentContent")]
	public partial class ComponentView : UserControl
	{
		public static readonly DependencyProperty HeaderProperty =
			DependencyProperty.Register(nameof(Header), typeof(string), typeof(ComponentView));

		public string Header
		{
			get { return (string)(GetValue(HeaderProperty)); }
			set { SetValue(HeaderProperty, value); }
		}

		public static readonly DependencyProperty ComponentContentProperty =
			DependencyProperty.Register(nameof(ComponentContent), typeof(FrameworkElement), typeof(ComponentView));

		public FrameworkElement ComponentContent
		{
			get { return (FrameworkElement)(GetValue(ComponentContentProperty)); }
			set { SetValue(ComponentContentProperty, value); }
		}

		public ComponentView()
		{
			InitializeComponent();
		}
	}
}
