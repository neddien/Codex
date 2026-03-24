using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Controls.Primitives;

namespace CodexEditor.ViewModel.Shared
{
    class DropDownMenu : ContentControl
    {
        public static readonly DependencyProperty IsOpenProperty =
            DependencyProperty.Register("IsOpen", typeof(bool), typeof(DropDownMenu), new PropertyMetadata(false));

        public bool IsOpen
        {
            get => (bool)GetValue(IsOpenProperty); 
            set => SetValue(IsOpenProperty, value);
        }

		public static readonly DependencyProperty TextProperty =
			DependencyProperty.Register("Text", typeof(string), typeof(DropDownMenu), new PropertyMetadata(""));

		public string Text
		{
			get => (string)GetValue(TextProperty);
			set => SetValue(TextProperty, value);
		}

		public static readonly RoutedEvent ClickEvent = EventManager.RegisterRoutedEvent(
			"Click",
			RoutingStrategy.Bubble,
			typeof(RoutedEventHandler),
			typeof(DropDownMenu));

		public event RoutedEventHandler Click
		{
			add { AddHandler(ClickEvent, value); }
			remove { RemoveHandler(ClickEvent, value); }
		}

		private Button _button;
		private Popup _popup;

		public override void OnApplyTemplate()
		{
			base.OnApplyTemplate();

			_button = GetTemplateChild("PART_Button") as Button;
			_popup = GetTemplateChild("PART_Popup") as Popup;

			if (_button != null)
				_button.Click += Button_Click;
		}

		private void Button_Click(object sender, RoutedEventArgs e)
		{
			IsOpen = !IsOpen;

			var args = new RoutedEventArgs(ClickEvent, this);
			RaiseEvent(args);
		}
	}
}
