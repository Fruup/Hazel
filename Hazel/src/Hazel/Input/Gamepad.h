#pragma once

#include <string>

namespace Hazel
{
	enum GamepadButton
	{
		GamepadButtonA,
		GamepadButtonB,
		GamepadButtonX,
		GamepadButtonY,
		GamepadButtonL,
		GamepadButtonR,
		GamepadButtonLstick,
		GamepadButtonRstick
	};

	class Gamepad
	{
	public:
		static const int NumButtons = 15;
		static const int NumAxes = 6;

		struct State
		{
			unsigned char buttons[NumButtons];
			float axes[NumAxes];
		};

		struct Config
		{
			float deadzone[4];		// stick1, stick2, left trigger, right trigger
			float saturation[4];	// analogous
		};

	public:
		Gamepad() = delete;
		Gamepad(int id, const char* name);

		bool PollState();

		inline bool IsActive() const { return m_Active; }
		inline const std::string& GetName() const { return m_Name; }
		inline int GetId() const { return m_Id; }

		inline const State& GetState() const { return m_State; }
		inline const State& GetUncorrectedState() const { return m_StateUncorrected; }
		inline const Config& GetConfig() const { return m_Config; }

	protected:
		virtual bool PollStateImpl() = 0;


		bool m_Active = true;

		int m_Id;
		std::string m_Name;

		State m_State, m_PrevState, m_StateUncorrected;
		Config m_Config;

	private:
		void CorrectState();
		void GenerateEvents();
	};
}
