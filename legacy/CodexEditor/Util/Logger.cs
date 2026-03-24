using System;
using System.Collections.ObjectModel;
using System.IO;
using System.Runtime.CompilerServices;
using System.Windows;
using System.Windows.Data;

namespace CodexEditor.Util
{
	public enum MessageType : byte
	{
		Info = 0x01,
		Warning = 0x02,
		Error = 0x03
	}

	public class LogMessage
	{
		public DateTime Date { get; }
		public MessageType MessageType { get; }
		public string Message { get; }
		public string File { get; }
		public string Caller { get; }
		public int Line { get; }
		public string MetaData => $"[{File}]: {Caller}  ({Line})";

		public LogMessage(
			MessageType type,
			string message,
			string file,
			string caller,
			int line)
		{
			Date = DateTime.Now;
			MessageType = type;
			Message = message;
			File = Path.GetFileName(file);
			Caller = caller;
			Line = line;
		}
	}

	public static class Logger
	{
		private static byte _messageFilter = (byte)(MessageType.Info | MessageType.Warning | MessageType.Error);
		private static readonly ObservableCollection<LogMessage> _messages = new ObservableCollection<LogMessage>();
		public static ReadOnlyObservableCollection<LogMessage> Messages { get; } = new ReadOnlyObservableCollection<LogMessage>(_messages);
		public static CollectionViewSource FilteredMessages { get; } = new CollectionViewSource() { Source = Messages };

		static Logger()
		{
			FilteredMessages.Filter += (s, e) =>
			{
				var item = e.Item as LogMessage;
				e.Accepted = ((byte)(item.MessageType) & _messageFilter) == (byte)(item.MessageType);
			};
		}

		public static async void Log(
			MessageType type,
			string message,
			[CallerFilePath] string file = "",
			[CallerMemberName] string caller = "",
			[CallerLineNumber] int line = 0)
		{
			await Application.Current.Dispatcher.BeginInvoke(new Action(() =>
			{
				_messages.Add(new LogMessage(type, message, file, caller, line));
			}));
		}

		public static async void Clear()
		{
			await Application.Current.Dispatcher.BeginInvoke(new Action(() => _messages.Clear()));
		}

		public static void SetFilter(byte mask)
		{
			_messageFilter = mask;
			FilteredMessages.View.Refresh();
		}
	}
}