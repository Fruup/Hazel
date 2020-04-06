#include "hzpch.h"
#include "AssetManager.h"

#include "TextFileAsset.h"
#include "TextureAsset.h"

namespace Hazel
{
	const char* GetFileExtension(const char* filename)
	{
		uint32_t i = 0;
		while (filename[i] != 0)
		{
			if (filename[i] == '.')
				return &filename[i + 1];
			++i;
		}

		return nullptr;
	}

	bool CheckString(const char* str, std::initializer_list<const char*> l)
	{
		for (const char* s : l)
		{
			if (!strcmp(s, str))
				return true;
		}

		return false;
	}

	AssetType GetAssetType(const std::string& filename, BaseMemoryBuffer& buffer)
	{
		const char* ext = GetFileExtension(filename.c_str());

		if (CheckString(ext, TextFileAsset::FileExtensions)) return AssetType::TextFile;
		else if (CheckString(ext, TextureAsset::FileExtensions)) return AssetType::Texture;
		else
		{
			// Try to extract asset type from file header
			// ...
		}

		return AssetType::None;
	}

	Ref<Asset> AssetManager::GetAsset(const std::string& filename)
	{
		// Load file in memory buffer
		std::ifstream stream(filename.c_str(), std::ios::binary | std::ios::ate);

		BaseMemoryBuffer buffer((uint32_t)stream.tellg());
		stream.read((char*)buffer.GetData(), buffer.GetCapacity());
		buffer.SetWritePointer(buffer.GetCapacity());

		// Get asset type
		AssetType type = GetAssetType(filename, buffer);

		// Compute asset id as hash of its filename
		std::hash<std::string> H;
		auto hash = H(filename);

#if _WIN64
		// XOR hi and lo part of 64-bit hash to fit into 32-bit asset id
		AssetId id = (uint32_t)(hash >> 32) ^ (uint32_t)(hash & 0x00000000ffffffff);
#else
		AssetId id = hash;
#endif

		// Create asset
		Ref<Asset> asset = Ref<Asset>(Asset::Create(type, id));

		// Load asset
		asset->Load(buffer, filename);

		// Add to map
		AddAsset(asset);

		return asset;
	}

	Ref<Asset> AssetManager::FindAsset(AssetId id)
	{
		auto it = s_Data.Assets.find(id);
		if (it == s_Data.Assets.end())
			return Ref<Asset>(nullptr);
		else
			return it->second;
	}

	void AssetManager::AddAsset(const Ref<Asset>& asset)
	{
		HZ_CORE_ASSERT(!FindAsset(asset->GetId()), "Asset already existent!");
		
		s_Data.Assets.insert(std::make_pair(asset->GetId(), asset));
	}

	Asset* Asset::Create(AssetType type, AssetId id)
	{
		Asset* asset = nullptr;

		switch (type)
		{
		case AssetType::TextFile:	asset = new TextFileAsset(); break;
		case AssetType::Texture:	asset = new TextureAsset(); break;
		default:
			HZ_CORE_ASSERT(false, "Unknown asset type!");
		}

		asset->m_Type = type;
		asset->m_Id = id;

		return asset;
	}
}
