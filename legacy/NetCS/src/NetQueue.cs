namespace NetCS
{
	public class NetQueue<T>
	{
		protected Mutex _mutex = new Mutex();
		protected LinkedList<T> _queue = new LinkedList<T>();

		public T Front()
		{
			this._mutex.WaitOne();
			T ret = this._queue.First();
			this._mutex.ReleaseMutex();
			return ret;
		}
		public T Back()
		{
			this._mutex.WaitOne();
			T ret = this._queue.Last();
			this._mutex.ReleaseMutex();
			return ret;
		}
		public void Push(T item)
		{
			this._mutex.WaitOne();
			this._queue.AddLast(item);
			this._mutex.ReleaseMutex();
		}
		public void PushFront(T item)
		{
			this._mutex.WaitOne();
			this._queue.AddFirst(item);
			this._mutex.ReleaseMutex();
		}
		public T Pop()
		{
			this._mutex.WaitOne();
			T ret = this._queue.Last();
			this._queue.RemoveLast();
			this._mutex.ReleaseMutex();
			return ret;
		}
		public T PopFront()
		{
			this._mutex.WaitOne();
			T ret = this._queue.First();
			this._queue.RemoveFirst();
			this._mutex.ReleaseMutex();
			return ret;
		}
		public bool IsEmpty()
		{
			this._mutex.WaitOne();
			bool isEmpty = this._queue.Count <= 0;
			this._mutex.ReleaseMutex();
			return isEmpty;
		}
		public void Clear()
		{
			this._mutex.WaitOne();
			this._queue.Clear();
			this._mutex.ReleaseMutex();
		}
	}
}
