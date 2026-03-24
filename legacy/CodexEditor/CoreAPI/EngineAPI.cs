using System;
using System.Diagnostics;
using System.Linq;
using CodexEditor.ViewModel.ECS;
using CodexEngine;
using CodexEngine.Components;
using CodexEngine.Structs;
using CodexEditor.Util;
using System.Collections.Generic;
using System.Collections.Concurrent;
using System.Threading.Tasks;
using System.Security.AccessControl;
using System.Threading;

namespace CodexEditor.CoreAPI
{
	public enum EngineCommandType
	{
		CreateEntity,
		RemoveEntity,
		UpdateEntityFromDescriptor,
		AddComponentToEntity,
		UpdateTransformComponent,
		UpdateTagComponent,
		UpdateSpriteRendererComponent,
		UpdateTilemapComponent
	}

	public class EngineCommand
	{
		private readonly ManualResetEvent _completionSignal;

		public EngineCommandType Type { get; set; }
		public object[] Args { get; set; }
		public object ReturnValue { get; set; }

		public EngineCommand(EngineCommandType type, params object[] args)
		{
			Type = type;
			Args = args;
			ReturnValue = (int)0;
			_completionSignal = new ManualResetEvent(false);
		}

		public void SignalCompletion() => _completionSignal.Set();

		public void WaitForCompletion() => _completionSignal.WaitOne();
	}

	public static class EngineAPI
	{
		public static readonly ConcurrentStack<EngineCommand> CommandStack = new ConcurrentStack<EngineCommand>();

		public static T ExecuteCommand<T>(EngineCommandType type, params object[] args)
		{
			var command = new EngineCommand(type, args);
			CommandStack.Push(command);
			command.WaitForCompletion();
			return (T)command.ReturnValue;
		}

		public static int ExecuteCommand(EngineCommandType type, params object[] args)
		{
			return ExecuteCommand<int>(type, args);
		}

		public static int CreateEntity(Entity entity)
		{
			EntityDescriptor desc = new EntityDescriptor();

			if (entity != null)
			{
				// Default non-removable components
				var transformComponent = entity.GetComponent<TransformComponent>();
				desc.Transform = transformComponent.ToDescriptor();

				var tagComponent = entity.GetComponent<TagComponent>();
				desc.Tag = tagComponent.ToDescriptor();
			}

			return ExecuteCommand(EngineCommandType.CreateEntity, desc);
		}

		public static void RemoveEntity(Entity entity)
		{
			ExecuteCommand(EngineCommandType.RemoveEntity, entity.EntityID);
		}

		public static void UpdateEntity(Entity entity)
		{
			var desc = new EntityDescriptor();

			foreach (var c in entity.Components)
			{
				// TODO: Use reflection to avoid code repetition here.
				if (c is TransformComponent trans)
				{
					desc.Transform = trans.ToDescriptor();
				}
				else if (c is TagComponent tag)
				{
					desc.Tag = tag.ToDescriptor();
				}
				else if (c is SpriteRendererComponent spriteRnederer)
				{
					desc.SpriteRenderer = spriteRnederer.ToDescriptor();
				}
			}

			ExecuteCommand(EngineCommandType.UpdateEntityFromDescriptor, entity.EntityID, desc);
		}

		public static void AddComponentToEntity<T>(Entity entity, T instance)
		{
			ExecuteCommand(EngineCommandType.AddComponentToEntity, entity.EntityID, instance);
		}

		public static void UpdateEntityComponent<T>(Entity entity, T instance)
		{
			// TODO: Use reflection or generics to avoid code repetition here

			if (instance is TransformComponent trans)
			{
				ExecuteCommand(EngineCommandType.UpdateTransformComponent, entity.EntityID, trans.ToDescriptor());
			}
			else if (instance is TagComponent tag)
			{
				ExecuteCommand(EngineCommandType.UpdateTagComponent, entity.EntityID, tag.ToDescriptor());
			}
			else if (instance is SpriteRendererComponent spriteRenderer)
			{
				ExecuteCommand(EngineCommandType.UpdateSpriteRendererComponent, entity.EntityID, spriteRenderer.ToDescriptor());
			}
			else if (instance is TilemapComponent tilemap)
			{
				TileInfo[] tiles = null;
				ExecuteCommand(EngineCommandType.UpdateTilemapComponent, entity.EntityID, tilemap.ToDescriptor(ref tiles), tiles);
			}
		}
	}
}