#pragma once

#include "AssetManager.h"

namespace Hazel
{
	class TextFileAsset : public Asset
	{
	public:
		static constexpr std::initializer_list<const char*> FileExtensions = { "txt", "ini", "cfg" };

		inline const std::vector<std::string>& GetLines() const { return m_Lines; }

	private:
		void Load(BaseMemoryBuffer& buffer, const std::string& filename) override
		{
			char c;

			m_Lines.push_back("");
			std::string& line = m_Lines.back();

			while (!buffer.IsEOF())
			{
				buffer.Read(&c, 1);

				if (c == '\n')
				{
					m_Lines.push_back("");
					line = m_Lines.back();
				}
				else
				{
					line.push_back(c);
				}
			}
		}

		std::vector<std::string> m_Lines;
	};
}
