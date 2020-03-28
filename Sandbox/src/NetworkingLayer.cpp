#include "NetworkingLayer.h"

#include <Hazel/Events/NetEvent.h>
#include <Hazel/Events/KeyEvent.h>
#include <imgui/imgui.h>

NetworkingLayer::NetworkingLayer() :
	Layer("Networking")
{
	strcpy_s(m_Address, "91.67.152.132:1234");
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

	if (event.GetEventType() == Hazel::KeyPressedEvent::GetStaticType())
	{
		if (dynamic_cast<Hazel::KeyPressedEvent*>(&event)->GetKeyCode() == Hazel::KeyCode::Space)
		{
			// Send message
			const char* data = "Hallo wie geht es dir?";

			Hazel::NetPacket packet(42, Hazel::All);
			packet.Push((void*)data, strlen(data) + 1);

			Hazel::Networking::QueuePacket(packet);
		}
	}
}

void NetworkingLayer::OnImGuiRender()
{
	ImGui::Begin("Networking", 0, ImGuiWindowFlags_NoMove);

	if (Hazel::Networking::CheckState(Hazel::Networking::ConnectedState))
	{

	}
	else
	{
		ImGui::InputTextWithHint("", "address", m_Address, 32);

		ImGui::Separator();

		if (ImGui::Button("Start server"))
		{
			Hazel::Networking::Server((enet_uint16)std::atoi(m_Address));

			if (Hazel::Networking::CheckState(Hazel::Networking::ConnectedState))
				m_Status = "Server ready.";
		}

		if (ImGui::Button("Connect to server"))
		{
			Hazel::Networking::Client(Hazel::NetAddress(m_Address), 3000);
			m_Status = "Connecting to server...";
		}
	}

	ImGui::Text("Status:");
	ImGui::Text(m_Status.c_str());

	ImGui::End();
}
