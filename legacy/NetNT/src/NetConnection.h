#ifndef NETLIB_NETCONNECTION_H
#define NETLIB_NETCONNECTION_H

#include "NetCommon.h"
#include "NetQueue.h"
#include "NetPacket.h"

namespace Codex {
    namespace Net {
        // Forward declerations
        template<typename T>
        class NetServer;
        template<typename T>
        class NetClient;

        template <typename T>
        class NetConnection : public std::enable_shared_from_this<NetConnection<T>>
        {
        public:
            enum class NetOwnerType : uint8_t
            {
                Server,
                Client
            };

        private:
            NetOwnerType m_connType;
            uint64_t m_ID;
            asio::io_context m_Context;
            asio::ip::tcp::socket m_Socket;
            NetQueue<NetOwnedPacket<T>> &m_InQueue;
            NetQueue<NetPacket<T>> m_OutQueue;
            NetPacket<T> m_TempPacket;
            uint64_t m_HandshakeIn;
            uint64_t m_HandshakeOut;
            bool m_Validated;

        public:
            NetConnection(NetOwnerType type, uint64_t id, asio::ip::tcp::socket socket, NetQueue<NetOwnedPacket<T>> &inQueue) : m_connType(type), m_ID(id), m_Socket(std::move(socket)), m_InQueue(inQueue)
            {
                m_Validated = false;

                if (type == NetOwnerType::Server)
                {
                    std::random_device rd;
                    std::mt19937_64 gen(rd());
                    m_HandshakeOut = gen();
                }
                else
                {
                    m_HandshakeOut = 0;
                    m_HandshakeIn = 0;
                }
            }

        public:
            inline bool IsConnected() const { return m_Socket.is_open(); }
            inline uint64_t GetID() const   { return m_ID; }

        public:
            uint64_t Randgen(uint64_t input)
            {
                return (~input << 8) ^ 0xFEEDFAAB00000001 ^ 0xFEEFAAF000000001;
            }
            void AddPacket()
            {
                NetOwnedPacket<T> owned_packet;
                owned_packet.packet = m_TempPacket;
                if (m_connType == NetOwnerType::Server)
                    owned_packet.remote = this->shared_from_this();
                m_InQueue.Push(owned_packet);
            }
            void ReadDataAsync()
            {
                asio::async_read(m_Socket, asio::buffer(m_TempPacket.data.data(), m_TempPacket.header.size),
                        [this](asio::error_code ec, size_t length)
                        {
                            if (!ec)
                            {
                                this->AddPacket();
                                this->ReadHeaderAsync();
                            }
                            else
                            {
                                std::cout << ec.message() << std::endl;
                            }
                        });
            }
            void ReadHeaderAsync()
            {
                asio::async_read(m_Socket, asio::buffer(&m_TempPacket.header, sizeof(NetPacketHeader<T>)),
                        [this](asio::error_code ec, size_t length)
                        {
                            if (!ec)
                            {
                                if (m_TempPacket.header.size > 0)
                                {
                                    m_TempPacket.data.resize(m_TempPacket.header.size);
                                    this->ReadDataAsync();
                                }
                                else
                                    this->AddPacket();
                            }
                            else
                            {
                                std::cout << ec.message() << std::endl;
                                m_Socket.close();
                            }
                        });
            }
            void SendDataAsync()
            {
                auto &packet = m_OutQueue.Front();
                asio::async_write(m_Socket, asio::buffer(packet.data.data(), packet.header.size),
                        [this, &packet](asio::error_code ec, size_t length)
                        {
                            if (!ec)
                            {
                                m_OutQueue.PopFront();
                                if (!m_OutQueue.IsEmpty())
                                    this->SendHeaderAsync();
                            }
                            else
                            {
                                std::cout << ec.message() << std::endl;
                                m_Socket.close();
                            }
                        });
            }
            void SendHeaderAsync()
            {
                asio::async_write(m_Socket, asio::buffer(&m_OutQueue.Front().header, sizeof(NetPacketHeader<T>)),
                        [this](asio::error_code ec, size_t length)
                        {
                            if (!ec)
                            {
                                if (m_OutQueue.Front().header.size > 0)
                                    this->SendDataAsync();
                            }
                            else
                            {
                                std::cout << ec.message() << std::endl;
                                m_Socket.close();
                            }
                        });
            }
            void SendValidationAsync(NetClient<T>* client = nullptr)
            {
                asio::async_write(m_Socket, asio::buffer(&m_HandshakeOut, sizeof(m_HandshakeOut)),
                        [this, client](asio::error_code ec, size_t length)
                        {
                            if (!ec)
                            {
                                if (m_connType == NetOwnerType::Client)
                                    this->ConfirmValidationAsync(client);
                            }
                            else
                            {
                                std::cout << ec.message() << std::endl;
                                m_Socket.close();
                            }
                        });
            }
            void ReadValidationAsync(NetServer<T>* server = nullptr, NetClient<T>* client = nullptr)
            {
                asio::async_read(m_Socket, asio::buffer(&m_HandshakeIn, sizeof(m_HandshakeIn)),
                        [this, server, client](asio::error_code ec, size_t length)
                        {
                            if (!ec)
                            {
                                if (m_connType == NetOwnerType::Server)
                                {
                                    uint64_t handshake_to_cmp_to = this->Randgen(m_HandshakeOut);
                                    if (m_HandshakeIn == handshake_to_cmp_to)
                                    {
                                        m_Validated = true;
                                        std::cout << "Client validated.\n";
                                        this->ConfirmValidationAsync(nullptr, server);
                                    }
                                    else
                                    {
                                        this->ConfirmValidationAsync(client);
                                        std::cout << "Client failed validation.\n";
                                        std::cout << "Expected handshake: " << handshake_to_cmp_to << " but got: " << m_HandshakeIn << std::endl;
                                        m_Socket.close();
                                    }
                                }
                                else
                                {
                                    m_HandshakeOut = this->Randgen(m_HandshakeIn);
                                    this->SendValidationAsync(client);
                                }
                            }
                            else
                            {
                                std::cout << ec.message() << std::endl;
                                m_Socket.close();
                            }
                        });
            }
            void ConfirmValidationAsync(NetClient<T>* client = nullptr, NetServer<T>* server = nullptr)
            {
                if (m_connType == NetOwnerType::Server)
                {
                    asio::async_write(m_Socket, asio::buffer(&m_Validated, sizeof(bool)),
                        [this, server](asio::error_code ec, size_t length)
                        {
                            if (!ec)
                            {
                                if (m_Validated)
                                {
                                    this->ReadHeaderAsync();
                                    if (server)
                                        server->OnClientValidated(this->shared_from_this());
                                }
                                else
                                    m_Socket.close();
                            }
                            else
                            {
                                std::cout << ec.message() << std::endl;
                                m_Socket.close();
                            }
                        });
                }
                else
                {
                    asio::async_read(m_Socket, asio::buffer(&m_Validated, sizeof(bool)),
                        [this, client](asio::error_code ec, size_t length)
                        {
                            if (m_Validated)
                            {
                                if (client)
                                    client->OnValidated();
                                this->ReadHeaderAsync();
                            }
                            else
                                m_Socket.close();
                        });
                }
            }
            void HandleNetClient(NetServer<T>* server = nullptr)
            {
                if (m_connType == NetOwnerType::Server)
                {
                    this->SendValidationAsync();
                    this->ReadValidationAsync(server);
                }
            }
            void HandleNetServer(NetClient<T>* client = nullptr)
            {
                if (m_connType == NetOwnerType::Client)
                {
                    this->ReadValidationAsync(nullptr, client);
                }
            }
            void Send(const NetPacket<T>& packet)
            {
                if (this->IsConnected())
                {
                    bool busy = !m_OutQueue.IsEmpty();
                    m_OutQueue.Push(packet);
                    if (!busy)
                        this->SendHeaderAsync();
                }
            }
            void Disconnected()
            {
                if (this->IsConnected())
                    m_Socket.close();
            }
        };
    }
}

#endif // NETLIB_NETCONNECTION_H