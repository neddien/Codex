using CodexEditor.ViewModel.ECS;
using CodexEngine.Components;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace CodexEditor.Util
{
	internal static class DescriptorConverter
	{
		// TODO: Perform null checks on ?value(s)

		public static DescriptorTransformComponent ToDescriptor(this TransformComponent? value)
		{
			var desc = new DescriptorTransformComponent();
			desc.Position = value.Position;
			desc.Rotation = value.Rotation;
			desc.Scale = value.Scale;
			return desc;
		}
		public static DescriptorTagComponent ToDescriptor(this TagComponent? value)
		{
			var tag = new DescriptorTagComponent();
			tag.Tag = value.Tag;
			return tag;
		}

		public static DescriptorSpriteRendererComponent ToDescriptor(this SpriteRendererComponent? value)
		{
			return new DescriptorSpriteRendererComponent(value.Sprite, value.Colour);
		}

		public static DescriptorTilemapComponent ToDescriptor(this TilemapComponent? value, ref TileInfo[] tiles)
		{
			var desc = new DescriptorTilemapComponent();
			desc.TextureFilePath = value.Texture.FilePath;
			desc.TileSize = value.TileSize;
			desc.GridSize = value.GridSize;
			desc.TileCount = value.GetTotalSize;
			desc.Layer = value.Layer;

			tiles = new TileInfo[value.Tiles.Count];

			int i = 0;
			foreach (var tile in value.Tiles)
			{
				var tileInfo = new TileInfo
				{
					Layer = tile.Layer,
					TileID = tile.SrcCoords,
					WorldPos = tile.WorldCoords
				};
				tiles[i++] = tileInfo;
			}

			return desc;
		}
	}
}
