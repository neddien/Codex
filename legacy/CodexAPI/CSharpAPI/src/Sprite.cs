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

		[DataMember]
		public Texture2D Texture { get => _texture; set => _texture = value; }
		[DataMember]

		public Sprite(Texture2D texture)
		{
			_texture = texture;
		}
		{
			_texture = texture;
		}
	}
}
