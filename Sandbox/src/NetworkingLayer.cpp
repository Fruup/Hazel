#include "NetworkingLayer.h"

#include <Hazel/Events/NetEvent.h>
#include <Hazel/Events/KeyEvent.h>
#include <imgui/imgui.h>

NetworkingLayer::NetworkingLayer() :
	Layer("Networking")
{
	Hazel::Networking::SetVersion(1);
	strcpy_s(m_Address, "127.0.0.1:1234");
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

	Hazel::Renderer2D::DrawQuad(glm::vec2(m_Data.x, m_Data.y), glm::vec2(.05f), glm::vec4(0.2f, 1.0f, 0.2f, 1.0f));

	Hazel::Renderer2D::EndScene();
}

void NetworkingLayer::OnEvent(Hazel::Event& event)
{
	if (event.GetEventType() == Hazel::ConnectedToServerEvent::GetStaticType())
	{
		m_Status = "Connected.";
	}
	else if (event.GetEventType() == Hazel::DisconnectedFromServerEvent::GetStaticType())
	{
		m_Status = "idle";
	}

	else if (event.GetEventType() == Hazel::ReceivedNetMessageEvent::GetStaticType())
	{
		auto e = dynamic_cast<Hazel::ReceivedNetMessageEvent*>(&event);
		m_Data.Deserialize((Hazel::MemoryType*)e->GetPacket()->GetData());
	}

	if (Hazel::Networking::IsConnected() && event.GetEventType() == Hazel::MouseMovedEvent::GetStaticType())
	{
		//if (Hazel::Networking::IsServer())
		{
			auto e = dynamic_cast<Hazel::MouseMovedEvent*>(&event);
			const float w = Hazel::Application::Get().GetWindow().GetWidth();
			const float h = Hazel::Application::Get().GetWindow().GetHeight();

			float x = e->GetX();
			float y = e->GetY();

			if (x > 0 && x < w &&
				y > 0 && y < h)
			{
				SomeData data;
				data.x = 2.0f * (x - 150.0f) / w;
				data.y = 2.0f * (y - 150.0f) / h;

				// Send message
				Hazel::NetPacket<16> packet(42, Hazel::ClientId::All);
				packet.Push(data);

				Hazel::Networking::QueuePacket(packet);
			}
		}
	}
}

void NetworkingLayer::OnImGuiRender()
{
	if (Hazel::Networking::IsConnected())
	{

	}
	else
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

			Hazel::Networking::Server::Start(2, port);

			if (Hazel::Networking::IsConnected())
				m_Status = "Server ready.";
		}

		if (ImGui::Button("Connect to server"))
		{
			Hazel::Networking::Client::Connect(Hazel::NetAddress(m_Address), 3000);
			m_Status = "Connecting to server...";
		}

		ImGui::Text("Status:");
		ImGui::Text(Hazel::Networking::GetStateString());

		ImGui::End();
	}
}
