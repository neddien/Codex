using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Drawing;
using CodexEngine.Renderer;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;

namespace CodexEngine.Scene
{
	[DataContract]
	[StructLayout(LayoutKind.Sequential)]
	public class Sprite
	{
		private Texture2D _texture;
		private RectangleF _srcRect;

		[DataMember]
		public Texture2D Texture { get => _texture; set => _texture = value; }
		[DataMember]
		public RectangleF TextureCoords { get => _srcRect; set => _srcRect = value; }

		public Sprite(Texture2D texture)
		{
			_texture = texture;
			_srcRect = new RectangleF(0.0f, 0.0f, (float)texture.Width, (float)texture.Height);
		}
		public Sprite(Texture2D texture, RectangleF textureCoords)
		{
			_texture = texture;
			_srcRect = textureCoords;
		}
	}
}
