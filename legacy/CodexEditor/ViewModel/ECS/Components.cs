
using System;
using System.Linq;
using System.Collections.Generic;
using System.Diagnostics;
using System.Numerics;
using CodexEditor.Util;
using System.Runtime.Serialization;
using CodexEditor.CoreAPI;
using CodexEngine.Scene;
using CodexEngine.Renderer;
using System.Windows.Documents;
using System.Collections.ObjectModel;
using System.Reflection;
using System.Drawing;
using CodexEditor.ViewModel.Editors;
using CodexEngine.Structs;

namespace CodexEditor.ViewModel.ECS
{
	public interface IMSComponent
	{

	}

	[DataContract]
	public abstract class Component : ViewModelBase
	{
		[DataMember]
		public Entity Parent { get; set; }

		public Component(Entity parent)
		{
			Debug.Assert(parent != null);
			Parent = parent;
		}

		public abstract IMSComponent GetMultiselectionComponent(MSEntityModel msEntity);
	}

	public abstract class MSComponent<T> : ViewModelBase, IMSComponent where T : Component
	{
		private bool _enableUpdates = true;
		public List<T> SelectedComponents { get; }

		public MSComponent(MSEntityModel msEntity)
		{
			Debug.Assert(msEntity?.SelectedEntities?.Any() == true);
			SelectedComponents = msEntity.SelectedEntities.Select(x => x.GetComponent<T>()).ToList();
			PropertyChanged += (s, e) =>
			{
				if (_enableUpdates)
					UpdateComponents(e.PropertyName);
			};
		}

		protected abstract bool UpdateComponents(string propertyName);
		protected abstract bool UpdateMSComponent();

		public void Refresh()
		{
			_enableUpdates = false;
			UpdateMSComponent();
			_enableUpdates = true;
		}
	}

	[DataContract]
	class TransformComponent : Component
	{
		private Vector3 _position;
		private Vector3 _rotation;
		private Vector3 _scale;
		[DataMember]
		public Vector3 Position
		{
			get => _position;
			set
			{
				if (_position != value)
				{
					_position = value;
					OnPropertyChanged();
				}
			}
		}
		[DataMember]
		public Vector3 Rotation
		{
			get => _rotation;
			set
			{
				if (_rotation != value)
				{
					_rotation = value;
					OnPropertyChanged();
				}
			}
		}
		[DataMember]
		public Vector3 Scale
		{
			get => _scale;
			set
			{
				if (_scale != value)
				{
					_scale = value;
					OnPropertyChanged();
				}
			}
		}

		public TransformComponent(Entity parent) : base(parent)
		{
			Position = new Vector3();
			Rotation = new Vector3();
			Scale = new Vector3(1.0f);
		}

		public override IMSComponent GetMultiselectionComponent(MSEntityModel msEntity) => new MSTransformComponent(msEntity);
	}

	[DataContract]
	class TagComponent : Component
	{
		private string _tag;
		[DataMember]
		public string Tag
		{
			get => _tag;
			set
			{
				if (_tag != value)
				{
					_tag = value;
					OnPropertyChanged();
				}
			}
		}

		public TagComponent(Entity parent, string tag = "default tag") : base(parent)
		{
			Tag = tag;
		}

		public override IMSComponent GetMultiselectionComponent(MSEntityModel msEntity) => new MSTagComponent(msEntity);
	}

	[DataContract]
	class SpriteRendererComponent : Component
	{
		private Vector4 _colour;
		private Sprite _sprite;
		private int _zIndex;

		[DataMember]
		public Vector4 Colour { get => _colour; set => _colour = value; }
		[DataMember]
		public Sprite Sprite { get => _sprite; set => _sprite = value; }
		[DataMember]
		public int ZIndex { get => _zIndex; set => _zIndex = value; }

		public SpriteRendererComponent(Entity parent, Vector4 colour) : this(parent, null, colour)
		{

		}

		public SpriteRendererComponent(Entity parent, Sprite sprite) : this(parent, sprite, new Vector4(1.0f))
		{

		}

		public SpriteRendererComponent(Entity parent, Sprite sprite, Vector4 colour) : base(parent)
		{
			_sprite = sprite;
			_colour = colour;
			_zIndex = 0;
		}

		public override IMSComponent GetMultiselectionComponent(MSEntityModel msEntity) => new MSSpriteRendererComponent(msEntity);
	}

	[DataContract]
	class TilemapComponent : Component
	{
		[DataMember(Name = "Texture")]
		private Texture2D _texture;
		[DataMember(Name = "Tiles")]
		//private Dictionary<int, Dictionary<Vector2, Vector2>> _tiles;
		private List<TileInfo> _tiles;
		[DataMember(Name = "GridSize")]
		private Vector2 _gridSize;
		[DataMember(Name = "TileSize")]
		private Vector2 _tileSize;
		[DataMember(Name = "TileOffset")]
		private Vector2 _tileOffset;
		[DataMember(Name = "Layer")]
		private int _layer;

		//public Dictionary<int, Dictionary<Vector2, Vector2>> Tiles { get => _tiles; set => _tiles = value; }
		public List<TileInfo> Tiles { get => _tiles; set => _tiles = value; }
		public Texture2D Texture { get => _texture; set => _texture = value; }
		public Vector2 GridSize { get => _gridSize; set => _gridSize = value; }
		public Vector2 TileSize { get => _tileSize; set => _tileSize = value; }
		public Vector2 TileOffset { get => _tileOffset; set => _tileOffset = value; }
		public int Layer
		{
			get => _layer;
			set
			{
				if (_layer != value)
				{
					_layer = value;
					OnPropertyChanged();
				}
			}
		}
		public ReadOnlyObservableCollection<Sprite> Sprites { get; private set; }
		public int GetTotalSize { get => _tiles.Count; }

		public TilemapComponent(
			Entity parent, 
			Texture2D texture, 
			Vector2 gridSize, 
			Vector2 tileSize, 
			Vector2 tileOffset) : base(parent)
		{
			_texture = texture;
			_gridSize = gridSize;
			_tileSize = tileSize;
			_tileOffset = tileOffset;
			_tiles = new List<TileInfo>();

			RefreshSprites();
		}

		public void RefreshSprites()
		{
			/*if (_tileSize.X > 0 && _tileSize.Y > 0 && _gridSize.X > 0 && _gridSize.Y > 0)
			{
				_sprites.Clear(); 
				for (int y = 0; y < _texture.Height; y += (int)_tileSize.Y)
				{
					for (int x = 0; x < _texture.Width; x += (int)_tileSize.X)
					{
						_sprites.Add(new Sprite(_texture, new RectangleF(x, y, _tileSize.X, _tileSize.Y)));
					}
				}
			}*/
		}

		public void AddTile(Vector2 worldPos, int tileID)
		{
			var vec = new Vector2();
			int columns = _texture.Width / (int)_gridSize.X;
			int x = tileID % columns;
			int y = tileID / columns;
			vec.X = y * _gridSize.X;
			vec.Y = y * _gridSize.X;
			RemoveTile(worldPos);
			_tiles.Add(new TileInfo { SrcCoords = vec, WorldCoords = worldPos, Layer = Layer });
		}

		public void AddTile(Vector2 worldPos, Vector2 tile)
		{
			RemoveTile(worldPos);
			_tiles.Add(new TileInfo { SrcCoords = tile, WorldCoords = worldPos, Layer = Layer });
		}

		public void RemoveTile(Vector2 worldPos)
		{
			_tiles.RemoveAll(x => x.WorldCoords == worldPos && x.Layer == Layer);
		}

		public override IMSComponent GetMultiselectionComponent(MSEntityModel msEntity) => new MSTilemapComponent(msEntity); 
	}

	// Multi select components
	sealed class MSTransformComponent : MSComponent<TransformComponent>
	{
		private float? _posX;
		public float? PosX
		{
			get => _posX;
			set
			{
				if (!_posX.IsEqualTo(value))
				{
					_posX = value;
					OnPropertyChanged();
				}
			}
		}
		private float? _posY;
		public float? PosY
		{
			get => _posY;
			set
			{
				if (!_posY.IsEqualTo(value))
				{
					_posY = value;
					OnPropertyChanged();
				}
			}
		}
		private float? _posZ;
		public float? PosZ
		{
			get => _posZ;
			set
			{
				if (!_posZ.IsEqualTo(value))
				{
					_posZ = value;
					OnPropertyChanged();
				}
			}
		}

		private float? _rotX;
		public float? RotX
		{
			get => _rotX;
			set
			{
				if (!_rotX.IsEqualTo(value))
				{
					_rotX = value;
					OnPropertyChanged();
				}
			}
		}
		private float? _rotY;
		public float? RotY
		{
			get => _rotY;
			set
			{
				if (!_rotY.IsEqualTo(value))
				{
					_rotY = value;
					OnPropertyChanged();
				}
			}
		}
		private float? _rotZ;
		public float? RotZ
		{
			get => _rotZ;
			set
			{
				if (!_rotZ.IsEqualTo(value))
				{
					_rotZ = value;
					OnPropertyChanged();
				}
			}
		}

		private float? _scaleX;
		public float? ScaleX
		{
			get => _scaleX;
			set
			{
				if (!_scaleX.IsEqualTo(value))
				{
					_scaleX = value;
					OnPropertyChanged();
				}
			}
		}
		private float? _scaleY;
		public float? ScaleY
		{
			get => _scaleY;
			set
			{
				if (!_scaleY.IsEqualTo(value))
				{
					_scaleY = value;
					OnPropertyChanged();
				}
			}
		}
		private float? _scaleZ;
		public float? ScaleZ
		{
			get => _scaleZ;
			set
			{
				if (!_scaleZ.IsEqualTo(value))
				{
					_scaleZ = value;
					OnPropertyChanged();
				}
			}
		}

		public MSTransformComponent(MSEntityModel msEntity) : base(msEntity)
		{
			Refresh();
		}

		protected override bool UpdateComponents(string propertyName)
		{
			switch (propertyName)
			{
				case nameof(PosX):
				case nameof(PosY):
				case nameof(PosZ):
					SelectedComponents.ForEach(x =>
					{
						x.Position = new Vector3(
							_posX ?? x.Position.X,
							_posY ?? x.Position.Y,
							_posZ ?? x.Position.Z
						);
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;

				case nameof(RotX):
				case nameof(RotY):
				case nameof(RotZ):
					SelectedComponents.ForEach(x =>
					{
						x.Rotation = new Vector3(
							_rotX ?? x.Rotation.X,
							_rotY ?? x.Rotation.Y,
							_rotZ ?? x.Rotation.Z);
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;

				case nameof(ScaleX):
				case nameof(ScaleY):
				case nameof(ScaleZ):
					SelectedComponents.ForEach(x =>
					{
						x.Scale = new Vector3(
							_scaleX ?? x.Scale.X,
							_scaleY ?? x.Scale.Y,
							_scaleZ ?? x.Scale.Z);
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;
			}
			return false;
		}

		protected override bool UpdateMSComponent()
		{
			PosX = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TransformComponent, float>(x => x.Position.X));
			PosY = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TransformComponent, float>(x => x.Position.Y));
			PosZ = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TransformComponent, float>(x => x.Position.Z));

			RotX = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TransformComponent, float>(x => x.Rotation.X));
			RotY = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TransformComponent, float>(x => x.Rotation.Y));
			RotZ = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TransformComponent, float>(x => x.Rotation.Z));

			ScaleX = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TransformComponent, float>(x => x.Scale.X));
			ScaleY = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TransformComponent, float>(x => x.Scale.Y));
			ScaleZ = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TransformComponent, float>(x => x.Scale.Z));

			return true;
		}
	}

	sealed class MSTagComponent : MSComponent<TagComponent>
	{
		private string _tag;
		public string Tag
		{
			get => _tag;
			set
			{
				if (_tag != value)
				{
					_tag = value;
					OnPropertyChanged();
				}
			}
		}
		public MSTagComponent(MSEntityModel msEntity) : base(msEntity)
		{
			Refresh();
		}

		protected override bool UpdateComponents(string propertyName)
		{
			switch (propertyName)
			{
				case nameof(Tag):
					SelectedComponents.ForEach(x => x.Tag = Tag);
					return true;
			}
			return false;
		}

		protected override bool UpdateMSComponent()
		{
			Tag = MSEntityModel.GetMixedValues(SelectedComponents, new Func<TagComponent, string>(x => x.Tag));
			return true;
		}
	}

	sealed class MSSpriteRendererComponent : MSComponent<SpriteRendererComponent>
	{
		private float _colourX;
		private float _colourY;
		private float _colourZ;
		private float _colourW;
		private Sprite _sprite;
		private int _zIndex;

		public float ColourX
		{
			get => _colourX;
			set
			{
				if (_colourX != value)
				{
					_colourX = value;
					OnPropertyChanged();
				}
			}
		}
		public float ColourY
		{
			get => _colourY;
			set
			{
				if (_colourY != value)
				{
					_colourY = value;
					OnPropertyChanged();
				}
			}
		}
		public float ColourZ
		{
			get => _colourZ;
			set
			{
				if (_colourZ != value)
				{
					_colourZ = value;
					OnPropertyChanged();
				}
			}
		}
		public float ColourW
		{
			get => _colourW;
			set
			{
				if (_colourW != value)
				{
					_colourW = value;
					OnPropertyChanged();
				}
			}
		}
		public Sprite Sprite
		{
			get => _sprite;
			set
			{
				if (_sprite != value)
				{
					_sprite = value;
					OnPropertyChanged();
				}
			}
		}
		public int ZIndex
		{
			get => _zIndex;
			set
			{
				if (_zIndex != value)
				{
					_zIndex = value;
					OnPropertyChanged();
				}
			}
		}

		public MSSpriteRendererComponent(MSEntityModel msEntity) : base(msEntity)
		{
			Refresh();
		}

		protected override bool UpdateComponents(string propertyName)
		{
			switch (propertyName)
			{
				case nameof(ColourX):
				case nameof(ColourY):
				case nameof(ColourZ):
				case nameof(ColourW):
					SelectedComponents.ForEach(x =>
					{
						x.Colour = new Vector4(ColourX, ColourY, ColourZ, ColourW);
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;
				case nameof(Sprite):
					SelectedComponents.ForEach(x =>
					{
						x.Sprite = Sprite;
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;
				case nameof(ZIndex):
					SelectedComponents.ForEach(x =>
					{
						x.ZIndex = ZIndex;
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;
			}
			return false;
		}

		protected override bool UpdateMSComponent()
		{
			var colourResX = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<SpriteRendererComponent, float>(x => x.Colour.X));
			var colourResY = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<SpriteRendererComponent, float>(x => x.Colour.Y));
			var colourResZ = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<SpriteRendererComponent, float>(x => x.Colour.Z));
			var colourResW = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<SpriteRendererComponent, float>(x => x.Colour.W));

			ColourX = colourResX == null ? 0 : (float)colourResX;
			ColourY = colourResY == null ? 0 : (float)colourResY;
			ColourZ = colourResZ == null ? 0 : (float)colourResZ;
			ColourW = colourResW == null ? 0 : (float)colourResW;

			int? zIndexRes = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<SpriteRendererComponent, int>(x => x.ZIndex));

			ZIndex = zIndexRes == null ? 0 : (int)zIndexRes;

			// TODO: Handle texture coordinates as well (aka src_rect)
			var spriteRes = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<SpriteRendererComponent, string>(x => x.Sprite.Texture.FilePath));

			Sprite = new Sprite(new CodexEngine.Renderer.Texture2D(spriteRes));

			return true;
		}
	}

	sealed class MSTilemapComponent : MSComponent<TilemapComponent>
	{
		private Texture2D _texture;
		private List<TileInfo> _tiles;
		private Vector2 _gridSize;
		private Vector2 _tileSize;
		private Vector2 _tileOffset;
		private int _layer;

		public Texture2D Texture
		{
			get => _texture;
			set
			{
				if (_texture != value)
				{
					_texture = value;
					OnPropertyChanged();
				}
			}
		}

		public List<TileInfo> Tiles
		{
			get => _tiles;
			set
			{
				if (_tiles != value)
				{
					_tiles = value;
					OnPropertyChanged();
				}
			}
		}

		public float GridSizeX
		{
			get => _gridSize.X;
			set
			{
				if (_gridSize.X != value)
				{
					_gridSize.X = value;
					OnPropertyChanged();
				}
			}
		}

		public float GridSizeY
		{
			get => _gridSize.Y;
			set
			{
				if (_gridSize.Y != value)
				{
					_gridSize.Y = value;
					OnPropertyChanged();
				}
			}
		}

		public float TileSizeX
		{
			get => _tileSize.X;
			set
			{
				if (_tileSize.X != value)
				{
					_tileSize.X = value;
					OnPropertyChanged();
				}
			}
		}

		public float TileSizeY
		{
			get => _tileSize.Y;
			set
			{
				if (_tileSize.Y != value)
				{
					_tileSize.Y = value;
					OnPropertyChanged();
				}
			}
		}

		public float TileOffsetX
		{
			get => _tileOffset.X;
			set
			{
				if (_tileOffset.X != value)
				{
					_tileOffset.X = value;
					OnPropertyChanged();
				}
			}
		}

		public float TileOffsetY
		{
			get => _tileOffset.Y;
			set
			{
				if (_tileOffset.Y != value)
				{
					_tileOffset.Y = value;
					OnPropertyChanged();
				}
			}
		}

		public int Layer
		{
			get => _layer;
			set
			{
				if (_layer != value)
				{
					_layer = value;
					OnPropertyChanged();
				}
			}
		}

		public MSTilemapComponent(MSEntityModel msEntity) : base(msEntity) 
		{
			Refresh();
		}

		protected override bool UpdateComponents(string propertyName)
		{
			switch (propertyName)
			{
				case nameof(Texture):
					SelectedComponents.ForEach(x =>
					{
						x.Texture = Texture;
						x.RefreshSprites();
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;
				case nameof(Tiles):
					SelectedComponents.ForEach(x =>
					{
						x.Tiles = Tiles;
						x.RefreshSprites();
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;
				case nameof(GridSizeX):
				case nameof(GridSizeY):
					SelectedComponents.ForEach(x =>
					{
						x.GridSize = new Vector2(GridSizeX, GridSizeY);
						x.RefreshSprites();
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;
				case nameof(TileSizeX):
				case nameof(TileSizeY):
					SelectedComponents.ForEach(x =>
					{
						x.TileSize = new Vector2(TileSizeX, TileSizeY);
						x.RefreshSprites();
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;
				case nameof(TileOffsetX):
				case nameof(TileOffsetY):
					SelectedComponents.ForEach(x =>
					{
						x.TileOffset = new Vector2(TileOffsetX, TileOffsetY);
						x.RefreshSprites();
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;
				case nameof(Layer):
					SelectedComponents.ForEach(x =>
					{
						x.Layer = Layer;
						x.RefreshSprites();
						EngineAPI.UpdateEntityComponent(x.Parent, x);
					});
					return true;
			}
			return false;
		}

		protected override bool UpdateMSComponent()
		{
			var gridResX = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TilemapComponent, float>(x => x.GridSize.X));
			var gridResY = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TilemapComponent, float>(x => x.GridSize.Y));
			var tileSizeResX = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TilemapComponent, float>(x => x.TileSize.X));
			var tileSizeResY = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TilemapComponent, float>(x => x.TileSize.Y));
			var tileOffsetResX = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TilemapComponent, float>(x => x.TileOffset.X));
			var tileOffsetResY = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TilemapComponent, float>(x => x.TileOffset.Y));
			GridSizeX = gridResX == null ? 0 : (float)gridResX;
			GridSizeY = gridResY == null ? 0 : (float)gridResY;
			TileSizeX = tileSizeResX == null ? 0 : (float)tileSizeResX;
			TileSizeY = tileSizeResY == null ? 0 : (float)tileSizeResY;
			TileOffsetX = tileOffsetResX == null ? 0 : (float)tileOffsetResX;
			TileOffsetY = tileOffsetResY == null ? 0 : (float)tileOffsetResY;

			var textureRes = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TilemapComponent, string>(x => x.Texture.FilePath));
			Texture = new Texture2D(textureRes);

			var layerRes = MSEntityModel.GetMixedValues(
				SelectedComponents, new Func<TilemapComponent, int>(x => x.Layer));
			Layer = layerRes == null ? 0 : (int)layerRes;

			return true;
		}
	}
}
