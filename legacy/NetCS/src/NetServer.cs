using System.Net;
using System.Net.Sockets;

namespace NetCS
{
	public class NetServer<T> where T : struct
	{
		private ushort _port;
		private UInt64 _idCount;
		private Socket _socket;
		private List<NetConnection<T>> _clients;
		private NetQueue<NetOwnedPacket<T>> _inQueue;
		private NetQueue<NetPacket<T>> _outQueue;

		public NetServer(ushort port) 
		{
			_port = port;
			_idCount = 0;
			_socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			_clients = new List<NetConnection<T>>();
			_inQueue = new NetQueue<NetOwnedPacket<T>>();
			_outQueue = new NetQueue<NetPacket<T>>();
		}

		public async Task Start()
		{
			try
			{
				_socket.Bind(new IPEndPoint(IPAddress.Any, _port));
				_socket.Listen(10);
				Console.WriteLine("Server started.");

				while (true)
				{
					var newSocket = await _socket.AcceptAsync();
					Console.WriteLine($"Client {_idCount} connected.");

					var conn = new NetConnection<T>(NetConnection<T>.NetOwnerType.Server, _idCount++, newSocket, _inQueue);

					if (this.OnClientConnected(conn))
					{
						_clients.Add(conn);
						conn.HandleNetClient(this);
					}
					else
					{
						Console.WriteLine("Connection refused.");
						newSocket.Close();
					}
				}
			}
			catch (Exception ex)
			{
				Console.WriteLine(ex);
			}
		}

		public void Send(NetConnection<T> client, NetPacket<T> packet)
		{
			if (client.IsConnected)
				client.Send(packet);
		}

		public void Update()
		{
			for (int i = 0; i < _clients.Count; ++i)
				if (!_clients[i].IsConnected)
				{
					this.OnClientDisconnected(_clients[i]);
					_clients.RemoveAt(i);
				}

			while (!_inQueue.IsEmpty())
			{
				var ownedPacket = _inQueue.PopFront();
				this.OnReceive(ownedPacket.Remote, ownedPacket.Packet);
			}
		}

		protected virtual bool OnClientConnected(NetConnection<T> client)
		{
			return false;
		}

		public virtual void OnClientValidated(NetConnection<T> client)
		{

		}

		protected virtual void OnClientDisconnected(NetConnection<T> client)
		{

		}

		protected virtual void OnReceive(NetConnection<T> client, NetPacket<T> packet)
		{ 
			
		}
	}
}
