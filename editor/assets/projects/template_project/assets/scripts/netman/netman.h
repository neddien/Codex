#pragma once

#include <enet/enet.h>
#include <modex.h>

using namespace codex;

CX_CUSTOM_EXCEPTION(NetworkException, "A network error occured");

template <typename T>
    requires(std::is_integral_v<T>)
constexpr void write(std::vector<u8>& buf, T data) noexcept
{
    const usize prev_size = buf.size();
    buf.resize(prev_size + sizeof(data));
    std::memcpy(buf.data() + prev_size, &data, sizeof(data));
}

constexpr void write_bytes(std::vector<u8>& buf, std::span<const u8> data) noexcept
{
    const usize prev_size = buf.size();
    buf.resize(prev_size + data.size());
    std::memcpy(buf.data() + prev_size, data.data(), data.size());
}

enum class PacketType : u16
{
    None,
    EntityCreate,
    EntityDestroy,
    EntitySynchronize,
    ClientJoin,
    ClientLeave,
};

enum class TargetKind : u8
{
    Unicast,
    Multicast,
    Broadcast,
};

using NetId = UUID;

struct IncomingPacket
{
    NetId           source = NetId{ 0 };
    PacketType      type;
    std::vector<u8> data;
};

struct OutgoingPacket
{
    std::vector<NetId> targets;
    PacketType         type     = PacketType::None;
    TargetKind         kind     = TargetKind::Unicast;
    u32                channel  = 0;
    bool               reliable = true;
    std::vector<u8>    data;
};

class NetManager : private Loggable<"NetworkManager">
{
public:
    enum class Mode
    {
        Server,
        Client,
    };
    struct NetPeer
    {
        NetId       id;
        ENetPeer*   peer;
        std::string ip;
        u16         port;
    };
    using OnClientAction = std::function<void(const NetPeer&)>;

public:
    NetManager(Mode mode, std::string_view ip = "127.0.0.1", u16 port = 7878, u16 client_count = 32);
    ~NetManager();

public:
    void attach_on_client_join(OnClientAction delegate) const noexcept
    {
        /* clang-format: no inline */
        client_join_delegate_ = std::move(delegate);
    }
    void attach_on_client_leave(OnClientAction delegate) const noexcept
    {
        /* clang-format: no inline */
        client_leave_delegate_ = std::move(delegate);
    }

public:
    void                          run();
    void                          replicate_entity(Entity entity);
    void                          update_entities();
    bool                          send(OutgoingPacket&& packet);
    std::optional<IncomingPacket> dequeue();

private:
    void initialize_as_server(std::string_view ip, u16 port);
    void initialize_as_client(std::string_view ip, u16 port);
    void run_as_server();
    void run_as_client();
    bool send_enet_packet(ENetPeer* peer, const OutgoingPacket& packet);
    void drain_outgoing_queue();

private:
    void on_peer_connect(ENetPeer* peer);
    void on_peer_disconnect(ENetPeer* peer);
    void on_peer_receive(ENetPeer* peer, ENetPacket* packet);

private:
    ENetAddress                        address_;
    ENetHost*                          host_;
    Mode                               mode_;
    u16                                client_count_;
    std::unordered_map<NetId, NetPeer> peers_;
    std::queue<OutgoingPacket>         out_queue_;
    std::queue<IncomingPacket>         in_queue_;
    mutable OnClientAction             client_join_delegate_;
    mutable OnClientAction             client_leave_delegate_;
    mutable std::recursive_mutex       mutex_;
};
