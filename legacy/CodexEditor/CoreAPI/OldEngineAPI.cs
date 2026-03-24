using System;
using System.Runtime.InteropServices;

namespace CodexEditor.CoreAPI
{
	public static class OldEngineAPI
    {
        public const string m_EngineDll = @"../../../../builds/vs2022/CodexAPI/Debug/CodexAPI.dll";

        [StructLayout(LayoutKind.Sequential)]
		public struct WindowProperties
		{
			[MarshalAs(UnmanagedType.LPStr)]
			public string Title;
			public int Width;
			public int Height;
			public int PosX;
			public int PosY;
			public int FrameCap;
			public UInt32 Flags;
			public bool Vsync;

			public WindowProperties(
				string title = "hello from c#",
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

		// Engine API Imports
		[DllImport(m_EngineDll)]
		public static extern IntPtr GetInstance();
		[DllImport(m_EngineDll)]
		public static extern void CreateWindow(IntPtr inst, ref WindowProperties props, nint nativeHandle = 0); // dont touch the last parameter
		[DllImport(m_EngineDll)]
		public static extern void CreateChildWindow(IntPtr inst, ref WindowProperties props, nint parentHandle, ref nint handle);
		[DllImport(m_EngineDll)]
		public static extern void Dispose(IntPtr inst);
		[DllImport(m_EngineDll)]
		public static extern IntPtr ReadBackBufferTo(IntPtr inst);
		[DllImport(m_EngineDll)]
		public static extern void StartEngineThread(IntPtr inst);
		[DllImport(m_EngineDll)]
		public static extern void Update(IntPtr inst);
        [DllImport(m_EngineDll)]
        public static extern void ResizeViewport(IntPtr inst, int newWidth, int newHeight);
    }
}