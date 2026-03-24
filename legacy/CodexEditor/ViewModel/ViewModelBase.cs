using CodexEditor.Util;
using System;
using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Runtime.Serialization;

namespace CodexEditor.ViewModel
{
	[DataContract(IsReference = true)]
	public class ViewModelBase : INotifyPropertyChanged
	{
		public event PropertyChangedEventHandler? PropertyChanged;

		protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
		{
			Logger.Log(MessageType.Info, $"Updating property: {propertyName}");
			PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
		}
	}
}
