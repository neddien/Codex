using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace CodexEditor.ViewModel
{
	internal class RelayCommand<T> : ICommand
	{
		private readonly Action<T> _execute;
		private readonly Predicate<T> _canExecute;

		public event EventHandler? CanExecuteChanged
		{
			add { CommandManager.RequerySuggested += value; }
			remove { CommandManager.RequerySuggested -= value; }
		}

		public RelayCommand(Action<T> execute, Predicate<T> canExecute = null)
		{
			_execute = (execute != null) ? execute : throw new ArgumentNullException(nameof(execute));
			_canExecute = canExecute;
		}

		public bool CanExecute(object? arg)
		{
			return _canExecute?.Invoke((T)(arg)) ?? true;
		}

		public void Execute(object? arg)
		{
			_execute((T)(arg));
		}
	}
}
