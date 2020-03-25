#pragma once

#include "Hazel/Core/Input.h"

namespace Hazel {

	class WindowsInput : public Input
	{
	protected:
		virtual bool IsKeyPressedImpl(KeyCode key) override;

		virtual bool IsMouseButtonPressedImpl(MouseCode button) override;
		virtual std::pair<float, float> GetMousePositionImpl() override;
		virtual float GetMouseXImpl() override;
		virtual float GetMouseYImpl() override;

		virtual void OnUpdateImpl(Timestep time) override;

		virtual Ref<Gamepad> CreateGamepadImpl(int id, const char* name) override;

	private:
		virtual void InitImpl() override;
		virtual void ShutdownImpl() override;
	};

}
