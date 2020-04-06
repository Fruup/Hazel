#pragma once

#include "AssetManager.h"
#include "Hazel/Renderer/Texture.h"

namespace Hazel
{
	class TextureAsset : public Asset
	{
	public:
		static constexpr std::initializer_list<const char*> FileExtensions = { "png", "bmp" };

		void Load(BaseMemoryBuffer& buffer, const std::string& filename) override
		{
			m_Texture = Texture2D::Create(filename);
		}

		inline const Ref<Texture>& GetTexture() const { return m_Texture; }

	private:
		Ref<Texture> m_Texture;
	};
}
