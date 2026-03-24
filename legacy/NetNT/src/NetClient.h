#ifndef NETLIB_NETCLIENT_H
#define NETLIB_NETCLIENT_H

#include "NetCommon.h"
#include "NetPacket.h"
#include "NetQueue.h"
#include "NetConnection.h"

namespace Codex {
	namespace Net {
		template<typename T>
		class NetClient
		{
		private:
			asio::io_context m_Context;
			asio::ip::tcp::socket m_Socket;
			std::thread m_Thread;
			uint64_t m_ID;
			NetQueue<NetOwnedPacket<T>> m_InQueue;
			std::unique_ptr<NetConnection<T>> m_Server;

		public:
			NetClient() : m_Socket(m_Context)
			{
				m_ID = 0;
			}

		public:
			void Connect(const std::string& host, uint16_t port)
			{
				asio::ip::tcp::resolver resolver(m_Context);
				auto eps = resolver.resolve(host, std::to_string(port));
				this->Connect(eps);
			}
			void Connect(const asio::ip::tcp::resolver::results_type& eps)
			{
				try
				{
					asio::connect(m_Socket, eps);
					this->OnConnected();
					m_Server = std::unique_ptr<NetConnection<T>>(new NetConnection<T>(
						NetConnection<T>::NetOwnerType::Client,
						m_ID,
						std::move(m_Socket),
						m_InQueue
					));
					m_Thread = std::thread([this]() { m_Context.run(); });
					m_Server->HandleNetServer(this);
				}
				catch (std::exception& ex)
				{
					std::cout << ex.what() << std::endl;
					m_Socket.close();
				}
				/*asio::async_connect(m_Socket, eps,
					[this](asio::error_code ec, asio::ip::tcp::endpoint ep)
					{
						if (!ec)
						{
							this->OnConnected();
							m_Server = std::unique_ptr<NetConnection<T>>(new NetConnection<T>(
								NetConnection<T>::NetOwnerType::Client,
								m_ID,
								std::move(m_Socket),
								m_InQueue
								));
							m_Server->HandleNetClient();
						}
						else
						{
							std::cout << ec.message() << std::endl;
							m_Socket.close();
						}
					});*/
			}
			void Send(const NetPacket<T>& packet)
			{
				m_Server->Send(packet);
			}
			void Update()
			{
				//if (!m_Server->IsConnected())
				//	this->OnDisconnected();

				while (!m_InQueue.IsEmpty())
				{
					auto owned_packet = m_InQueue.PopFront();
					this->OnReceive(owned_packet.packet);
				}
			}

		protected:
			virtual void OnConnected()
			{

			}
			virtual void OnReceive(const NetPacket<T>& packet)
			{

			}
			virtual void OnDisconnected()
			{

			}

		public:
			virtual void OnValidated()
			{

			}
		};
	}
}

#endif // NETLIB_NETCLIENT_H