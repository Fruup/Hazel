#pragma once

#include "Hazel/Networking/NetworkingDefs.h"
#include "Hazel/Core/MemoryBuffer.h"

namespace Hazel
{
	//class Networking;

	class BaseNetPacket
	{
		//friend static void Networking::QueuePacket(const BaseNetPacket& _1, enet_uint8 _2 = 0, enet_uint32 _3 = 0);
		friend class Networking;

	public:
		BaseNetPacket(NetMsgType::Type type, PeerId::Type recipient, uint32_t capacity) :
			m_Buffer(capacity),
			m_Type(*reinterpret_cast<NetMsgType::Type*>(m_Buffer.GetData() + 0)),
			m_To(*reinterpret_cast<PeerId::Type*>(m_Buffer.GetData() + sizeof(NetMsgType::Type))),
			m_From(*reinterpret_cast<PeerId::Type*>(m_Buffer.GetData() + sizeof(NetMsgType::Type) + sizeof(PeerId::Type)))
		{
			SetHeader(type, recipient);
			SetPointers();
		}
		BaseNetPacket(const ENetPacket* packet) :
			m_Buffer((uint32_t)packet->dataLength),
			m_Type(*reinterpret_cast<NetMsgType::Type*>(m_Buffer.GetData() + 0)),
			m_To(*reinterpret_cast<PeerId::Type*>(m_Buffer.GetData() + sizeof(NetMsgType::Type))),
			m_From(*reinterpret_cast<PeerId::Type*>(m_Buffer.GetData() + sizeof(NetMsgType::Type) + sizeof(PeerId::Type)))
		{
			// Copy data
			m_Buffer.Write((void*)packet->data, packet->dataLength);
			SetPointers();
		}

		// Forward read/write commands to internal buffer
		template <typename ...Args>
		inline void Write(Args&& ...args) { m_Buffer.Write(args...); }

		template <typename ...Args>
		inline void Read(Args&& ...args) { m_Buffer.Read(args...); }

		// Getters
		inline BaseMemoryBuffer& GetBuffer() { return m_Buffer; }
		inline void* GetPackedData() const { return (void*)m_Buffer.GetData(); }
		inline MemoryType* GetData() const { return m_Buffer.GetData() + DataStart; }
		inline uint32_t GetDataSize() const { return m_Buffer.GetSize() - DataStart; }
		inline uint32_t GetPackedSize() const { return m_Buffer.GetSize(); }
		inline uint32_t GetHeadroom() const { return m_Buffer.GetHeadroom(); }
		inline NetMsgType::Type GetType() const { return m_Type; }
		inline PeerId::Type GetRecipient() const { return m_To; }
		inline PeerId::Type GetSender() const { return m_From; }

	private:
		/* This member order is important! Buffer has to be initialized first. */

		BaseMemoryBuffer m_Buffer;

		NetMsgType::Type& m_Type;
		PeerId::Type& m_To;
		PeerId::Type& m_From;

		static constexpr uint32_t DataStart = 2 * sizeof(PeerId::Type) + sizeof(NetMsgType::Type);

		// Helpers
		inline void SetPointers()
		{
			m_Buffer.SetWritePointer(DataStart);
			m_Buffer.SetReadPointer(DataStart);
		}
		inline void BaseNetPacket::SetHeader(NetMsgType::Type type, PeerId::Type recipient)
		{
			// Set packet header
			m_Type = type;
			m_To = recipient;
			//m_From = Networking::GetThisPeerId();
			m_From = PeerId::Invalid;	// This is set in Networking::QueuePacket
		}

	protected:
		BaseNetPacket(NetMsgType::Type type, PeerId::Type recipient, MemoryType* data, uint32_t capacity, bool delete_ = false) :
			m_Buffer(data, capacity, delete_),
			m_Type(*reinterpret_cast<NetMsgType::Type*>(m_Buffer.GetData() + 0)),
			m_To(*reinterpret_cast<NetMsgType::Type*>(m_Buffer.GetData() + sizeof(NetMsgType::Type))),
			m_From(*reinterpret_cast<NetMsgType::Type*>(m_Buffer.GetData() + sizeof(NetMsgType::Type) + sizeof(PeerId::Type)))
		{
			SetHeader(type, recipient);
			SetPointers();
		}
		BaseNetPacket(MemoryType* data, uint32_t capacity, const ENetPacket* packet) :
			m_Buffer(data, capacity),
			m_Type(*reinterpret_cast<NetMsgType::Type*>(m_Buffer.GetData() + 0)),
			m_To(*reinterpret_cast<PeerId::Type*>(m_Buffer.GetData() + sizeof(NetMsgType::Type))),
			m_From(*reinterpret_cast<PeerId::Type*>(m_Buffer.GetData() + sizeof(NetMsgType::Type) + sizeof(PeerId::Type)))
		{
			// Copy data
			m_Buffer.Write(packet->data, packet->dataLength);
			SetPointers();
		}
	};

	// --------------------------------------

	template <uint32_t Capacity>
	class NetPacket : public BaseNetPacket
	{
		friend class Networking;

	public:
		NetPacket() = delete;
		NetPacket(const NetPacket&) = delete;
		NetPacket(const NetPacket&&) = delete;

		inline NetPacket(NetMsgType::Type type, PeerId::Type recipient) :
			BaseNetPacket(type, recipient, m_Stream, Capacity) {}

		inline NetPacket(const ENetPacket* packet) :
			BaseNetPacket(m_Stream, Capacity, packet) {}

		inline NetPacket(NetMsgType::Type type, PeerId::Type recipient, void* data, uint32_t size) :
			BaseNetPacket(type, recipient, Capacity)
		{
			Write(data, size);
		}

	private:
		MemoryType m_Stream[Capacity];
	};
}
