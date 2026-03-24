#ifndef NETLIB_NETSERVER_H
#define NETLIB_NETSERVER_H

#include "NetCommon.h"
#include "NetPacket.h"
#include "NetQueue.h"
#include "NetConnection.h"

namespace Codex {
	namespace Net {
		template<typename T>
		class NetServer
		{
		private:
			uint16_t m_Port;
			uint64_t m_IDCount;
			asio::io_context m_Context;
			std::thread m_Thread;
			asio::ip::tcp::acceptor m_Listener;
			std::vector<std::shared_ptr<NetConnection<T>>> m_Clients;
			NetQueue<NetOwnedPacket<T>> m_InQueue;
			NetQueue<NetPacket<T>> m_OutQueue;

		public:
			NetServer(uint16_t port) : m_Port(port), m_Listener(asio::ip::tcp::acceptor(m_Context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), m_Port)))
			{
				m_IDCount = 0;
			}

		public:
			void WaitForConnection()
			{
				m_Listener.async_accept(
					[this](asio::error_code ec, asio::ip::tcp::socket socket)
					{
						if (!ec)
						{
							std::cout << "Client connected\n";

							auto conn = std::make_shared<NetConnection<T>>(
								NetConnection<T>::NetOwnerType::Server, m_IDCount++, std::move(socket), m_InQueue);

							if (this->OnClientConnected(conn))
							{
								m_Clients.push_back(conn);
								conn->HandleNetClient(this);
							}
							else
							{
								std::cout << "Connection refused.\n";
								socket.close();
							}
						}
						else
						{
							std::cout << ec.message() << std::endl;
						}
						this->WaitForConnection();
					});
			}
			void Start()
			{
				this->WaitForConnection();
				m_Thread = std::thread([this]() { m_Context.run(); });
				std::cout << "Server started.\n";
			}
			void Send(std::weak_ptr<NetConnection<T>> client, const NetPacket<T>& packet)
			{

			}
			void Update()
			{
				//m_Clients.

				while (!m_InQueue.IsEmpty())
				{
					auto owned_packet = m_InQueue.PopFront();
					this->OnReceive(owned_packet.remote, owned_packet.packet);
				}
			}

		protected:
			virtual bool OnClientConnected(std::weak_ptr<NetConnection<T>> client) // TODO: Use a weak_ptr
			{
				return false;
			}
			virtual void OnClientDisconnected(std::weak_ptr<NetConnection<T>> client)
			{

			}
			virtual void OnReceive(std::weak_ptr<NetConnection<T>> client, const NetPacket<T>& packet)
			{

			}

		public:
			virtual void OnClientValidated(std::weak_ptr<NetConnection<T>> client)
			{

			}
		};
	}
}

#endif // NETLIB_NETSERVER_H