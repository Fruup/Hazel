#pragma once

#include "Hazel.h"

#include "Hazel/Core/MemoryBuffer.h"

namespace Hazel
{
	using AssetId = uint32_t;

	class Asset
	{
	public:
		virtual void Load(BaseMemoryBuffer& buffer) = 0;

		inline AssetId GetId() const { return m_Id; }

	protected:
		Asset() : m_Id(0) {}

	private:
		AssetId m_Id;
	};

	class TextFileAsset : public Asset
	{
	public:

	private:
		void Load(BaseMemoryBuffer& buffer) override;

		std::vector<std::string> m_Lines;
	};

	class AssetManager
	{
	public:
		static Ref<Asset> GetAsset(const std::string& filename);

		static Ref<Asset> FindAsset(AssetId id);

	private:
		static void AddAsset(const Ref<Asset>& asset);

		struct AssetManagerData
		{
			std::unordered_map<AssetId, Ref<Asset>> Assets;
		};

		static AssetManagerData s_Data;
	};
}
