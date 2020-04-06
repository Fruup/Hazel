#pragma once

#include "Hazel.h"

#include "Hazel/Core/MemoryBuffer.h"

namespace Hazel
{
	using AssetId = uint32_t;

	enum class AssetType
	{
		None,
		TextFile,
		Texture
	};

	class Asset
	{
		friend class AssetManager;

	public:
		inline AssetId GetId() const { return m_Id; }
		inline AssetType GetType() const { return m_Type; }

		static Asset* Create(AssetType type, AssetId id);

	protected:
		Asset() = default;

		virtual void Load(BaseMemoryBuffer& buffer, const std::string& filename) = 0;

	private:
		AssetType m_Type = AssetType::None;
		AssetId m_Id = 0;
	};

	class AssetManager
	{
	public:
		static Ref<Asset> GetAsset(const std::string& filename); // Find or create asset

		static Ref<Asset> FindAsset(AssetId id); // Find existing asset

	private:
		static void AddAsset(const Ref<Asset>& asset);

		struct AssetManagerData
		{
			std::unordered_map<AssetId, Ref<Asset>> Assets;
		};

		static AssetManagerData s_Data;
	};
}
