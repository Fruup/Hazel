#pragma once

#include "Hazel/Events/Event.h"

namespace Hazel
{
	class GamepadConnectedEvent : public Event
	{
	public:
		GamepadConnectedEvent() = delete;
		GamepadConnectedEvent(int jid) :
			m_jid(jid) {}

		inline int GetJID() const { return m_jid; }

		EVENT_CLASS_TYPE(GamepadConnected)
		EVENT_CLASS_CATEGORY(EventCategoryGamepad)

	private:
		int m_jid;
	};

	class GamepadDisconnectedEvent : public Event
	{
	public:
		GamepadDisconnectedEvent() = delete;
		GamepadDisconnectedEvent(int jid) :
			m_jid(jid) {}

		inline int GetJID() const { return m_jid; }

		EVENT_CLASS_TYPE(GamepadDisconnected)
		EVENT_CLASS_CATEGORY(EventCategoryGamepad)

	private:
		int m_jid;
	};
}
