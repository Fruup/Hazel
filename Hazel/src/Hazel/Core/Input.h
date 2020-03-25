#pragma once

#include "Hazel/Core/Core.h"
#include "Hazel/Core/KeyCodes.h"
#include "Hazel/Core/MouseCodes.h"
#include "Hazel/Core/Timestep.h"
#include "Hazel/Input/Gamepad.h"

namespace Hazel {

	class Input
	{
	protected:
		Input() = default;
	public:
		Input(const Input&) = delete;
		Input& operator=(const Input&) = delete;

		inline static bool IsKeyPressed(KeyCode key) { return s_Instance->IsKeyPressedImpl(key); }

		inline static bool IsMouseButtonPressed(MouseCode button) { return s_Instance->IsMouseButtonPressedImpl(button); }
		inline static std::pair<float, float> GetMousePosition() { return s_Instance->GetMousePositionImpl(); }
		inline static float GetMouseX() { return s_Instance->GetMouseXImpl(); }
		inline static float GetMouseY() { return s_Instance->GetMouseYImpl(); }

		inline static void Init() { return s_Instance->InitImpl(); }
		inline static void Shutdown() { return s_Instance->ShutdownImpl(); }
		static void OnUpdate();

		static Ref<Gamepad> FindGamepad(int id);
		inline static const std::vector<Ref<Gamepad>>& GetGamepadList() { return s_Instance->m_Gamepads; }

		static Scope<Input> Create();
	protected:
		virtual bool IsKeyPressedImpl(KeyCode key) = 0;

		virtual bool IsMouseButtonPressedImpl(MouseCode button) = 0;
		virtual std::pair<float, float> GetMousePositionImpl() = 0;
		virtual float GetMouseXImpl() = 0;
		virtual float GetMouseYImpl() = 0;

		virtual void OnUpdateImpl(Timestep time) = 0;

		inline static Ref<Gamepad> CreateGamepad(int id, const char* name) { return s_Instance->CreateGamepadImpl(id, name); }
		virtual Ref<Gamepad> CreateGamepadImpl(int id, const char* name) = 0;

		virtual void InitImpl() = 0;
		virtual void ShutdownImpl() = 0;


		std::vector<Ref<Gamepad>> m_Gamepads;

	private:
		static Scope<Input> s_Instance;
	};
}
