#pragma once

#include <vector>
#include <string>
#include "Hazel/Core/Log.h"

int main(int argc, char** argv);

namespace Hazel {
	class CommandLineArguments {
		friend int ::main(int argc, char** argv);
	public:
		inline static const std::vector<std::string>& get() { return s_CommandLineArguments; }
		static std::string getProgramLocation() {
			std::string r = s_CommandLineArguments[0];
			r = r.substr(0, r.find_last_of('\\') + 1);
			return r;
		}
	private:
		static void set(int argc, char** argv) {
			HZ_ASSERT(s_CommandLineArguments.empty(), "Command line args have already been set!");

			s_CommandLineArguments.reserve(argc);
			for (int i = 0; i < argc; i++)
				s_CommandLineArguments.push_back(std::string(argv[i]));
		}
	private:
		static std::vector<std::string> s_CommandLineArguments;
	};
}
