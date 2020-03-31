#pragma once

#include <Hazel.h>

#include <Hazel/Core/Serialize.h>

struct SomeData : public Hazel::Serializable<2>
{
	SomeData() : Serializable(x, y) {}

	float x = 0.0f, y = 0.0f;
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
	char m_Address[32];

	SomeData m_Data;
};
