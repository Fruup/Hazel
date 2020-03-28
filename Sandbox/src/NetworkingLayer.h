#pragma once

#include <Hazel.h>

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
	char m_Address[32];
	std::string m_Status = "idle";
};
