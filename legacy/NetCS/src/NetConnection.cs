using System;
using System.Net;
using System.Net.Sockets;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Threading.Tasks;

namespace NetCS
{
	public class NetConnection<T> where T : struct
	{
		public enum NetOwnerType
		{ 
			Server,
			Client
		}

		private NetOwnerType _ownerType;
		private UInt64 _id;
		private Socket _socket;
		private NetQueue<NetOwnedPacket<T>> _inQueue;
		private NetQueue<NetPacket<T>> _outQueue;
		private NetPacket<T> _tempPacket;
		private int _packetHeaderSize;
		private UInt64 _handshakeOut;
		private UInt64 _handshakeIn;
		private bool _validated;

		public NetConnection(NetOwnerType type, UInt64 id, Socket socket, NetQueue<NetOwnedPacket<T>> inQueue)
		{
			_ownerType = type;
			_id = id;
			_socket = socket;
			_inQueue = inQueue;
			_outQueue = new NetQueue<NetPacket<T>>();
			_tempPacket = new NetPacket<T>();
			_packetHeaderSize = Marshal.SizeOf(_tempPacket.Header);
			_validated = false;

			if (_ownerType == NetOwnerType.Server)
			{
				var rnd = new Random();
				byte[] buffer = new byte[sizeof(UInt64)];
				rnd.NextBytes(buffer);
				_handshakeOut = BitConverter.ToUInt64(buffer);
			}
			else
			{
				_handshakeOut = 0;
				_handshakeIn = 0;
			}
		}

		public bool IsValidated		{ get { return _validated; } }
		public bool IsConnected		{ get { return _socket.Connected; } }
		public UInt64 GetID			{ get { return _id; } }

		public void HandleNetClient(NetServer<T> server = null)
		{
			if (_ownerType == NetOwnerType.Server)
			{
				_ = this.SendValidationAsync();
				_ = this.ReadValidationAsync(server);
			}
		}

		public void HandleNetServer(NetClient<T> client = null)
		{
			if (_ownerType == NetOwnerType.Client)
			{
				_ = this.ReadValidationAsync(null, client);
			}
		}

		public async Task ReadHeaderAsync()
		{
			while (this.IsConnected)
			{
				Console.WriteLine("Waiting for a header...");
				byte[] buffer = new byte[_packetHeaderSize];
				var task = _socket.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
				_ = await task;

				if (task.Exception == null)
				{
					_tempPacket.Header = BitUtils.Deserialize<NetPacketHeader<T>>(buffer);
					if (_tempPacket.Header.Size > 0)
					{
						_tempPacket.Resize(_tempPacket.Header.Size);
						_ = this.ReadDataAsync();
					}
					else
						this.AddPacket();
				}
				else
				{
					Console.WriteLine(task.Exception.ToString());
				}
			}
		}

		public async Task SendValidationAsync(NetClient<T> client = null)
		{
			byte[] buffer = BitConverter.GetBytes(_handshakeOut);
			var task = _socket.SendAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
			_ = await task;

			if (task.Exception == null)
			{
				if (_ownerType == NetOwnerType.Client)
					_ = this.ConfirmValidationAsync(client);
			}
			else
			{
				Console.WriteLine(task.Exception.ToString());
				this.Disconnect();
			}
		}

		public async Task ReadValidationAsync(NetServer<T> server = null, NetClient<T> client = null)
		{
			byte[] buffer = new byte[sizeof(UInt64)];
			var task = _socket.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
			_ = await task;

			if (task.Exception == null)
			{
				_handshakeIn = BitConverter.ToUInt64(buffer);
				if (_ownerType == NetOwnerType.Server)
				{
					UInt64 handshakeToCompareTo = this.Randgen(_handshakeOut);
					if (_handshakeIn == handshakeToCompareTo)
					{
						Console.WriteLine("Client validated.");
						_validated = true;
						_ = this.ConfirmValidationAsync(null, server);
					}
					else
					{
						_ = this.ConfirmValidationAsync(client);
						Console.WriteLine("Client failed validation.");
						Console.WriteLine($"Expected handshake: {handshakeToCompareTo} but got: {_handshakeIn}");
						this.Disconnect();
					}
				}
				else
				{
					_handshakeOut = this.Randgen(_handshakeIn);
					_ = this.SendValidationAsync(client);
				}

			}
			else
			{
				Console.WriteLine(task.Exception.ToString());
				this.Disconnect();
			}
		}

		public async Task ConfirmValidationAsync(NetClient<T> client = null, NetServer<T> server = null)
		{
			if (_ownerType == NetOwnerType.Server)
			{
				byte[] buffer = new byte[1];
				buffer[0] = (byte)((_validated) ? 1 : 0);
				var task = _socket.SendAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
				_ = await task;

				if (task.Exception == null)
				{
					if (_validated)
					{
						_ = this.ReadHeaderAsync();
						if (server != null) server.OnClientValidated(this);
					}
					else
						this.Disconnect();
				}
				else
				{
					Console.WriteLine(task.Exception.ToString());
					this.Disconnect();
				}
			}
			else
			{
				byte[] buffer = new byte[1];
				var task = _socket.ReceiveAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
				_ = await task;

				if (task.Exception == null)
				{
					if (buffer[0] == 1)
					{
						_validated = true;
						if (client != null) client.OnValidated();
						_ = this.ReadHeaderAsync();
					}
					else
					{
						_validated = false;
						this.Disconnect();
					}
				}
				else
				{
					Console.WriteLine(task.Exception.ToString());
					this.Disconnect();
				}
			}
		}

		public async Task ReadDataAsync()
		{
			var task = _socket.ReceiveAsync(new ArraySegment<byte>(_tempPacket.Data), SocketFlags.None);
			_ = await task;

			if (task.Exception == null)
				this.AddPacket();
			else
			{
				Console.WriteLine(task.Exception.ToString());
			}
		}

		public async Task SendHeaderAsync()
		{
			// TODO: This will cause stack overflow, consider using an iterative approach instead.
			var packet = _outQueue.Front();
			byte[] buffer = BitUtils.Serialize(packet.Header);
			var task = _socket.SendAsync(new ArraySegment<byte>(buffer), SocketFlags.None);
			_ = await task;

			if (task.Exception == null)
			{
				if (packet.Header.Size > 0)
				{
					_ = this.SendDataAsync();
				}
				else
				{
					if (!_outQueue.IsEmpty())
						_ = this.SendHeaderAsync();
				}
			}
			else
			{
				Console.WriteLine(task.Exception.ToString());
				this.Disconnect();
			}
		}

		public async Task SendDataAsync()
		{
			// TODO: This will cause stack overflow, consider using an iterative approach instead.
			var packet = _outQueue.Front();
			var task = _socket.SendAsync(new ArraySegment<byte>(packet.Data), SocketFlags.None);
			_ = await task;

			if (task.Exception == null)
			{
				_outQueue.PopFront();
				if (!_outQueue.IsEmpty())
					_ = this.SendHeaderAsync();
			}
			else
			{
				Console.WriteLine(task.Exception.ToString());
			}
		}

		public void AddPacket()
		{
			if (_ownerType == NetOwnerType.Server)
				_inQueue.Push(new NetOwnedPacket<T>() { Remote = this, Packet = _tempPacket });
			else
				_inQueue.Push(new NetOwnedPacket<T>() { Remote = null, Packet = _tempPacket });
		}

		public void Send(NetPacket<T> packet)
		{
			if (this.IsConnected && this.IsValidated)
			{
				bool isEmpty = _outQueue.IsEmpty();
				_outQueue.PushFront(packet);
				if (isEmpty)
					_ = this.SendHeaderAsync();
			}
		}

		public void Disconnect()
		{
			if (_socket != null && _socket.Connected)
			{
				_socket.Shutdown(SocketShutdown.Both);
				_socket.Close();
			}
		}

		UInt64 Randgen(UInt64 input)
		{
			return (~input << 8) ^ 0xFEEDFAAB00000001 ^ 0xFEEFAAF000000001;
		}
	}
}
