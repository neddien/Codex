using System;
using System.Numerics;
using System.Runtime.InteropServices;
using CodexEngine.Scene;

namespace CodexEngine.Components
{

	[StructLayout(LayoutKind.Sequential)]
	public class DescriptorTransformComponent
	{
		public Vector3 Position = new Vector3(0, 0, 0);
		public Vector3 Rotation = new Vector3(0, 0, 0);
		public Vector3 Scale = new Vector3(1, 1, 1);

		public DescriptorTransformComponent() 
		{

		}
	}

	[StructLayout(LayoutKind.Sequential)]
	public class DescriptorTagComponent
	{
		[MarshalAs(UnmanagedType.LPStr)]
		public string Tag = "default tag";

		public DescriptorTagComponent() 
		{ 
		
		}
	}

	[StructLayout(LayoutKind.Sequential)]
	public class DescriptorSpriteRendererComponent
	{
		private Vector4 _colour;
		private Sprite _sprite;
		private Int32 _zIndex;

		public Vector4 Colour	{ get => _colour; set => _colour = value; }
		public Sprite Sprite	{ get => _sprite; set => _sprite = value; }
		public Int32 ZIndex		{ get => _zIndex; set => _zIndex = value; }

		public DescriptorSpriteRendererComponent(Vector4 colour) : this(null, colour)
		{

		}

		public DescriptorSpriteRendererComponent(Sprite sprite) : this(sprite, new Vector4(1.0f))
		{

		}

		public DescriptorSpriteRendererComponent(Sprite sprite, Vector4 colour) 
		{
			_sprite = sprite;
			_colour = colour;
			_zIndex = 0;
		}
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct TileInfo
	{
		public Int32 Layer;
		public Vector2 TileID;
		public Vector2 WorldPos;
	}


	[StructLayout(LayoutKind.Sequential)]
	public class DescriptorTilemapComponent
	{
		[MarshalAs(UnmanagedType.LPStr)]
		public string TextureFilePath;
		public Vector2 GridSize;
		public Vector2 TileSize;
		public Int32 Layer;
		public Int32 TileCount;
		// TODO: Turn this into a struct so that Tiles can be marshaled as well.
		// Note: Changing it to a struct causes a weird type error.
		//public TileInfo[] Tiles;
	}
}
