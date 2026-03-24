using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace CodexEngine.Renderer
{
	public enum TextureWrapMode : byte
	{
		Mirror,
		Stretch,
		Border
	};

	public enum TextureFilterMode : byte
	{
		Blur,
		Pixelate
	};

	[DataContract]
	[StructLayout(LayoutKind.Sequential)]
	public struct TextureProperties
	{
		[DataMember]
		public TextureWrapMode textureWrapMode = TextureWrapMode.Mirror;
		[DataMember]
		public TextureFilterMode textureFilterMode = TextureFilterMode.Blur;

		public TextureProperties() { }
	};

	[DataContract]
	[StructLayout(LayoutKind.Sequential)]
	public class Texture2D
	{
		[DataMember(Name = "FilePath")]
		[MarshalAs(UnmanagedType.LPStr)]
		private string _filePath;
		[DataMember(Name = "TextureProperties")]
		private TextureProperties _properties;

		// TODO: Come up with a better solution, loading the image everytime for the width and the height is bad.
		public int Width		{ get => Image.FromFile(_filePath).Width; }
		public int Height		{ get => Image.FromFile(_filePath).Height; }
		public string FilePath	{ get => _filePath; }
		
		public Texture2D(string filePath, TextureProperties properties)
		{
			_filePath = filePath;
			//OnDeserialized(new StreamingContext());
		}

		public Texture2D(string filePath) : this(filePath, new TextureProperties())
		{

		}

		/*
		[OnDeserialized]
		private void OnDeserialized(StreamingContext ctx)
		{
			_image = Image.FromFile(FilePath);
		}
		*/
	}
}
