#include "game_netman.h"

GameNetworkManager::GameNetworkManager(Scene* scene, NetManager& net, const scene::Prefab& player_prefab,
                                       const transform& spawn_location)
    : scene_{ scene }
    , net_{ net }
    , player_prefab_{ player_prefab_ }
    , network_spawn_transform_{ spawn_location }
{
    net_.attach_on_client_join(bind_event_delegate(this, &GameNetworkManager::on_player_join));
    net_.attach_on_client_leave(bind_event_delegate(this, &GameNetworkManager::on_player_leave));

    if (!scene_)
        throw NetworkException("Cannot replicate a null scene");
}

Entity GameNetworkManager::create_replicated_entity(const std::optional<transform>& transform,
                                                    std::string_view default_tag, UUID uuid)
{
    Entity entity = scene_->create_entity(std::move(transform), default_tag, uuid);
    register_entity_for_replication(entity);
    return entity;
}

Entity GameNetworkManager::create_replicated_prefab(
    const scene::Prefab& prefab, const std::optional<transform>& transform, std::string_view default_tag, UUID uuid)
{
    Entity entity = scene_->instantiate_prefab(prefab, std::move(transform), default_tag, uuid);
    register_entity_for_replication(entity);
    return entity;
}

void GameNetworkManager::on_update(f32 dt)
{
    std::optional<IncomingPacket> maybe_packet;
    while ((maybe_packet = net_.dequeue()) != std::nullopt) {
        handle_incoming_packet(std::move(*maybe_packet));
    }

    for (Entity e : replicated_entities_) {
        transform& trans = e.get_component<TransformComponent>();

        OutgoingPacket            packet;
        NetworkedEntityDescriptor desc;

        auto it = std::find_if(net_to_local_.begin(), net_to_local_.end(),
                               [entity = e](const auto& e) { return e.second == entity; });
        if (it != net_to_local_.end()) {
            desc.entity = it->first;
        } else {
            throw NetworkException("Entity does not map to a NetId");
        }

        packet.type     = PacketType::EntitySynchronize;
        packet.kind     = TargetKind::Broadcast;
        packet.channel  = 0;
        packet.reliable = true;
        packet.data.resize(sizeof(desc));
        *(NetworkedEntityDescriptor*)packet.data.data() = desc;
        desc.trans                                  = trans;

        net_.send(std::move(packet));
    }
}

void GameNetworkManager::on_player_join(const NetManager::NetPeer& client)
{
    Entity entity = scene_->instantiate_prefab(player_prefab_, network_spawn_transform_,
                                               fmt::format("game_netman:networked_entity:id#{}", client.id));

    register_entity_for_replication(entity, client.id);
}

void GameNetworkManager::on_player_leave(const NetManager::NetPeer& client)
{
    Entity entity = net_to_local_[client.id];
    if (auto it = std::find_if(replicated_entities_.begin(), replicated_entities_.end(),
                               [entity](const Entity& e) { return entity == e; });
        it != replicated_entities_.end())
        replicated_entities_.erase(it);

    net_to_local_.erase(client.id);
}

void GameNetworkManager::register_entity_for_replication(Entity entity, NetId id)
{
    net_to_local_[id] = entity;
    replicated_entities_.push_back(entity);
}

void GameNetworkManager::handle_incoming_packet(IncomingPacket&& packet)
{
    switch (packet.type) {
        using enum PacketType;

        case None:
        default: break;
        case EntityCreate: {
        } break;
        case EntityDestroy: {
        } break;
        case EntitySynchronize: {
            Entity                     entity = net_to_local_[packet.source];
            NetworkedEntityDescriptor* desc   = (NetworkedEntityDescriptor*)packet.data.data();

            entity.get_component<TransformComponent>() = (transform)desc->trans;
        } break;
        case ClientJoin: {
        } break;
        case ClientLeave: {
        } break;
    }
}
