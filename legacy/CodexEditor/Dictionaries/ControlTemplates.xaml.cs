using System;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace CodexEditor.Dictionaries
{
	public partial class ControlTemplates : ResourceDictionary
	{
		private void OnTextBox_KeyDown(object sender, KeyEventArgs e)
		{
			var textBox = sender as TextBox;
			var exp = textBox.GetBindingExpression(TextBox.TextProperty);
			if (exp == null) return;

			if (e.Key == Key.Enter)
			{
				if (textBox.Tag is ICommand command && command.CanExecute(textBox.Text))
					command.Execute(textBox.Text);
				else
					exp.UpdateSource();
				Keyboard.ClearFocus();
				e.Handled = true;
			}
			else if (e.Key == Key.Escape)
			{
				exp.UpdateTarget();
				Keyboard.ClearFocus();
			}
		}

		private void TextBoxWithRename_KeyDown(object sender, KeyEventArgs e)
		{
			var textBox = sender as TextBox;
			var exp = textBox.GetBindingExpression(TextBox.TextProperty);
			if (exp == null) return;

			if (e.Key == Key.Enter)
			{
				if (textBox.Tag is ICommand command && command.CanExecute(textBox.Text))
					command.Execute(textBox.Text);
				else
					exp.UpdateSource();
				textBox.Visibility = Visibility.Collapsed;
				e.Handled = true;
			}
			else if (e.Key == Key.Escape)
			{
				exp.UpdateTarget();
				textBox.Visibility = Visibility.Collapsed;
			}
		}

		private void TextBoxWithRename_LostFocus(object sender, RoutedEventArgs e)
		{
			var textBox = sender as TextBox;
			if (!textBox.IsVisible) return;
			var exp = textBox.GetBindingExpression(TextBox.TextProperty);
			if (exp != null)
			{
				exp.UpdateTarget();
				textBox.Visibility = Visibility.Collapsed;
			}
		}
    }
}
