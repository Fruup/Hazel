#include "NetworkingLayer.h"

#include <imgui/imgui.h>

NetworkingLayer::NetworkingLayer() :
	Layer("Networking")
{
	Hazel::Networking::SetVersion(1);
	strcpy_s(m_Address, "127.0.0.1:1234");

	srand(262362346);
}

void NetworkingLayer::OnAttach()
{
	Hazel::RenderCommand::SetClearColor(glm::vec4(.1f));

	Hazel::Application::Get().GetWindow().SetSize(300, 300);
}

void NetworkingLayer::OnDetach()
{
}

void NetworkingLayer::OnUpdate(Hazel::Timestep time)
{
	// Clear
	Hazel::RenderCommand::Clear();

	// Render
	Hazel::Renderer2D::BeginScene(Hazel::OrthographicCameraController(Hazel::Application::Get().GetWindow().GetHeight() / Hazel::Application::Get().GetWindow().GetWidth()).GetCamera());

	const float s = .1f;
	const float speed = .5f;

	for (auto& p : m_Players)
	{
		// Move
		if (p.input[0]) p.obj.y += speed * time;
		if (p.input[1]) p.obj.x -= speed * time;
		if (p.input[2]) p.obj.y -= speed * time;
		if (p.input[3]) p.obj.x += speed * time;

		// Render
		Hazel::Renderer2D::DrawQuad(
			glm::vec2(p.obj.x, p.obj.y) - glm::vec2(.5f * s),
			glm::vec2(s),
			p.color);
	}

	Hazel::Renderer2D::EndScene();
}

void NetworkingLayer::OnEvent(Hazel::Event& event)
{
	if (Hazel::Networking::IsConnected())
	{
		Hazel::EventDispatcher d(event);

		d.Dispatch<Hazel::ReceivedNetMessageEvent>(HZ_BIND_EVENT_FN(NetworkingLayer::MsgReceived));
		d.Dispatch<Hazel::PeerConnectedEvent>(HZ_BIND_EVENT_FN(NetworkingLayer::OnPeerConnected));
		d.Dispatch<Hazel::PeerDisconnectedEvent>(HZ_BIND_EVENT_FN(NetworkingLayer::OnPeerDisconnected));

		d.Dispatch<Hazel::KeyPressedEvent>(HZ_BIND_EVENT_FN(NetworkingLayer::KeyDown));
		d.Dispatch<Hazel::KeyReleasedEvent>(HZ_BIND_EVENT_FN(NetworkingLayer::KeyUp));
	}
}

bool NetworkingLayer::OnPeerConnected(Hazel::PeerConnectedEvent& e)
{
	m_Players.push_back(Player(e.GetPeer().Id));

	return false;
}

bool NetworkingLayer::OnPeerDisconnected(Hazel::PeerDisconnectedEvent& e)
{
	for (auto it = m_Players.begin(); it != m_Players.end(); ++it)
	{
		if (it->peerId == e.GetPeerId())
		{
			m_Players.erase(it);
			return false;
		}
	}

	return false;
}

bool NetworkingLayer::MsgReceived(Hazel::ReceivedNetMessageEvent& e)
{
	auto packet = e.GetPacket();

	if (packet->GetType() == 0)
	{
		Player& player = FindPlayerByPeerId(e.GetPacket()->GetSender());

		for (int i = 0; i < 4; i++)
			packet->Read(&player.input[i]);
		packet->Read(player.obj);
	}

	return false;
}

void NetworkingLayer::SendMsg()
{
	Hazel::NetPacket<32> packet(0, Hazel::PeerId::All);

	const Player& player = FindThisPlayer();

	for (int i = 0; i < 4; ++i)
		packet.Write(player.input[i]);
	packet.Write(player.obj);

	Hazel::Networking::QueuePacket(packet);
}

bool NetworkingLayer::KeyDown(Hazel::KeyPressedEvent& e)
{
	auto input = FindThisPlayer().input;

	switch (e.GetKeyCode())
	{
		case Hazel::KeyCode::W: input[0] = true; break;
		case Hazel::KeyCode::A: input[1] = true; break;
		case Hazel::KeyCode::S: input[2] = true; break;
		case Hazel::KeyCode::D: input[3] = true; break;
		default:
			return false;
	}

	SendMsg();

	return false;
}

bool NetworkingLayer::KeyUp(Hazel::KeyReleasedEvent& e)
{
	auto input = FindThisPlayer().input;

	switch (e.GetKeyCode())
	{
		case Hazel::KeyCode::W: input[0] = false; break;
		case Hazel::KeyCode::A: input[1] = false; break;
		case Hazel::KeyCode::S: input[2] = false; break;
		case Hazel::KeyCode::D: input[3] = false; break;
		default:
			return false;
	}

	SendMsg();

	return false;
}

void NetworkingLayer::OnImGuiRender()
{
	if (!Hazel::Networking::IsConnected())
	{
		ImGui::DockSpaceOverViewport();

		ImGui::Begin("Networking", 0, ImGuiWindowFlags_NoMove);

		ImGui::InputTextWithHint("", "address", m_Address, 32);

		ImGui::Separator();

		if (ImGui::Button("Start server"))
		{
			enet_uint16 port;
			if (std::string(m_Address).find(':') != std::string::npos)
				port = Hazel::NetAddress(m_Address)->port;
			else port = (enet_uint16)std::atoi(m_Address);

			Hazel::Networking::Server::Start(1, port);
		}

		if (ImGui::Button("Connect to server"))
			Hazel::Networking::Client::Connect(m_Address, 3000);

		ImGui::Text("Status:");
		ImGui::Text(Hazel::Networking::GetStateString());

		ImGui::End();
	}
}
