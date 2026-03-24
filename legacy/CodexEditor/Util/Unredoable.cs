using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Reflection;

namespace CodexEditor.Util
{
    public interface IUnredoable
    {
        string Name { get; }
        void Undo();
        void Redo();
    }

    public class UnredoableAction : IUnredoable
    {
        private Action _undoAction;
        private Action _redoAction;

        public string Name { get; }

        public void Undo() => _undoAction();
        public void Redo() => _redoAction();

        public UnredoableAction(string name)
        {
            Name = name;
        }

        public UnredoableAction(Action undoAction, Action redoAction, string name)
        {
            Debug.Assert(undoAction != null && redoAction != null);
            _undoAction = undoAction;
            _redoAction = redoAction;
            Name = name;
        }

        public UnredoableAction(string propertyName, object instance, object undoValue, object redoValue, string name)
		    : this
		    	(
		    		() => instance.GetType().GetProperty(propertyName).SetValue(instance, undoValue),
		    		() => instance.GetType().GetProperty(propertyName).SetValue(instance, redoValue),
		    		name
		    	)
        {
            
        }
    }

    public class Unredoable
    {
        private bool _addEnabled = true;
        private readonly ObservableCollection<IUnredoable> _redoList = new ObservableCollection<IUnredoable>();
        private readonly ObservableCollection<IUnredoable> _undoList = new ObservableCollection<IUnredoable>();
        public ReadOnlyObservableCollection<IUnredoable> RedoList { get; }
        public ReadOnlyObservableCollection<IUnredoable> UndoList { get; }

        public Unredoable()
        {
            RedoList = new ReadOnlyObservableCollection<IUnredoable>(_redoList);
            UndoList = new ReadOnlyObservableCollection<IUnredoable>(_undoList);
        }

        public void Add(IUnredoable cmd)
        {
            if (_addEnabled)
			{
				_undoList.Add(cmd);
				_redoList.Clear();
			}
        }

        public void Undo()
        {
            if (_undoList.Any())
            {
                var cmd = _undoList.Last();
                _undoList.RemoveAt(_undoList.Count - 1);
                _addEnabled = false;
                cmd.Undo();
                _addEnabled = true;
                _redoList.Insert(0, cmd);
            }
        }

        public void Redo()
        {
            if (_redoList.Any())
            {
                var cmd = _redoList.First();
                _redoList.RemoveAt(0);
                _addEnabled = false;
                cmd.Redo();
                _addEnabled = true;
                _undoList.Add(cmd);
            }
        }

        public void Reset()
        {
            _redoList.Clear();
            _undoList.Clear();
        }
    }
}
