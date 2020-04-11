#pragma once

#include "Hazel/Networking/NetworkingDefs.h"
#include "Hazel/Networking/NetPacket.h"
#include "Hazel/Events/Event.h"

namespace Hazel
{
	class NetEvent : public Event
	{
	public:
		EVENT_CLASS_CATEGORY(EventCategoryNetwork)
	protected:
		NetEvent() = default;
	};

	class PeerConnectedEvent : public NetEvent
	{
	public:
		PeerConnectedEvent(const NetPeerInfo& peer) : m_Peer(peer) {}
		EVENT_CLASS_TYPE(PeerConnected)

			inline const NetPeerInfo& GetPeer() const { return m_Peer; }

	private:
		const NetPeerInfo& m_Peer;
	};

	class PeerDisconnectedEvent : public NetEvent
	{
	public:
		PeerDisconnectedEvent(PeerId::Type id) : m_PeerId(id) {}
		EVENT_CLASS_TYPE(PeerDisconnected)

		inline PeerId::Type GetPeerId() const { return m_PeerId; }

	private:
		PeerId::Type m_PeerId;
	};

	class ReceivedNetMessageEvent : public NetEvent
	{
	public:
		ReceivedNetMessageEvent(const Ref<BaseNetPacket>& packet) :
			m_Packet(packet) {}

		inline const Ref<BaseNetPacket>& GetPacket() const { return m_Packet; }

		EVENT_CLASS_TYPE(ReceivedNetMessage)

	private:
		Ref<BaseNetPacket> m_Packet;
	};
}
