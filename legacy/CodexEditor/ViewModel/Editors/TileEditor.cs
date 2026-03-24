using CodexEditor.View.Editors.Components;
using CodexEditor.ViewModel.ECS;
using CodexEditor.ViewModel.GameProject;
using CodexEngine;
using CodexEngine.Scene;
using CodexEngine.Structs;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Numerics;
using System.Reflection;
using System.Runtime.Intrinsics;
using System.Runtime.Serialization;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Automation;
using System.Windows.Input;
using System.Windows.Media.Imaging;
using System.Xml.Linq;

namespace CodexEditor.ViewModel.Editors
{
	[DataContract]
	public class TileInfo
	{
		[DataMember]
		public Vector2 SrcCoords { get; set; }
		[DataMember]
		public Vector2 WorldCoords { get; set; }
		[DataMember]
		public int Layer { get; set; }
		public CroppedBitmap Image { get; set; }
	}

	internal class TileEditor : ViewModelBase
	{
		private BitmapImage _image;
		private ObservableCollection<TileInfo> _tiles;
		private Vector2 _selectedTileCoord;
		private Vector2 _tileSize;

		private Project _project;
		private TilemapComponent _component;

		private ObservableCollection<Entity> _entities;
		public ReadOnlyObservableCollection<Entity> Entities { get; private set; }
		public ReadOnlyObservableCollection<Sprite> Sprites { get; private set; }
		public ReadOnlyObservableCollection<TileInfo> Tiles { get; private set; }
		public TilemapComponent Component { get; private set; }
		public Vector2 SelectedTileCoord
		{
			get => _selectedTileCoord;
			set
			{
				if (_selectedTileCoord != value)
				{
					_selectedTileCoord = value;
					// Don't need to call this from the engine thread.
					CodexAPI.SetSelectedTileCoord(_selectedTileCoord);
					OnPropertyChanged();
				}
			}
		}

		private Entity _activeEntity;
		public Entity ActiveEntity
		{
			get => _activeEntity;
			set
			{
				_activeEntity = value;
				Refresh();
				OnPropertyChanged();
			}
		}

		private string _activeEntityName;
		public string ActiveEntityName
		{
			get
			{
				if (_activeEntity != null) return _activeEntityName;
				else return "No active entity";
			}
			private set
			{
				if (_activeEntityName != value)
				{
					_activeEntityName = value;
					OnPropertyChanged();
				}
			}
		}

		public static TileEditor Instance { get; private set; }

		public TileEditor(Project project) // TODO: Shouldn't this class be static?
		{
			_project = project;
			_tiles = new ObservableCollection<TileInfo>();
			Tiles = new ReadOnlyObservableCollection<TileInfo>(_tiles);
			RefreshEntities();
			Instance = this;
		}

		public Vector2 GetCoordSnappedToGrid(Vector2 coord)
		{
			var gridSize = _component.GridSize;
			return new Vector2
			{
				X = (int)(coord.X / gridSize.X) * gridSize.X,
				Y = (int)(coord.Y / gridSize.Y) * gridSize.Y
			};
		}

		public void RefreshEntities()
		{
			_entities = new ObservableCollection<Entity>(
				_project.ActiveScene.Entities.Where(x => x.HasComponent<TilemapComponent>()).ToList());
			Entities = new ReadOnlyObservableCollection<Entity>(_entities);
			OnPropertyChanged("Entities");
		}

		public void Refresh()
		{
			_component = ActiveEntity.GetComponent<TilemapComponent>();
			Component = _component;
			ActiveEntityName = _activeEntity.Name;
			Sprites = _component.Sprites;

			_tileSize = _component.TileSize;
			var tilesetSize = new Vector2(_component.Texture.Width, _component.Texture.Height);
			int col = (int)(tilesetSize.X / _tileSize.X);
			int row = (int)(tilesetSize.Y / _tileSize.Y);
			var offset = _component.TileOffset;
			_image = new BitmapImage(new Uri(_component.Texture.FilePath));
			_tiles.Clear();

			for (int y = 0; y < row; ++y)
			{
				for (int x = 0; x < col; ++x)
				{
					_tiles.Add(new TileInfo
					{
						SrcCoords = new Vector2(x, y),
						Image = new CroppedBitmap(
							_image,
							new Int32Rect(
								(int)((x * (int)_tileSize.X) + offset.X),
								(int)((y * (int)_tileSize.Y) + offset.Y),
								(int)(_tileSize.X - offset.X),
								(int)(_tileSize.Y - offset.Y)))
					});
				}
			}
			OnPropertyChanged("Tiles");
		}

		public void NotifyTilePick(TileInfo tileInfo)
		{
			SelectedTileCoord = new Vector2(
				tileInfo.SrcCoords.X * _tileSize.X, tileInfo.SrcCoords.Y * _tileSize.Y);
		}
	}
}
