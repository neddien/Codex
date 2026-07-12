#include "netman.h"

NetManager::NetManager(Mode mode, std::string_view ip, u16 port, u16 client_count)
    : mode_{ mode }
    , host_{ nullptr }
    , client_count_{ client_count }
{
    if (enet_initialize() != 0) {
        throw NetworkException("Failed to initialize enet");
    }

    if (mode == Mode::Server) {
        initialize_as_server(ip, port);
    } else {
        initialize_as_client(ip, port);
    }
}

NetManager::~NetManager()
{
    if (host_)
        enet_host_destroy(host_);

    enet_deinitialize();
}
void NetManager::initialize_as_server(std::string_view ip, u16 port)
{
    if (enet_address_set_host(&address_, ip.data()) != 0)
        throw NetworkException("Server: Failed to initialize address");
    address_.port = port;

    constexpr auto default_channel_count = 2;
    host_                                = enet_host_create(&address_, client_count_, default_channel_count, 0, 0);

    if (!host_)
        throw NetworkException("Server: Failed to create ENet host");
}

void NetManager::initialize_as_client(std::string_view ip, u16 port)
{
}

void NetManager::run()
{
    if (mode_ == Mode::Server)
        run_as_server();
    else
        run_as_client();
}

void NetManager::replicate_entity(Entity entity)
{
}

void NetManager::update_entities()
{
}

bool NetManager::send(OutgoingPacket&& packet)
{
    out_queue_.push(std::move(packet));
    return true;
}

std::optional<IncomingPacket> NetManager::dequeue()
{
    if (!in_queue_.empty()) {
        IncomingPacket packet = std::move(in_queue_.front());
        in_queue_.pop();
        return std::move(packet);
    }
    return std::nullopt;
}

void NetManager::run_as_server()
{
    ENetEvent event;

    constexpr auto default_event_timeout = 1000;
    while (enet_host_service(host_, &event, default_event_timeout) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                char ip[64]{};
                if (enet_address_get_host_ip(&event.peer->address, ip, sizeof(ip)) == 0) {
                    log(Info, "Received CONNECT event from ({}:{})", ip, event.peer->address.port);
                } else {
                    log(Warn, "Failed to convert IP of peer to string");
                    log(Info, "Received CONNECT event from ({}:{})", event.peer->address.host,
                        event.peer->address.port);
                }

                // handle connection as server
                on_peer_connect(event.peer);
            } break;
            case ENET_EVENT_TYPE_RECEIVE: {
                log(Info, "Received RECEIVE event");

                on_peer_receive(event.peer, event.packet);
            } break;
            case ENET_EVENT_TYPE_DISCONNECT: {
                log(Info, "Received DISCONNECT event");

                on_peer_disconnect(event.peer);
            } break;
            default: {
                log(Warn, "Ignored ENet event");
            } break;
        }
    }

    drain_outgoing_queue();
}

void NetManager::run_as_client()
{
}

bool NetManager::send_enet_packet(ENetPeer* peer, const OutgoingPacket& packet)
{
    if (!peer)
        return false;

    std::scoped_lock guard{ mutex_ };

    std::vector<u8> buf;
    buf.reserve(packet.data.size() + sizeof(packet.type) + sizeof(usize));
    write<u16>(buf, (u16)packet.type);
    write<usize>(buf, packet.data.size());
    write_bytes(buf, packet.data);

    u32 flags = packet.reliable ? ENET_PACKET_FLAG_RELIABLE : 0;

    ENetPacket* epacket = enet_packet_create(buf.data(), buf.size(), flags);

    if (!epacket) {
        log(Error, "Failed to create an ENetPacket");
    }

    if (enet_peer_send(peer, packet.channel, epacket) != 0) {
        enet_packet_destroy(epacket);
        return false;
    }

    return true;
}

void NetManager::drain_outgoing_queue()
{
    while (!out_queue_.empty()) {
        std::scoped_lock guard{ mutex_ };

        OutgoingPacket packet = std::move(out_queue_.front());
        out_queue_.pop();

        if (packet.targets.empty() && packet.kind != TargetKind::Broadcast)
            continue;

        switch (packet.kind) {
            case TargetKind::Unicast: {
                NetId    peer_id = packet.targets.front();
                NetPeer& peer    = peers_[peer_id];

                send_enet_packet(peer.peer, packet);
            } break;
            case TargetKind::Multicast: {
                for (NetId peer_id : packet.targets) {
                    NetPeer& peer = peers_[peer_id];
                    send_enet_packet(peer.peer, packet);
                }
            } break;
            case TargetKind::Broadcast: {
                for (auto& [id, peer] : peers_) {
                    send_enet_packet(peer.peer, packet);
                }
            } break;
        }
    }
}

void NetManager::on_peer_connect(ENetPeer* peer)
{
    if (!peer)
        log(Warn, "on_peer_connect: Bad ENet peer");

    std::scoped_lock guard{ mutex_ };

    NetPeer npeer;
    npeer.peer = peer;

    char ip[64]{};
    if (enet_address_get_host_ip(&peer->address, ip, sizeof(ip)) == 0) {
        npeer.ip   = ip;
        npeer.port = peer->address.port;
    } else {
        log(Warn, "on_peer_connect: Failed to retrieve IP ");
    }

    npeer.id   = id;
    peers_[id] = std::move(npeer);

    if (client_join_delegate_)
        client_join_delegate_(peers_[id]);
}

void NetManager::on_peer_disconnect(ENetPeer* peer)
{
    if (!peer)
        log(Warn, "on_peer_disconnect: Bad ENet peer");

    std::scoped_lock guard{ mutex_ };

    NetId    id    = NetId{ *(u64*)peer->data };
    NetPeer& npeer = peers_[id];

    if (client_leave_delegate_)
        client_leave_delegate_(npeer);

    log(Info, "({}:{}) disconnected", npeer.ip, npeer.port);
    peers_.erase(id);
}

void NetManager::on_peer_receive(ENetPeer* peer, ENetPacket* packet)
{
    if (!peer || !packet)
        log(Warn, "on_peer_disconnect: Bad ENet peer or Packet");

    std::scoped_lock guard{ mutex_ };

    for (const auto& [peer_id, npeer] : peers_) {
        if (npeer.peer == peer) {
            IncomingPacket npacket;
            npacket.source = peer_id;
            npacket.type   = *(PacketType*)packet->data;
            npacket.data.resize(packet->dataLength - sizeof(PacketType));
            std::memcpy(npacket.data.data() + sizeof(PacketType), packet->data,
                        packet->dataLength - sizeof(PacketType));

            log(Info, "Received packet of type: {}, len: {}", enum_name(npacket.type), npacket.data.size());
            in_queue_.push(std::move(npacket));
        }
    }
}
