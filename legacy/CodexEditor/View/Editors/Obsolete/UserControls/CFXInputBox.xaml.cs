using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Controls;

namespace CodexEditor.View.MainWindow.UserControls
{
	/// <summary>
	/// Interaction logic for CFXInputBox.xaml
	/// </summary>
	public partial class CFXInputBox : UserControl, INotifyPropertyChanged
	{
		private string _defaultText = "Default text";

		public event PropertyChangedEventHandler? PropertyChanged;

		public string DefaultText
		{
			get
			{
				return _defaultText;
			}
			set
			{
				_defaultText = value;
				this.OnPropertyChanged();
			}
		}

		private void OnPropertyChanged([CallerMemberName] string name = null)
		{
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(name));
		}

		public CFXInputBox()
		{
			DataContext = this;
			InitializeComponent();
		}

		private void TextBox_TextChanged(object sender, TextChangedEventArgs e)
		{
			if (!string.IsNullOrEmpty(this.textBox.Text))
				this.greyText.Visibility = Visibility.Hidden;
			else
				this.greyText.Visibility = Visibility.Visible;
        }
    }
}
