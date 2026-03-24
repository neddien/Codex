using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;
using System.Runtime.Serialization.Formatters.Binary;
using System.Text;
using System.Threading.Tasks;

namespace NetCS
{
	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct NetPacketHeader<T> where T : struct
	{
		public T ID;
		public int Size;

		public int SizeOf { get { return Marshal.SizeOf(new NetPacketHeader<T>()); } }
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct NetPacket<T> where T : struct
	{
		public NetPacketHeader<T> Header = new NetPacketHeader<T>();
		public byte[] Data = null;

		public NetPacket() { }
		public NetPacket(T id) { this.Header.ID = id; }

		public NetPacket<T> AddBuffer(byte[] buffer)
		{
			if (this.Data == null)
				this.Data = buffer;
			else
			{
				int prevSize = this.Data.Length;
				Array.Resize(ref this.Data, prevSize + buffer.Length);
				Array.Copy(buffer, 0, this.Data, prevSize, buffer.Length);
			}
			this.Header.Size = this.Data.Length;
			return this;
		}

		public void Add<U>(U data) where U : struct
		{
			if (this.Data == null)
				this.Data = BitUtils.Serialize(data);
			else
			{
				byte[] buffer = BitUtils.Serialize(data);
				int prevSize = this.Data.Length;
				Array.Resize(ref this.Data, prevSize + buffer.Length);
				Array.Copy(buffer, 0, this.Data, prevSize, buffer.Length);
			}
			this.Header.Size = this.Data.Length;
		}

		public U Get<U>() where U : struct
		{
			int size = Marshal.SizeOf(typeof(U));

			if (this.Data == null)
				throw new NullReferenceException("No data has been appended to this packet.");
			else if (this.Data.Length < size)
				throw new ArgumentOutOfRangeException("Retrieving type is bigger than the internal buffer.");

			byte[] buffer = new byte[size];
			Array.Copy(this.Data, this.Data.Length - size, buffer, 0, size);
			Array.Resize(ref this.Data, this.Data.Length - size);
			return BitUtils.Deserialize<U>(buffer);
		}

		public byte[] GetBuffer(int length)
		{
			if (this.Data == null)
				throw new NullReferenceException("No data has been appended to this packet.");
			else if (this.Data.Length < length)
				throw new ArgumentOutOfRangeException("Retrieving buffer is bigger than the internal buffer.");

			byte[] buffer = new byte[length];
			int prevSize = this.Data.Length;
			Array.Copy(this.Data, prevSize - length, buffer, 0, length);
			Array.Resize(ref this.Data, prevSize - length);
			return buffer;
		}

		public void Resize(int newSize)
		{
			Array.Resize(ref this.Data, newSize);
		}
	}

	[StructLayout(LayoutKind.Sequential, Pack = 1)]
	public struct NetOwnedPacket<T> where T : struct
	{
		public NetConnection<T> Remote;
		public NetPacket<T> Packet;
	}
}
