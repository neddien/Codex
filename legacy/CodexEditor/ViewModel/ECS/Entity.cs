using CodexEditor.CoreAPI;
using CodexEditor.Util;
using CodexEditor.ViewModel.GameProject;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Runtime.Serialization;
using CodexEngine;

namespace CodexEditor.ViewModel.ECS
{
    [DataContract(IsReference = true)]
	[KnownType(typeof(TransformComponent))]
	[KnownType(typeof(TagComponent))]
	[KnownType(typeof(SpriteRendererComponent))]
	[KnownType(typeof(TilemapComponent))]
	public class Entity : ViewModelBase, IDisposable
    {
        // TODO: Change INVALID_ENTITY_ID from 0 to -1 because 0 is an actual valid ENTT entity ID.
        public const int INVALID_ENTITY_ID = -1;

        private int _entityID = INVALID_ENTITY_ID;
        public int EntityID
        {
            get => _entityID;
            set
            {
                if (_entityID != value)
                {
                    _entityID = value;
                    OnPropertyChanged();
                }
            }
        }

        private bool _isActive;
		public bool IsActive
        {
            get => _isActive;
            set
            {
                if (_isActive != value)
                {
                    _isActive = value;
                    if (_isActive)
					{
						EntityID = EngineAPI.CreateEntity(this);
                        Logger.Log(MessageType.Info, $"Created entt id: {EntityID}");
                        Logger.Log(MessageType.Info, $"Appending {_components.Count - 2} component(s)...");

                        // 
                        foreach (var c in _components)
						{
							var type = c.GetType();

							// we've got motherfucking python here
							if (c is not TransformComponent && c is not TagComponent)
							{
								var method = typeof(EngineAPI).GetMethod("AddComponentToEntity");
								var genericMethod = method.MakeGenericMethod(type);
								genericMethod.Invoke(null, new object[] { this, Convert.ChangeType(c, type) });
							}
                            else
                            {
                                // Since Transform and Tag components are automatically added we do not need to re-add them, instead we just update them.
                                var method = typeof(EngineAPI).GetMethod("UpdateEntityComponent");
                                var genericMethod = method.MakeGenericMethod(type);
								genericMethod.Invoke(null, new object[] { this, Convert.ChangeType(c, type) });
							}
						}
					}
                    else if (EntityID != INVALID_ENTITY_ID)
					{
						Logger.Log(MessageType.Info, $"Destroyed entt id: {EntityID}");
						EngineAPI.RemoveEntity(this);
                        EntityID = INVALID_ENTITY_ID;
					}
                }
            }
        }

        private string _name;
		[DataMember(Order = 4)]
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
        private bool _enabled;
		[DataMember(Order = 5)]
		public bool Enabled
        {
            get => _enabled;
            set 
            {
                if (value != _enabled)
                {
                    _enabled = value;
                    OnPropertyChanged();
                }
            }
        }
        private bool _visible;
		[DataMember(Order = 6)]
		public bool Visible
		{
			// TODO: do the undo redo and multi selection thing with Visible property as well
			// should be same as Enabled property
			get => _visible;
            set
            {
                if (value != _visible)
                {
                    _visible = value;
                    OnPropertyChanged();
                }
            }
		}

		[DataMember(Order = 1)]
		public Scene ParentScene { get; set; }

		[DataMember(Name = "Components", Order = 2)]
		private ObservableCollection<Component> _components = new ObservableCollection<Component>();

		public ReadOnlyObservableCollection<Component> Components { get; private set; }

		public Entity(Scene scene)
        {
            Debug.Assert(scene != null);
            ParentScene = scene;
            Enabled = true;
            _components.Add(new TransformComponent(this));
            _components.Add(new TagComponent(this));
            OnDeserialized(new StreamingContext());
		}

		public Component GetComponent(Type type) => Components.FirstOrDefault(x => x.GetType() == type);
		public T GetComponent<T>() where T : Component => GetComponent(typeof(T)) as T;
        public bool HasComponent<T>() where T : Component => GetComponent<T>() != null ? true : false;
		public T AddComponent<T>(params object[] args) where T : Component
		{
            if (!_components.Any(x => x.GetType() == typeof(T)))
            {
                T component = (T)(Activator.CreateInstance(typeof(T), args));
                _components.Add(component);
				OnPropertyChanged("Components");

				// Notify the engine about the new component
				EngineAPI.AddComponentToEntity(this, component);
                return component;
            }
            else throw new Exception($"Entity already has component type: {typeof(T)} assign to it.");
		}


		[OnDeserialized]
        private void OnDeserialized(StreamingContext ctx)
        {
            if (_components != null)
            {
				Components = new ReadOnlyObservableCollection<Component>(_components);
				OnPropertyChanged("Components");
			}
		}

		#region IDisposable stuff
		private bool _disposed;
		protected virtual void Dispose(bool disposing)
		{
			if (!_disposed)
			{
				if (disposing)
				{
					// TODO: dispose managed state (managed objects)
				}

                // TODO: free unmanaged resources (unmanaged objects) and override finalizer
                // TODO: set large fields to null
                IsActive = false;
				_disposed = true;
			}
		}

		// // TODO: override finalizer only if 'Dispose(bool disposing)' has code to free unmanaged resources
		// ~Entity()
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

	public class MSEntityModel : ViewModelBase
    {
        private bool _enableUpdates = true;
        private bool? _enabled;
        public bool? Enabled
        {
            get => _enabled;
            set
            {
                if (_enabled != value)
                {
                    _enabled = value;
                    OnPropertyChanged();
                }
            }
        }

        private bool? _visible;
        public bool? Visible
        {
            get => _visible;
            set
            {
                if (_visible != value)
                {
                    _visible = value;
                    OnPropertyChanged();
                }
            }
        }

        private string _name;

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

        private readonly ObservableCollection<IMSComponent> _components = new ObservableCollection<IMSComponent>();
        public ReadOnlyObservableCollection<IMSComponent> Components { get; private set; }
        public List<Entity> SelectedEntities { get; }

        public MSEntityModel(List<Entity> entities)
        {
            Debug.Assert(entities != null && entities.Any());
            Components = new ReadOnlyObservableCollection<IMSComponent>(_components);
            SelectedEntities = entities;
            PropertyChanged += (s, e) =>
            {
                if (_enableUpdates) UpdateEntities(e.PropertyName);
            };
		}
		public T GetMSComponent<T>() where T : IMSComponent => (T)Components.FirstOrDefault(x => x.GetType() == typeof(T));

		protected virtual bool UpdateEntities(string propertyName)
        {
			var property = typeof(Entity).GetProperty(propertyName);
			if (property != null)
			{
				var value = typeof(MSEntityModel).GetProperty(propertyName).GetValue(this);
				SelectedEntities.ForEach(x => property.SetValue(x, value));
				return true;
			}
			return false;
        }

        public static float? GetMixedValues<T>(List<T> objects, Func<T, float> getProperty)
        {
            var value = getProperty(objects.First());
            return objects.Skip(1).Any(x => !getProperty(x).IsEqualTo(value)) ? null : value;
        }
		public static int? GetMixedValues<T>(List<T> objects, Func<T, int> getProperty)
		{
			var value = getProperty(objects.First());
			return objects.Skip(1).Any(x => value != getProperty(x)) ? null : value;
		}

		public static bool? GetMixedValues<T>(List<T> objects, Func<T, bool> getProperty)
		{
			var value = getProperty(objects.First());
			return objects.Skip(1).Any(x => value != getProperty(x)) ? null : value;
		}

        public static string GetMixedValues<T>(List<T> objects, Func<T, string> getProperty)
		{
			var value = getProperty(objects.First());
			return objects.Skip(1).Any(x => value != getProperty(x)) ? null : value;
		}

        protected virtual bool UpdateMSEntities()
        {
            Enabled = GetMixedValues(SelectedEntities, new Func<Entity, bool>(x => x.Enabled));
            Name = GetMixedValues(SelectedEntities, new Func<Entity, string>(x => x.Name));

            return true;
        }

        private void MakeComponentList()
        {
            _components.Clear();
            var firstEnttiy = SelectedEntities.FirstOrDefault();
            if (firstEnttiy == null) return;

            foreach (var c in firstEnttiy.Components)
            {
                var type = c.GetType();
                if (!SelectedEntities.Skip(1).Any(x => x.GetComponent(type) == null))
                {
                    Debug.Assert(Components.FirstOrDefault(x => x.GetType() == type) == null);
                    _components.Add(c.GetMultiselectionComponent(this));
                }
            }
            OnPropertyChanged("Components");
        }

        public void Refresh()
        {
            _enableUpdates = false;
            UpdateMSEntities();
            MakeComponentList();
            _enableUpdates = true;
        }
    }

    class MSEntity : MSEntityModel
    {
        public MSEntity(List<Entity> entities) : base(entities)
        {
            Refresh();
        }
    }
}
