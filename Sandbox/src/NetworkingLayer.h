#pragma once

#include <Hazel.h>

#include <Hazel/Core/Serialize.h>

#include "Hazel/Events/NetEvent.h"
#include "Hazel/Events/KeyEvent.h"

struct GameObject : public Hazel::Serializable<GameObject>
{
	GameObject() : Serializable(x, y) {}

	float x = 0.0f, y = 0.0f;
};

struct Player
{
	bool input[4];
	GameObject obj;
	Hazel::PeerId::Type peerId = Hazel::PeerId::Invalid;
	glm::vec4 color;

	Player(Hazel::PeerId::Type id) :
		input(),
		obj(),
		peerId(id),
		color(
			(float)(rand() % 100) * .01f,
			(float)(rand() % 100) * .01f,
			(float)(rand() % 100) * .01f,
			1.0f)
	{
		for (int i = 0; i < 4; ++i)
			input[i] = false;
	}
};

class NetworkingLayer : public Hazel::Layer
{
public:
	NetworkingLayer();

	void OnAttach() override;
	void OnDetach() override;

	void OnUpdate(Hazel::Timestep time) override;

	void OnEvent(Hazel::Event& event) override;

	void OnImGuiRender() override;

private:
	void SendMsg();

	bool MsgReceived(Hazel::ReceivedNetMessageEvent& e);
	bool OnPeerConnected(Hazel::PeerConnectedEvent& e);
	bool OnPeerDisconnected(Hazel::PeerDisconnectedEvent& e);
	bool KeyDown(Hazel::KeyPressedEvent& e);
	bool KeyUp(Hazel::KeyReleasedEvent& e);

	Player& FindPlayerByPeerId(Hazel::PeerId::Type id)
	{
		for (auto& p : m_Players)
			if (p.peerId == id)
				return p;

		assert(false);
		return Player(0);
	}

	inline Player& FindThisPlayer()
	{
		return FindPlayerByPeerId(Hazel::Networking::GetThisPeerId());
	}

	char m_Address[32];

	std::vector<Player> m_Players;
};
