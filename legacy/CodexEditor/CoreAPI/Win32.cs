using System;
using System.Windows;
using System.Runtime.InteropServices;
using static CodexEditor.CoreAPI.Win32;

namespace CodexEditor.CoreAPI
{
	public static class Win32
	{
		public delegate IntPtr LowLevelMouseProc(int nCode, IntPtr wParam, IntPtr lParam);
		public delegate IntPtr WindowProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam);

		[DllImport("user32.dll", SetLastError = true)]
		public static extern IntPtr CreateWindowEx(uint dwExStyle, string lpClassName, string lpWindowName, uint dwStyle,
			uint x, uint y, uint nWidth, uint nHeight, IntPtr hWndParent, IntPtr hMenu, IntPtr hInstance, IntPtr lpParam);
		[DllImport("user32.dll", SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool DestroyWindow(IntPtr hWnd);
		[DllImport("user32.dll", CharSet = CharSet.Unicode)]
		public static extern IntPtr GetDC(IntPtr hWnd);
		[DllImport("gdi32.dll")]
		public static extern int ChoosePixelFormat(IntPtr hdc, ref PIXELFORMATDESCRIPTOR pfd);
		[DllImport("gdi32.dll")]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool SetPixelFormat(IntPtr hdc, int iPixelFormat, ref PIXELFORMATDESCRIPTOR pfd);
		[DllImport("gdi32.dll", CharSet = CharSet.Auto, SetLastError = true, ExactSpelling = true)]
		public static extern bool SwapBuffers(IntPtr hDC);
		[DllImport("user32.dll", SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool IsWindowEnabled(IntPtr hwnd);
		[DllImport("user32.dll")]
		public static extern bool ScreenToClient(IntPtr hwnd, ref Point lpPoint); 
		[DllImport("user32.dll")]
		public static extern bool SetCursorPos(int x, int y);
		[DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
		public static extern IntPtr SetWindowsHookEx(int idHook, LowLevelMouseProc lpfn, IntPtr hMod, uint dwThreadId);

		[DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
		[return: MarshalAs(UnmanagedType.Bool)]
		public static extern bool UnhookWindowsHookEx(IntPtr hhk);

		[DllImport("user32.dll", CharSet = CharSet.Auto, SetLastError = true)]
		public static extern IntPtr CallNextHookEx(IntPtr hhk, int nCode, IntPtr wParam, IntPtr lParam);

		[DllImport("kernel32.dll", CharSet = CharSet.Auto, SetLastError = true)]
		public static extern IntPtr GetModuleHandle(string lpModuleName);

		public const int WS_CHILD = 0x40000000;
		public const int WS_VISIBLE = 0x10000000;
		public const int WS_OVERLAPPEDWINDOW = 0x00CF0000;
		public const int WS_CLIPSIBLINGS = 0x04000000;
		public const int WS_EX_CONTROLPARENT = 0x00010000;
		public const int COLOR_WINDOW = 5;
		public const int IDC_ARROW = 32512;
		public const int CS_HREDRAW = 2;
		public const int CS_VREDRAW = 1;
		public const int CS_OWNDC = 0x0020;
		public const int CW_USEDEFAULT = unchecked((int)-2147483648);
		public const int GWL_STYLE = -16;
		public const byte PFD_OVERLAY_PLANE = 1;
		public const byte PFD_UNDERLAY_PLANE = 255;
		public const byte PFD_TYPE_COLORINDEX = 1;
		public const byte PFD_TYPE_RGBA = 0;
		public const uint PFD_DRAW_TO_WINDOW = 0x00000004;
		public const uint PFD_SUPPORT_OPENGL = 0x00000020;
		public const uint PFD_DOUBLEBUFFER = 0x00000001;
		public const byte PFD_MAIN_PLANE = 0;
		public const int HOST_ID = 0x00000002;
		public const int WM_DESTROY = 0x0002;
		public const int WM_NULL = 0x0000;
		public const int WM_CREATE = 0x0001;
		public const int WM_MOVE = 0x0003;
		public const int WM_SIZE = 0x0005;
		public const int WM_LBUTTONDOWN = 0x0201;
		public const int WM_RBUTTONDOWN = 0x204;
		public const int WM_LBUTTONUP = 0x0202;
		public const int WM_RBUTTONUP = 0x205;
		public const int WM_MOUSEMOVE = 0x0200;
		public const int WM_MOUSEWHEEL = 0x020A;
		public const int WM_CHAR = 0x0102;
		public const int WM_KEYUP = 0x0101;
		public const int WM_KEYDOWN = 0x0100;
		public const int WM_SYSKEYDOWN = 0x0104;
		public const int WM_SYSKEYUP = 0x0105;
		public const int WM_SYSCHAR = 0x0106;
		public const int WM_NCHITTEST = 0x0084;
		public const int WM_MBUTTONDOWN = 0x0207;
		public const int WM_MBUTTONUP = 0x0208;
		public const int WM_XBUTTONDOWN = 0x020B;
		public const int WM_XBUTTONUP = 0x020C;
		public const int WM_CAPTURECHANGED = 0x0215;
		public const int WM_MOUSELEAVE = 0x02A3;
		public const int WM_MOUSEHOVER = 0x02A1;
		public const int WM_NCMOUSEHOVER = 0x02A0;
		public const int WM_NCMOUSELEAVE = 0x02A2;
		public const int WH_MOUSE_LL = 14;
		public const int HTCLIENT = 1;
		[DllImport("user32.dll")]
		[return: MarshalAs(UnmanagedType.Bool)]
		static extern bool GetKeyboardState(byte[] lpKeyState);
		[DllImport("user32.dll", CharSet = CharSet.Unicode)]
		public static extern IntPtr SendMessage(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam);
		[DllImport("user32.dll")]
		public static extern IntPtr SetFocus(IntPtr hwnd);
		[DllImport("opengl32.dll")]
		public static extern IntPtr wglCreateContext(IntPtr hdc);
		[DllImport("opengl32.dll")]
		public static extern bool wglMakeCurrent(IntPtr hdc, IntPtr hglrc);
		[DllImport("opengl32.dll")]
		public static extern int wglChoosePixelFormat(IntPtr hdc, int[] piAttribIList, float[] pfAttribFList, uint nMaxFormats, int[] piFormats, out uint nNumFormats);
		[DllImport("opengl32.dll")]
		public static extern bool wglDeleteContext(IntPtr hglrc);
		[DllImport("user32.dll", SetLastError = true)]
		public static extern IntPtr LoadCursor(IntPtr hInstance, uint lpCursorName);
		[DllImport("user32.dll")]
		public static extern IntPtr RegisterClassEx([In] ref WNDCLASSEX lpwcx);
		[DllImport("user32.dll", SetLastError = true)]
		public static extern IntPtr RegisterClass([In] ref WNDCLASS lpWndClass); 
		[DllImport("user32.dll", SetLastError = true)]
		public static extern bool UnregisterClass(IntPtr lpClassName, IntPtr hInstance);
		[DllImport("user32.dll", SetLastError = true)]
		public static extern IntPtr DefWindowProc(IntPtr hWnd, int uMsg, IntPtr wParam, IntPtr lParam);
		[DllImport("user32.dll", SetLastError = true)]
		public static extern IntPtr SetWindowLong(IntPtr hWnd, int nIndex, uint dwNewLong);
		[DllImport("user32.dll", SetLastError = true)]
		public static extern bool SetWindowPos(
			IntPtr hWnd,
			IntPtr hWndInsertAfter,
			int X,
			int Y,
			int cx,
			int cy,
			uint uFlags);
		[DllImport("user32.dll", SetLastError = true)]
		public static extern void PostQuitMessage(int nExitCode);
		[DllImport("user32.dll")]
		static extern IntPtr SetParent(IntPtr hWndChild, IntPtr hWndNewParent);
		[StructLayout(LayoutKind.Sequential)]
		public struct WNDCLASS
		{
			public uint style;
			public WindowProc lpfnWndProc;
			public int cbClsExtra;
			public int cbWndExtra;
			public IntPtr hInstance;
			public IntPtr hIcon;
			public IntPtr hCursor;
			public IntPtr hbrBackground;
			public string lpszMenuName;
			public string lpszClassName;
		}
		[StructLayout(LayoutKind.Sequential)]
		public struct WNDCLASSEX
		{
			public uint cbSize;
			public uint style;
			public IntPtr lpfnWndProc;
			public int cbClsExtra;
			public int cbWndExtra;
			public IntPtr hInstance;
			public IntPtr hIcon;
			public IntPtr hCursor;
			public IntPtr hbrBackground;
			public string lpszMenuName;
			public string lpszClassName;
			public IntPtr hIconSm;
		}
		[DllImport("user32.dll", SetLastError = true)]
		public static extern bool GetClientRect(IntPtr hWnd, ref RECT lpRect);
		[StructLayout(LayoutKind.Sequential)]
		public struct RECT
		{
			public int left;
			public int top;
			public int right;
			public int bottom;
		}
		public const uint SWP_NOZORDER = 0x0004;
		public const uint SWP_NOMOVE = 0x0002;
		public const uint SWP_NOACTIVATE = 0x0010;
		[StructLayout(LayoutKind.Sequential)]
		public struct PIXELFORMATDESCRIPTOR
		{
			/// <summary>
			/// 
			/// </summary>
			/// <param name="colorBits">Specifies the number of color bitplanes in each color buffer. For RGBA pixel types, it is the size of the color buffer, excluding the alpha bitplanes. For color-index pixels, it is the size of the color-index buffer.</param>
			/// <param name="depthBits">Specifies the depth of the depth (z-axis) buffer.</param>
			/// <param name="stencilBits">Specifies the depth of the stencil buffer.</param>
			public void Init(byte colorBits, byte depthBits, byte stencilBits)
			{
				nSize = (ushort)Marshal.SizeOf(typeof(PIXELFORMATDESCRIPTOR));
				nVersion = 1;
				dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |	PFD_DOUBLEBUFFER;
				iPixelType = PFD_TYPE_RGBA;
				cColorBits = colorBits;
				cRedBits = cRedShift = cGreenBits = cGreenShift = cBlueBits = cBlueShift = 0;
				cAlphaBits = cAlphaShift = 0;
				cAccumBits = cAccumRedBits = cAccumGreenBits = cAccumBlueBits = cAccumAlphaBits = 0;
				cDepthBits = depthBits;
				cStencilBits = stencilBits;
				cAuxBuffers = 0;
				iLayerType = PFD_MAIN_PLANE;
				bReserved = 0;
				dwLayerMask = dwVisibleMask = dwDamageMask = 0;
			}
			ushort nSize;
			ushort nVersion;
			uint dwFlags;
			byte iPixelType;
			byte cColorBits;
			byte cRedBits;
			byte cRedShift;
			byte cGreenBits;
			byte cGreenShift;
			byte cBlueBits;
			byte cBlueShift;
			byte cAlphaBits;
			byte cAlphaShift;
			byte cAccumBits;
			byte cAccumRedBits;
			byte cAccumGreenBits;
			byte cAccumBlueBits;
			byte cAccumAlphaBits;
			byte cDepthBits;
			byte cStencilBits;
			byte cAuxBuffers;
			byte iLayerType;
			byte bReserved;
			uint dwLayerMask;
			uint dwVisibleMask;
			uint dwDamageMask;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct POINT
		{
			public int x;
			public int y;
		}

		[StructLayout(LayoutKind.Sequential)]
		public struct MSLLHOOKSTRUCT
		{
			public POINT pt;
			public uint mouseData;
			public uint flags;
			public uint time;
			public IntPtr dwExtraInfo;
		}
		public static IntPtr CustomWndProc(IntPtr hWnd, uint msg, IntPtr wParam, IntPtr lParam)
		{
			return IntPtr.Zero;
			//return DefWindowProc(hWnd, msg, wParam, lParam);
		}
	}
}
