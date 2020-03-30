#pragma once

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

	class ClientConnectedEvent : public NetEvent
	{
	public:
		ClientConnectedEvent() = default;
		EVENT_CLASS_TYPE(ClientConnected)
	};

	class ClientDisconnectedEvent : public NetEvent
	{
	public:
		ClientDisconnectedEvent() = default;
		EVENT_CLASS_TYPE(ClientDisconnected)
	};

	class ConnectedToServerEvent : public NetEvent
	{
	public:
		ConnectedToServerEvent() = default;
		EVENT_CLASS_TYPE(ConnectedToServer)
	};

	class DisconnectedFromServerEvent : public NetEvent
	{
	public:
		DisconnectedFromServerEvent() = default;
		EVENT_CLASS_TYPE(DisconnectedFromServer)
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
