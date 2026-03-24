using System.Reflection.Emit;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace NetCS
{
	public static class BitUtils
	{
		private static DynamicMethod _SizeOfDynamicMethod = new DynamicMethod("SizeOfType", typeof(int), new Type[] { });
		private static ILGenerator _SizeOfDynamicMethodIL = null;

		public static int SizeOf<T>()
		{
			if (_SizeOfDynamicMethodIL == null)
			{
				_SizeOfDynamicMethodIL = _SizeOfDynamicMethod.GetILGenerator();
				_SizeOfDynamicMethodIL.Emit(OpCodes.Sizeof, typeof(T));
				_SizeOfDynamicMethodIL.Emit(OpCodes.Ret);
			}
			return (int)(_SizeOfDynamicMethod.Invoke(null, null));
		}

		public static void PrintExln(Exception ex) => Console.WriteLine(ex.ToString());

		public static byte[] Serialize<T>(T data) where T : struct
		{
			int size = Marshal.SizeOf(data);
			byte[] buffer = new byte[size];
			var handle = GCHandle.Alloc(data, GCHandleType.Pinned);
			Marshal.Copy(handle.AddrOfPinnedObject(), buffer, 0, size);
			handle.Free();
			return buffer;
		}

		public static T Deserialize<T>(byte[] buffer) where T : struct
		{
			T data = default(T);
			int size = Marshal.SizeOf(data);

			if (size > buffer.Length)
			{
				throw new ArgumentException($"Passed buffer is smaller than the size of T, where T is {data.GetType()}.");
			}

			unsafe
			{
				var handle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
				try
				{
					Unsafe.CopyBlock(Unsafe.AsPointer(ref data), (void*)(handle.AddrOfPinnedObject()), (uint)(size));
				}
				finally
				{
					handle.Free();
				}
			}

			return data;
		}
	}
}
