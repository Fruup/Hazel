#include "hzpch.h"
#include "AssetManager.h"

namespace Hazel
{
	enum class AssetType
	{
		None,
		TextFile
	};

	const char* GetFileExtension(const char* filename)
	{
		uint32_t i = 0;
		while (filename[i] != 0)
		{
			if (filename[i] == '.')
				return &filename[i + 1];
			++i;
		}
	}

	template <uint32_t count>
	bool CheckString(const char* str, const char* l[count])
	{
		for (uint32_t i = 0; i < count; i++)
		{
			if (!strcmp(l[i], str))
				return true;
		}

		return false;
	}

	AssetType GetAssetType(const std::string& filename, BaseMemoryBuffer& buffer)
	{
		const char* ext = GetFileExtension(filename.c_str());

		//if (CheckString(ext, { "txt", "ini", "cfg" })) return AssetType::TextFile;
	}

	Ref<Asset> AssetManager::GetAsset(const std::string& filename)
	{
		// Load file in memory buffer
		std::ifstream stream(filename.c_str(), std::ios::binary | std::ios::ate);

		BaseMemoryBuffer buffer(stream.tellg());
		stream.read((char*)buffer.GetData(), buffer.GetCapacity());
		buffer.SetWritePointer(buffer.GetCapacity());

		// Get asset type
		GetAssetType(filename, buffer);

		// Load asset
		Ref<Asset> asset = std::make_shared<Asset>();
		asset->Load(buffer);

		// Add to map
		AddAsset(asset);

		return asset;
	}

	Ref<Asset> AssetManager::FindAsset(AssetId id)
	{
		auto it = s_Data.Assets.find(id);
		if (it == s_Data.Assets.end())
			return Ref<Asset>();
		else
			return it->second;
	}

	void AssetManager::AddAsset(const Ref<Asset>& asset)
	{
		HZ_CORE_ASSERT(!FindAsset(asset->GetId()), "Asset already existent!");
		
		s_Data.Assets.insert(std::make_pair(asset->GetId(), asset));
	
	}
	void TextFileAsset::Load(BaseMemoryBuffer& buffer)
	{
		char c;
		std::string line;

		while (!buffer.IsEOF())
		{
			buffer.Read(&c, 1);

			if (c == '\n')
			{
				m_Lines.push_back(line);
				line = "";
			}
			else
			{
				line.push_back(c);
			}
		}
	}
}
