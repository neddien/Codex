using CodexEditor.ViewModel.ECS;
using System;
using System.IO;
using System.Runtime.Serialization;
using System.Text;
using System.Xaml;
using System.Xml;

namespace CodexEditor.Util
{
	public static class Serializer
	{
		public static void ToFile<T>(string path, T value)
		{
			try
			{
				var settings = new XmlWriterSettings
				{
					Indent = true,
					IndentChars = "\t",
					NewLineChars = "\n"
				};

				var serializer = new DataContractSerializer(typeof(T)); 
				using (var memoryStream = new MemoryStream())
				{
					using (var xmlWriter = XmlWriter.Create(memoryStream, settings))
					{
						serializer.WriteObject(xmlWriter, value);
					}

					File.WriteAllBytes(path, memoryStream.ToArray());	
				}
			}
			catch (Exception ex)
			{
				Console.WriteLine(ex.Message);
				Logger.Log(MessageType.Error, $"Failed to serialize file {path}: {ex.Message}");
				throw;
			}
		}

		public static T FromFile<T>(string path)
		{
			try
			{
				using var fs = new FileStream(path, FileMode.Open);
				var serializer = new DataContractSerializer(typeof(T));
				T instance = (T)(serializer.ReadObject(fs));
				return instance;
			}
			catch (Exception ex)
			{
				Console.WriteLine(ex.Message);
				Logger.Log(MessageType.Error, $"Failed to deserialize file {path}: {ex.Message}");
				throw; 
			}
		}
	}
}
