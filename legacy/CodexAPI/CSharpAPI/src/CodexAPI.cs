using System;
using System.Numerics;
using System.Runtime.InteropServices;
using CodexEngine.Components;
using CodexEngine.Enums;
using CodexEngine.Structs;

namespace CodexEngine.Enums
{ 
	public enum EditorAction : byte
	{
		Select,
		TilemapBrush,
		TilemapErase
	}
}

namespace CodexEngine.Structs
{
	[StructLayout(LayoutKind.Sequential)]
	public struct WindowProperties
	{
		[MarshalAs(UnmanagedType.LPStr)]
		public string Title = "embedded window";
		public int Width = 1280;
		public int Height = 720;
		public int PosX = 30;
		public int PosY = 30;
		public int FrameCap = 0;
		public UInt32 Flags = 0;
		public bool Vsync = true;

		public WindowProperties(
			string title = "embedded window",
			int width = 1280,
			int height = 720,
			int posX = 0,
			int posY = 0,
			int frameCap = 0,
			UInt32 flags = 2,
			bool vsync = true
			)
		{
			Title = title;
			Width = width;
			Height = height;
			PosX = posX;
			PosY = posY;
			FrameCap = frameCap;
			Flags = flags;
			Vsync = vsync;
		}
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct EntityDescriptor
	{
		public DescriptorTransformComponent Transform = null;
		public DescriptorTagComponent Tag = null;
		public DescriptorSpriteRendererComponent SpriteRenderer = null;

		public EntityDescriptor() { }
	}

	[StructLayout(LayoutKind.Sequential)]
	public struct Vector2f
	{
		public float X;
		public float Y;
	}
}

namespace CodexEngine
{
	public static class CodexAPI
	{
		public const string ENGINE_DLL = @"CodexNative.dll";

		// Engine API Imports
		[DllImport(ENGINE_DLL)]
		public static extern IntPtr Init();
		[DllImport(ENGINE_DLL)]
		public static extern void CreateWindow(ref Structs.WindowProperties props, nint nativeHandle = 0); // do not touch the last parameter
		[DllImport(ENGINE_DLL)]
		public static extern void Destroy();
		[DllImport(ENGINE_DLL)]
		public static extern void StartEngineThread();
		[DllImport(ENGINE_DLL)]
		public static extern void Update();
		[DllImport(ENGINE_DLL)]
		public static extern void ResizeViewport(int newWidth, int newHeight);
		[DllImport(ENGINE_DLL)]
		public static extern int CreateEntity(EntityDescriptor desc);
		[DllImport(ENGINE_DLL)]
		public static extern void RemoveEntity(int entityID);
		[DllImport(ENGINE_DLL)]
		public static extern void UpdateEntityFromDescriptor(int entityID, EntityDescriptor desc);

		// Adds
		[DllImport(ENGINE_DLL)]
		public static extern void AddSpriteRendererComponent(int entityID, DescriptorSpriteRendererComponent component);
		[DllImport(ENGINE_DLL)]
		public static extern void AddTilemapComponent(int entityID, DescriptorTilemapComponent component, TileInfo[] tiles);

		// Updates
		[DllImport(ENGINE_DLL)]
		public static extern void UpdateTransformComponent(int entityID, DescriptorTransformComponent component);
		[DllImport(ENGINE_DLL)]
		public static extern void UpdateTagComponent(int entityID, DescriptorTagComponent component);
		[DllImport(ENGINE_DLL)]
		public static extern void UpdateSpriteRendererComponent(int entityID, DescriptorSpriteRendererComponent component);
		[DllImport(ENGINE_DLL)]
		public static extern void UpdateTilemapComponent(int entityID, DescriptorTilemapComponent component, TileInfo[] tiles);
		[DllImport(ENGINE_DLL)]
		public static extern Vector2 GetSelectedTileCoord();
		[DllImport(ENGINE_DLL)]
		public static extern void SetSelectedTileCoord(Vector2 newCoord);
		[DllImport(ENGINE_DLL)]
		public static extern void GetMousePositionInWorld(ref Vector2 vec);
		[DllImport(ENGINE_DLL)]
		public static extern void SetActiveAction(EditorAction newAction);
		[DllImport(ENGINE_DLL)]
		public static extern void SetSelectedEntityID(int entityID);
	}
}