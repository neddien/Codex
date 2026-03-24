using CodexEditor.Util;
using System.Windows.Controls;

namespace CodexEditor.View.Shared
{
	/// <summary>
	/// Interaction logic for LoggerView.xaml
	/// </summary>
	public partial class LoggerView : UserControl
	{
		public LoggerView()
		{
			InitializeComponent();
		}

		private void Button_Click(object sender, System.Windows.RoutedEventArgs e)
		{

		}

		private void ToggleButton_Click(object sender, System.Windows.RoutedEventArgs e)
		{
			byte filter = 0;

			if (msgTglBtn.IsChecked == true) filter |= (byte)(MessageType.Info);
			if (wrnTglBtn.IsChecked == true) filter |= (byte)(MessageType.Warning);
			if (errTglBtn.IsChecked == true) filter |= (byte)(MessageType.Error);

			Logger.SetFilter(filter);
		}
	}
}
