#pragma once

#include "netman.h"

using namespace codex;

struct NetworkedEntityDescriptor
{
    NetId     entity;
    transform trans;
};

class GameNetworkManager
{
    GameNetworkManager(Scene* scene, NetManager& net, const scene::Prefab& player_prefab,
                       const transform& spawn_location = transform{});

public:
    Entity create_replicated_entity(const std::optional<transform>& transform = std::nullopt,
                                    std::string_view default_tag = "default tag", UUID uuid = UUID{});
    Entity create_replicated_prefab(const scene::Prefab&            prefab,
                                    const std::optional<transform>& transform = std::nullopt,
                                    std::string_view default_tag = "default tag", UUID uuid = UUID{});

public:
    void on_update(f32 dt);
    void on_player_join(const NetManager::NetPeer& client);
    void on_player_leave(const NetManager::NetPeer& client);

private:
    void register_entity_for_replication(Entity entity, NetId id = NetId{});
    void handle_incoming_packet(IncomingPacket&& packet);

private:
    NetManager&                       net_;
    Scene*                            scene_;
    std::vector<Entity>               replicated_entities_;
    scene::Prefab                     player_prefab_;
    transform                         network_spawn_transform_;
    std::unordered_map<NetId, Entity> net_to_local_;
    mutable std::recursive_mutex      mutex_;
};
