#ifndef NETLIB_NETPACKET_H
#define NETLIB_NETPACKET_H

#include "NetCommon.h"

namespace Codex::Net {
	// Forward decelerations
	template<typename T>
	class NetConnection;

	template<typename T>
	struct NetPacketHeader
	{
	public:
		T id;
		uint32_t size;
	};

	template<typename T>
	struct NetPacket
	{
	public:
		NetPacketHeader<T> header;
		std::vector<uint8_t> data;

	public:
		NetPacket() = default;
		NetPacket(T id) 
		{ 
			header.id = id;
		}

	public:
		NetPacket<T>& AddBuffer(const uint8_t* buffer, const size_t size)
		{
			size_t prev_size = data.size();
			data.resize(prev_size + size);
			std::memcpy(data.data() + prev_size, buffer, size);
			this->header.size = data.size();
			return *this;
		}

		template<typename U>
		NetPacket<T>& operator << (const U& data)
		{
			static_assert(std::is_standard_layout<U>::value, "Data passed is too complex for serialiaztion.");

			size_t prev_size = this->data.size();
			this->data.resize(prev_size + sizeof(U));
			std::memcpy(this->data.data() + prev_size, &data, sizeof(U));
			this->header.size = this->data.size();
			return *this;
		}

		std::unique_ptr<uint8_t> GetBuffer(const size_t size)
		{
			size_t total_size = this->data.size();
			std::unique_ptr<uint8_t> buffer(new uint8_t[size]);
			std::memcpy(buffer.get(), this->data.data() + (total_size - size), size);
			this->data.resize(total_size - size);
			this->header.size = this->data.size();
			return std::move(buffer);
		}

		template<typename U>
		NetPacket<T>& operator >> (U& data)
		{
			static_assert(std::is_standard_layout<U>::value, "Data passed is too complex for deserialiaztion.");

			size_t total_size = this->data.size();
			std::memcpy(&data, this->data.data() + (total_size - sizeof(U)), sizeof(U));
			this->data.resize(total_size - sizeof(U));
			this->header.size = this->data.size();
			return *this;
		}

		/*template<typename U>
		friend NetPacket<T>& operator << (NetPacket<T>& packet, const U& data)
		{
			static_assert(std::is_standard_layout<U>::value, "Data passed is too complex for serialization.");

			size_t prev_size = packet.data.size();
			packet.data.resize(prev_size + sizeof(data));

			return packet;
		}*/
	};

	template<typename T>
	struct NetOwnedPacket
	{
		std::shared_ptr<NetConnection<T>> remote = nullptr;
		NetPacket<T> packet;
	};
}

#endif // NETLIB_NETPACKET_H