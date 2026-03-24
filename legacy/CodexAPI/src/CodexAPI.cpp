#include "CodexAPI.h"

extern codex::Window* g_WindowInstance;

// Utility functions
namespace {
	codex::Entity CreateCodexEntityFromID(int32_t id, codex::Scene* scene)
	{
		return std::move(codex::Entity((entt::entity)id, scene));
	}
}

void Init()
{
	if (g_WindowInstance == nullptr)
		g_WindowInstance = codex::Window::Get();
	else printf("instance already initialized.\n");
}

void CreateWindow(const codex::Window::Properties& properties, const void* nativeWindow)
{
	g_WindowInstance->Init(properties, nativeWindow);
}

void Destroy()
{
	g_WindowInstance->Destroy();
}

void StartEngineThread()
{
	g_WindowInstance->EngineThread();
}

void Update()
{
	g_WindowInstance->Update();
}

void ResizeViewport(int newWidth, int newHeight)
{
	g_WindowInstance->OnWindowResize_Event(newWidth, newHeight);
}

codex::Scene* GetCurrentScene()
{
	return nullptr;
}

void ChangeScene(int sceneID)
{

}

void SetCameraProjection(int minX, int maxX, int minY, int maxY)
{
	auto* active_scene = g_WindowInstance->GetCurrentScene();
	//active_scene->SetCameraProjection(minX, maxX, minY, maxY);
}

int32_t CreateEntity(EntityDescriptor* desc)
{
	return g_WindowInstance->GetCurrentScene()->CreateEntity(desc->tag.tag).GetId();
}

void RemoveEntity(int32_t id)
{
	g_WindowInstance->GetCurrentScene()->RemoveEntity(id);
}

void UpdateEntityFromDescriptor(int32_t id, EntityDescriptor* desc)
{
	// TODO: Update more efficiently
	auto* active_scene = (codex::EditorScene*)g_WindowInstance->GetCurrentScene();
	auto entity = CreateCodexEntityFromID(id, active_scene);

	auto& transform_component = entity.GetComponent<codex::TransformComponent>();

	// TODO: Use memcpy here.
	transform_component.position.x = desc->transform.position[0];
	transform_component.position.y = desc->transform.position[1];
	transform_component.position.z = desc->transform.position[2];

	transform_component.rotation.x = desc->transform.rotation[0];
	transform_component.rotation.y = desc->transform.rotation[1];
	transform_component.rotation.z = desc->transform.rotation[2];

	transform_component.scale.x = desc->transform.scale[0];
	transform_component.scale.y = desc->transform.scale[1];
	transform_component.scale.z = desc->transform.scale[2];
	//desc->transform.ToNative(transform_component);

	/*
	if (entity.HasComponent<codex::SpriteRendererComponent>())
	{
		// TODO: Stop creating a new Texture2D object on every update.
		auto& sprite_component = entity.GetComponent<codex::SpriteRendererComponent>();
		
		auto file_path = desc->spriteRenderer.sprite.texture.filePath;
		std::shared_ptr<codex::Texture2D> texture = nullptr;
		if (!codex::Resources::HasResource(file_path))
			texture = codex::Resources::Load<codex::Texture2D>(file_path);
		else 
			texture = codex::Resources::GetResource<codex::Texture2D>(file_path);

		auto sprite = codex::Sprite(texture, desc->spriteRenderer.sprite.srcRect);
		sprite_component.SetSprite(sprite);
		sprite_component.SetZIndex(desc->spriteRenderer.zIndex);
		sprite_component.SetColour(desc->spriteRenderer.colour);
	}
	else AddSpriteRendererComponent(id, &desc->spriteRenderer);*/
}

void SetSelectedTileCoord(codex::Vector2f newCoord)
{
	auto* active_scene = (codex::EditorScene*)g_WindowInstance->GetCurrentScene();
	active_scene->SetSelectedTileCoord(newCoord);
}

codex::Vector2f GetSelectedTileCoord()
{
	auto* active_scene = (codex::EditorScene*)g_WindowInstance->GetCurrentScene();
	return active_scene->GetSelectedTileCoord();
}

void GetMousePositionInWorld(codex::Vector2f& vec)
{
	auto* active_scene = (codex::EditorScene*)g_WindowInstance->GetCurrentScene();
	vec = active_scene->GetMousePositionInWorld();
}

void SetActiveAction(codex::EditorAction newAction)
{
	auto* active_scene = (codex::EditorScene*)g_WindowInstance->GetCurrentScene();
	active_scene->SetActiveAction(newAction);
}

void SetSelectedEntityID(uint32_t id)
{
	auto* active_scene = (codex::EditorScene*)g_WindowInstance->GetCurrentScene();
	active_scene->SetSelectedEntityId(id);
}

void AddSpriteRendererComponent(int32_t id, DescriptorSpriteRendererComponent* c)
{
	auto* active_scene = (codex::EditorScene*)g_WindowInstance->GetCurrentScene();
	auto entity = CreateCodexEntityFromID(id, active_scene);

	const auto& descriptor_sprite = c->sprite;
	auto native_sprite = codex::Sprite(
		codex::Resources::Load<codex::Texture2D>(descriptor_sprite.texture.filePath),
		descriptor_sprite.srcRect);
	entity.AddComponent<codex::SpriteRendererComponent>(native_sprite, c->colour);
}

// TODO: Consider adding argument-less constructors to components and only using Add functions to
// literally only add and then set the parameters using their Update functions.

void AddTilemapComponent(int32_t id, DescriptorTilemapComponent* c, TileInfo* const tiles)
{
	auto* active_scene = (codex::EditorScene*)g_WindowInstance->GetCurrentScene();
	auto entity = CreateCodexEntityFromID(id, active_scene);

	auto& tilemap_component = entity.AddComponent<codex::TilemapComponent>(
		codex::Resources::Load<codex::Texture2D>(c->textureFilePath),
		c->gridSize,
		c->tileSize);

	for (int i = 0; i < c->tileCount; ++i)
	{
		tilemap_component.SetLayer(tiles[i].layer);
		tilemap_component.AddTile(tiles[i].worldPos, tiles[i].tilePos);
	}

	tilemap_component.SetLayer(c->layer);
}

void UpdateTransformComponent(int32_t id, DescriptorTransformComponent* c)
{
	std::cout << "Updating transform component of entity: " << id << std::endl;
	auto* active_scene = (codex::EditorScene*)g_WindowInstance->GetCurrentScene();
	auto entity = CreateCodexEntityFromID(id, active_scene);

	auto& transform_component = entity.GetComponent<codex::TransformComponent>();
	std::memcpy(&transform_component.position, c->position, sizeof(float) * 3);
	std::memcpy(&transform_component.rotation, c->rotation, sizeof(float) * 3);
	std::memcpy(&transform_component.scale, c->scale, sizeof(float) * 3);
}

void UpdateTagComponent(int32_t id, DescriptorTagComponent* c)
{
	std::cout << "Updating tag component of entity: " << id << std::endl;
}

void UpdateSpriteRendererComponent(int32_t id, DescriptorSpriteRendererComponent* c)
{
	std::cout << "Updating sprite renderer component of entity: " << id << std::endl;
	auto* active_scene = (codex::EditorScene*)g_WindowInstance->GetCurrentScene();
	auto entity = CreateCodexEntityFromID(id, active_scene);

	auto& sprite_component = entity.GetComponent<codex::SpriteRendererComponent>();

	codex::Sprite sprite(codex::Resources::Load<codex::Texture2D>(c->sprite.texture.filePath), c->sprite.srcRect);
	sprite_component.SetSprite(sprite);
	sprite_component.SetColour(c->colour);
	sprite_component.SetZIndex(c->zIndex);
}

void UpdateTilemapComponent(int32_t id, DescriptorTilemapComponent* c, TileInfo* const tiles)
{
	std::cout << "Updating tilemap component of entity: " << id << std::endl;
	auto* active_scene = (codex::EditorScene*)g_WindowInstance->GetCurrentScene();
	auto entity = CreateCodexEntityFromID(id, active_scene);

	auto& tilemap_component = entity.GetComponent<codex::TilemapComponent>();

	tilemap_component.SetTexture(codex::Resources::Load<codex::Texture2D>(c->textureFilePath));
	tilemap_component.SetGridSize(c->gridSize);
	tilemap_component.SetTileSize(c->tileSize);

	for (int i = 0; i < c->tileCount; ++i)
	{
		tilemap_component.SetLayer(tiles[i].layer);
		tilemap_component.AddTile(tiles[i].worldPos, tiles[i].tilePos);
	}

	tilemap_component.SetLayer(c->layer);
}
