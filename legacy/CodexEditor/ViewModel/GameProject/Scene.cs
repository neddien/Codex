using CodexEditor.Util;
using CodexEditor.ViewModel.ECS;
using System;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Runtime.Serialization;
using System.Windows.Input;

namespace CodexEditor.ViewModel.GameProject
{
	[DataContract(IsReference = true)]
	public class Scene : ViewModelBase, IDisposable
	{
		private string _name;
		[DataMember]
		public string Name
		{
			get => _name;
			set
			{
				if (_name != value)
				{
					_name = value;
					OnPropertyChanged();
				}
			}
		}
		[DataMember]
		public Project Project { get; set; }

		[DataMember(Name = "IsActive")]
		private bool _isActive = true;
		public bool IsActive
		{
			get => _isActive;
			set
			{
				if (_isActive != value)
				{
					_isActive = value;
					OnPropertyChanged();
				}
			}
		}

		[DataMember(Name = "Entities")]
		private ObservableCollection<Entity> _entities = new ObservableCollection<Entity>();

		public ReadOnlyObservableCollection<Entity> Entities { get; private set; }

		public ICommand AddEntityCmd { get; private set; }
		public ICommand RemoveEntityCmd { get; private set; }

		public Scene(Project project, string name)
		{
			Debug.Assert(project != null);
			Project = project;
			Name = name;
			OnDeserialized(new StreamingContext());
		}

		private void AddEntity(Entity entity, int index = -1)
		{
			Debug.Assert(entity != null && !_entities.Contains(entity));
			entity.IsActive = IsActive;
			if (index == -1)
				_entities.Add(entity);
			else
			{
			}
		}

		private void RemoveEntity(Entity entity)
		{
			Debug.Assert(entity != null && _entities.Contains(entity));
			entity.IsActive = false;
			_entities.Remove(entity);
		}

		[OnDeserialized]
		private void OnDeserialized(StreamingContext ctx)
		{
			if (_entities != null)
			{
				Entities = new ReadOnlyObservableCollection<Entity>(_entities);
				OnPropertyChanged();
			}

			foreach (var entity in _entities)
				entity.IsActive = IsActive;

			AddEntityCmd = new RelayCommand<Entity>(x =>
			{
				AddEntity(x);
				var entityIndex = _entities.Count - 1;
				Project.Unredoable.Add(new UnredoableAction(
					() => RemoveEntity(x),
					() => AddEntity(x, entityIndex),
					$"Add {x.Name} in {Name}"
					));
			});

			RemoveEntityCmd = new RelayCommand<Entity>(x =>
			{
				RemoveEntity(x);
				var entityIndex = _entities.Count - 1; // handle -1
				if (entityIndex < 0) return;
				Project.Unredoable.Add(new UnredoableAction(
					() => AddEntity(x, entityIndex),
					() => RemoveEntity(x),
					$"Removed {x.Name} in {Name}"
					));

			});
		}

		#region IDisposable stuff
		private bool disposedValue;
		protected virtual void Dispose(bool disposing)
		{
			if (!disposedValue)
			{
				if (disposing)
				{
					// TODO: dispose managed state (managed objects)
				}

				// TODO: free unmanaged resources (unmanaged objects) and override finalizer
				// TODO: set large fields to null
				foreach (var entity in _entities)
					entity.Dispose();
				disposedValue = true;
			}
		}

		// // TODO: override finalizer only if 'Dispose(bool disposing)' has code to free unmanaged resources
		// ~Scene()
		// {
		//     // Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
		//     Dispose(disposing: false);
		// }

		public void Dispose()
		{
			// Do not change this code. Put cleanup code in 'Dispose(bool disposing)' method
			Dispose(disposing: true);
			GC.SuppressFinalize(this);
		}
		#endregion
	}
}
