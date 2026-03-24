using CodexEditor.CoreAPI;
using System;
using System.Threading;
using System.Runtime.InteropServices;
using System.Windows.Controls;
using System.Windows.Interop;
using CodexEngine;
using CodexEngine.Structs;
using CodexEditor.ViewModel.ECS;
using CodexEngine.Components;
using System.Windows;
using System.Windows.Input;
using System.Net;
using System.Diagnostics;

namespace CodexEditor.Util
{
    public class EngineHost : HwndHost
    {
        private static EngineHost _instance;

        private readonly Border _parent;
        private IntPtr _hwnd;
        private HandleRef _engineHandleRef;
        private HandleRef _parentHandleRef;
        private Thread _renderThread;
        private volatile bool _running;
		private Point _previousMousePosition;
		private bool _isFirstUpdate = true;
		private int _renderWidth;
        private int _renderHeight;
        private Point _mousePos;
		private IntPtr _hook = IntPtr.Zero;
		private Win32.LowLevelMouseProc _mouseProc;
		private bool _isMouseInWindow;

		public event EventHandler<MouseEventArgs> MouseMove;
        public event EventHandler<MouseEventArgs> MouseEnter;
        public event EventHandler<MouseEventArgs> MouseLeave;
		public event EventHandler<MouseButtonEventArgs> MouseUp;
		public event EventHandler<MouseButtonEventArgs> MouseDown;
		public event EventHandler<KeyEventArgs> KeyDown;
		public event EventHandler<KeyEventArgs> KeyUp;

        //public static EngineHost Instance { get; private set; }

        private bool IsMouseInWindow
        {
            set
            {
                _isMouseInWindow = value;
                if (value) Win32.SetFocus(_hwnd);
                else Win32.SetFocus(IntPtr.Zero);
            }
            get => _isMouseInWindow;
        }

        public EngineHost(Border parent)
        {
            _instance = this;
            _parent = parent;
            _hwnd = IntPtr.Zero;
            _running = true;
            _renderWidth = (int)parent.ActualWidth;
            _renderHeight = (int)parent.ActualHeight;
            _renderThread = null;
            _mousePos = new Point(0, 0);
            _mouseProc = HookCallback;
            //_hook = SetMouseHook(_mouseProc);
        }

        public static void SignalTermination()
        {
            _instance.DestroyWindowCore(_instance._engineHandleRef);
        }

		private static IntPtr SetMouseHook(Win32.LowLevelMouseProc proc)
		{
			using (Process currentProcess = Process.GetCurrentProcess())
			using (ProcessModule currentModule = currentProcess.MainModule)
			{
                return Win32.SetWindowsHookEx(Win32.WH_MOUSE_LL, proc, Win32.GetModuleHandle(currentModule.ModuleName), 0);
			}
		}

		private void EngineThread(object hwndParent)
        {
            CodexAPI.Init();
            var props = new WindowProperties()
            {
                Width = _renderWidth,
                Height = _renderHeight,
                FrameCap = 0,
                Vsync = true,
                PosX = 0,
                PosY = 0,
            };
            CodexAPI.CreateWindow(ref props, _hwnd);

            while (_running)
            {
                // Check for commands
                if (EngineAPI.CommandStack.TryPop(out var command))
                {
                    switch (command.Type)
                    {
                        case EngineCommandType.CreateEntity:
                            {
                                var desc = (EntityDescriptor)command.Args[0];
                                var returnValue = CodexAPI.CreateEntity(desc);
                                command.ReturnValue = returnValue;
                                break;
                            }
                        case EngineCommandType.RemoveEntity:
                            {
                                CodexAPI.RemoveEntity((int)command.Args[0]);
                                break;
                            }
                        case EngineCommandType.UpdateEntityFromDescriptor:
                            {
                                var desc = (EntityDescriptor)command.Args[1];
                                CodexAPI.UpdateEntityFromDescriptor((int)command.Args[0], desc);
                                break;
                            }

                        // Component creators
                        case EngineCommandType.AddComponentToEntity:
                            {
                                if (command.Args[1] is SpriteRendererComponent spriteRenderer)
                                {
                                    var desc = spriteRenderer.ToDescriptor();
                                    CodexAPI.AddSpriteRendererComponent((int)command.Args[0], desc);
                                }
                                else if (command.Args[1] is TilemapComponent tilemapComponent)
                                {
                                    TileInfo[] tiles = null;
                                    CodexAPI.AddTilemapComponent((int)command.Args[0], tilemapComponent.ToDescriptor(ref tiles), tiles);
                                }
                                break;
                            }

                        // Component updates
                        case EngineCommandType.UpdateTransformComponent:
                            {
                                CodexAPI.UpdateTransformComponent((int)command.Args[0], (DescriptorTransformComponent)command.Args[1]);
                                break;
                            }
                        case EngineCommandType.UpdateTagComponent:
                            {
                                CodexAPI.UpdateTagComponent((int)command.Args[0], (DescriptorTagComponent)command.Args[1]);
                                break;
                            }
                        case EngineCommandType.UpdateSpriteRendererComponent:
                            {
                                CodexAPI.UpdateSpriteRendererComponent((int)command.Args[0], (DescriptorSpriteRendererComponent)command.Args[1]);
                                break;
                            }
                        case EngineCommandType.UpdateTilemapComponent:
                            {
								CodexAPI.UpdateTilemapComponent(
                                    (int)command.Args[0], 
                                    (DescriptorTilemapComponent)command.Args[1],
                                    (TileInfo[])command.Args[2]);
                                break;
                            }
                    }
                    command.SignalCompletion();
                }

                // Engine Update
                HandleResize();
                CodexAPI.Update();
            }
            CodexAPI.Destroy();
        }

        private void HandleResize()
        {
            int newWidth = (int)Math.Ceiling(ActualWidth);
            int newHeight = (int)Math.Ceiling(ActualHeight);

            if (_renderWidth != newWidth || _renderHeight != newHeight)
            {
                _renderWidth = newWidth;
                _renderHeight = newHeight;
                CodexAPI.ResizeViewport(_renderWidth, _renderHeight);

                /*if (!resize(renderWidth, renderHeight))
                    throw new Exception(GetDllError());*/
            }
        }

        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
			_hwnd = Win32.CreateWindowEx(
                0,
                "static",
                "",
                Win32.WS_CHILD | Win32.WS_VISIBLE,
                0,
                0,
                (uint)_renderWidth,
                (uint)_renderHeight,
                hwndParent.Handle,
                (IntPtr)Win32.HOST_ID,
                IntPtr.Zero,
                IntPtr.Zero
            );

            _parentHandleRef = hwndParent;
            _engineHandleRef = new HandleRef(this, _hwnd);
            Win32.SetFocus(_hwnd);

            _renderThread = new Thread(new ParameterizedThreadStart(EngineThread));
            _renderThread.Start(hwndParent.Handle);

            return new HandleRef(this, _hwnd);
        }

		private IntPtr HookCallback(int nCode, IntPtr wParam, IntPtr lParam)
		{
            bool _toLoseFocus = false;
            if (nCode >= 0 && _toLoseFocus)
			{
				Win32.MSLLHOOKSTRUCT hookStruct =
				(Win32.MSLLHOOKSTRUCT)Marshal.PtrToStructure(lParam, typeof(Win32.MSLLHOOKSTRUCT));

				switch ((int)wParam)
				{
					case Win32.WM_MOUSEMOVE:
						{
							Win32.SendMessage(_hwnd, Win32.WM_MOUSEMOVE, IntPtr.Zero, IntPtr.Zero);
							Console.WriteLine("to lose focus mode");
							break;
						}
					case Win32.WM_LBUTTONDOWN:
						{
							break;
						}
					case Win32.WM_LBUTTONUP:
						{
							_toLoseFocus = false;
                            //_isMouseDown = false;
							break;
						}
					case Win32.WM_RBUTTONDOWN:
						{
							break;
						}
					case Win32.WM_RBUTTONUP:
						{
							_toLoseFocus = false;
							//_isMouseDown = false;
							break;
						}
				}
			}

			return Win32.CallNextHookEx(_hook, nCode, wParam, lParam);
		}

		public System.Drawing.PointF GetMouseDelta()
		{
			if (_isFirstUpdate)
			{
				_isFirstUpdate = false;
				_previousMousePosition = _mousePos;
				return System.Drawing.PointF.Empty;
			}

			float deltaX = (float)(_mousePos.X - _previousMousePosition.X);
			float deltaY = (float)(_mousePos.Y - _previousMousePosition.Y);

            _previousMousePosition = _mousePos;

			return new System.Drawing.PointF(deltaX, deltaY);
		}

        protected override void OnMouseMove(MouseEventArgs e)
        {
            base.OnMouseMove(e);
            MouseMove?.Invoke(this, e);
		}

		protected override void OnMouseDown(MouseButtonEventArgs e)
		{
			base.OnMouseDown(e);
            MouseDown?.Invoke(this, e);
		}

		protected override void OnMouseUp(MouseButtonEventArgs e)
		{
			base.OnMouseUp(e);
			MouseUp?.Invoke(this, e);
		}

		protected override void OnKeyDown(KeyEventArgs e)
		{
			base.OnKeyDown(e);
			KeyDown?.Invoke(this, e);
		}

        protected override void OnKeyUp(KeyEventArgs e)
        {
            base.OnKeyUp(e);
            KeyUp?.Invoke(this, e);
        }

		protected override void OnMouseEnter(MouseEventArgs e)
		{
			base.OnMouseEnter(e);
            MouseEnter?.Invoke(this, e);
		}

		protected override void OnMouseLeave(MouseEventArgs e)
		{
			base.OnMouseLeave(e);
            MouseLeave?.Invoke(this, e);
		}

		protected override IntPtr WndProc(IntPtr hwnd, int msg, IntPtr wParam, IntPtr lParam, ref bool handled)
		{
			switch (msg)
            {
                case Win32.WM_KEYDOWN:
                    {
                        handled = !IsMouseInWindow;
                        if (handled) Win32.SendMessage(_hwnd, msg, wParam, lParam);

						OnKeyDown(new KeyEventArgs(
								Keyboard.PrimaryDevice,
								Keyboard.PrimaryDevice.ActiveSource,
								0,
								(Key)wParam.ToInt32()));
						break;
                    }
                case Win32.WM_KEYUP:
					{
                        handled = !IsMouseInWindow;
						if (handled) Win32.SendMessage(_hwnd, msg, wParam, lParam);

						OnKeyUp(new KeyEventArgs(
								Keyboard.PrimaryDevice,
								Keyboard.PrimaryDevice.ActiveSource,
								0,
								(Key)wParam.ToInt32()));
						break;
                    }
                case Win32.WM_NCHITTEST:
                    {
                        handled = true;
                        IsMouseInWindow = true;
						//Win32.SendMessage(_hwnd, msg, wParam, lParam);
                        _mousePos = new Point((int)lParam & 0xFFFF, ((int)lParam >> 16) & 0xFFFF);
                        Win32.ScreenToClient(_hwnd, ref _mousePos);
						OnMouseMove(new MouseEventArgs(Mouse.PrimaryDevice, 0));
						return (IntPtr)(Win32.HTCLIENT);
                    }
                case Win32.WM_MOUSELEAVE:
                    {
                        IsMouseInWindow = false;
                        break;
                    }
                case Win32.WM_LBUTTONDOWN:
                    {
                        OnMouseDown(new MouseButtonEventArgs(Mouse.PrimaryDevice, 0, MouseButton.Left));
						handled = false;
						break;
					}
				case Win32.WM_LBUTTONUP:
					{
						OnMouseUp(new MouseButtonEventArgs(Mouse.PrimaryDevice, 0, MouseButton.Left));
						handled = false;
						break;
					}
				case Win32.WM_RBUTTONDOWN:
					{
						OnMouseDown(new MouseButtonEventArgs(Mouse.PrimaryDevice, 0, MouseButton.Right));
						handled = false;
						break;
					}
				case Win32.WM_RBUTTONUP:
					{
						OnMouseUp(new MouseButtonEventArgs(Mouse.PrimaryDevice, 0, MouseButton.Right));
						handled = false;
						break;
					}
				default:
                    handled = false;
                    break;
			}
			return IntPtr.Zero;
        }

		protected override void DestroyWindowCore(HandleRef hwnd)
        {
            _running = false;
            _renderThread.Join();
            Win32.DestroyWindow(hwnd.Handle);
        }
    }
}
