using System.Net;
using System.Net.Sockets;
using System.Threading.Tasks;

namespace NetCS
{
	public class NetClient<T> where T : struct
	{
		private Socket _socket;
		private IPEndPoint _endPoint;
		private UInt64 _id;
		private NetQueue<NetOwnedPacket<T>> _inQueue;
		private NetConnection<T> _server;

		public NetClient()
		{
			_socket = new Socket(AddressFamily.InterNetwork, SocketType.Stream, ProtocolType.Tcp);
			_id = 0;
			_inQueue = new NetQueue<NetOwnedPacket<T>>();
		}

		public bool IsConnected { get { return _server.IsConnected; } }

		public void Connect(string host, ushort port)
		{
			try
			{
				IPAddress testIp;
				if (!IPAddress.TryParse(host, out testIp))
				{
					var address = Dns.GetHostEntry(IPAddress.Parse(host)).AddressList.FirstOrDefault();
					_endPoint = new IPEndPoint(address, port);
				}
				else _endPoint = new IPEndPoint(testIp, port);
				this.Connect(_endPoint);
			}
			catch (Exception ex)
			{
				Console.WriteLine(ex.ToString());
			}
		}

		public void Connect(IPEndPoint ep)
		{
			try
			{
				_socket.Connect(ep);
				this.OnConnected();
				_server = new NetConnection<T>(NetConnection<T>.NetOwnerType.Client, _id, _socket, _inQueue);
				_server.HandleNetServer(this);
			}
			catch (Exception ex)
			{
				Console.WriteLine(ex.ToString());
			}
		}

		public bool GetIsValidated()
		{
			return _server != null && _server.IsValidated;
		}

		public void Send(NetPacket<T> packet)
		{
			_server.Send(packet);
		}

		public void Update()
		{
			if (!_server.IsConnected)
				this.OnDisconnected();

			while (!_inQueue.IsEmpty())
			{
				var ownedPacket = _inQueue.PopFront();
				this.OnReceive(ownedPacket.Packet);
			}
		}

		protected virtual void OnConnected()
		{

		}

		public virtual void OnValidated()
		{

		}

		protected virtual void OnReceive(NetPacket<T> packet)
		{

		}

		protected virtual void OnDisconnected()
		{

		}
	}
}
