#pragma once

#include "Hazel/Networking/NetworkingDefs.h"
#include "Hazel/Core/MemoryBuffer.h"

namespace Hazel
{
	class BaseNetPacket
	{
	public:	
		virtual const MemoryType* GetData() const = 0;
		virtual void* GetPackedData() const = 0;
		virtual size_t GetPackedSize() const = 0;
		virtual size_t GetHeadroom() const = 0;
		virtual NetMsgType::Type GetType() const = 0;
		virtual ClientId::Type GetRecipient() const = 0;
		virtual ClientId::Type GetSender() const = 0;

	protected:
		BaseNetPacket() = default;
	};

	// --------------------------------------

	template <uint32_t MaxSize /*= 64*/>
	class NetPacket : public BaseNetPacket
	{
		friend class Networking;

	public:
		NetPacket() = delete;
		NetPacket(const NetPacket&) = delete;
		NetPacket(const NetPacket&&) = delete;

		NetPacket(const ENetPacket* packet)
		{
			m_Type = *(NetMsgType::Type*)(packet->data + 0);
			m_To = *(ClientId::Type*)(packet->data + sizeof(NetMsgType::Type));
			m_From = *(ClientId::Type*)(packet->data + sizeof(NetMsgType::Type) + sizeof(ClientId::Type));

			// Copy data, if existent
			int dataLength = (signed)packet->dataLength - (signed)StreamStart;
			if (dataLength > 0)
				m_Buffer.Push(packet->data + StreamStart, (uint32_t)packet->dataLength - StreamStart);
		}

		NetPacket(NetMsgType::Type type, ClientId::Type to) :
			m_Type(type), m_To(to) {}

		NetPacket(void* data, size_t size, NetMsgType::Type type, ClientId::Type to) :
			m_Type(type), m_To(to)
		{
			Push(data, size);
		}

		template <typename ...Args>
		inline void Push(Args&& ...args) { m_Buffer.Push(std::forward<Args>(args)...); }

		inline const MemoryBuffer<MaxSize>& GetBuffer() const { return m_Buffer; }

		inline const MemoryType* GetData() const override { return m_Buffer.GetStream(); }
		inline void* GetPackedData() const override { return (void*)&m_Type; }
		inline size_t GetPackedSize() const override { return StreamStart + m_Buffer.GetPointer(); }
		inline size_t GetHeadroom() const override { return m_Buffer.GetHeadroom(); }
		inline NetMsgType::Type GetType() const override { return m_Type; }
		inline ClientId::Type GetRecipient() const override { return m_To; }
		inline ClientId::Type GetSender() const override { return m_From; }

		//inline MemoryBuffer<MaxSize>& operator -> () { return m_Buffer; }

		static constexpr uint32_t StreamStart = 2 * sizeof(ClientId::Type) + sizeof(NetMsgType::Type);

	private:
		NetMsgType::Type m_Type;
		ClientId::Type m_To, m_From = ClientId::Invalid;

		MemoryBuffer<MaxSize> m_Buffer;
	};
}
