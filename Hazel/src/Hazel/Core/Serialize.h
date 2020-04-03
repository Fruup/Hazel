#pragma once

#include "Hazel/Core/MemoryBuffer.h"

#include <unordered_map>

namespace Hazel
{
	class Layout;

	using HashType = size_t;

	// -----------------------------

	class BaseSerializable
	{
	public:
		virtual HashType GetTypeHash() const = 0;

		inline uint32_t Serialize(BaseMemoryBuffer& stream) const;
		inline uint32_t Deserialize(BaseMemoryBuffer& stream) const;

		inline const Layout& GetLayout() const;

	protected:
		BaseSerializable() = default;
	};

	template <typename T>
	class Serializable : public BaseSerializable
	{
	public:
		inline HashType GetTypeHash() const override { return typeid(T).hash_code(); }

	protected:
		Serializable() = delete;

		template <typename ...Args>
		inline Serializable(Args&& ...args);
	};

	// -----------------------------

	enum class MemoryChunkType : uint32_t
	{
		Fundamental8, Fundamental16, Fundamental32, Fundamental64,
		//Fundamental,
		StdString, Serializable
	};

	// -----------------------------

	struct MemoryChunk
	{
		MemoryChunk() = default;
		MemoryChunk(const MemoryChunk&) = delete;
		MemoryChunk(MemoryChunk&&) = delete;
		MemoryChunk& operator = (const MemoryChunk&) = default;

		MemoryChunk(intptr_t objPtr, const MemoryChunk& other) :
			Offset(other.Ptr - objPtr), Type(other.Type)
		{
		}

		template <typename T>
		MemoryChunk(const T& data) :
			Ptr((intptr_t)&data)
		{
			SetType(data);
		}


		inline intptr_t GetPtr() const { return Ptr; }
		inline intptr_t GetOffset() const { return Offset; }
		inline MemoryChunkType GetType() const { return Type; }
		inline HashType GetHash() const { return Hash; }

	private:
		union { intptr_t Ptr, Offset; };
		MemoryChunkType Type;
		HashType Hash = 0;

	private:
		template <typename T>
		inline std::enable_if_t<std::is_fundamental_v<T>> SetType(const T& data)
		{
			switch (sizeof(T))
			{
			case 1: Type = MemoryChunkType::Fundamental8; break;
			case 2: Type = MemoryChunkType::Fundamental16; break;
			case 4: Type = MemoryChunkType::Fundamental32; break;
			case 8: Type = MemoryChunkType::Fundamental64; break;
			default:
				HZ_CORE_ASSERT(false, "Unsupported memory chunk size!");
			}
		}

		inline void SetType(const std::string& data)
		{
			Type = MemoryChunkType::StdString;
		}

		inline void SetType(const BaseSerializable& obj)
		{
			Type = MemoryChunkType::Serializable;
			Hash = obj.GetTypeHash();
		}
	};

	// -----------------------------

	class Layout
	{
	public:
		Layout(intptr_t objPtr, uint32_t num, MemoryChunk* chunks) :
			m_Num(num)
		{
			m_Chunks = new MemoryChunk[m_Num];
			for (uint32_t i = 0; i < num; i++)
				m_Chunks[i] = MemoryChunk(objPtr, chunks[i]);
		}

		~Layout()
		{
			delete[] m_Chunks;
		}

		inline const MemoryChunk& operator [] (int i) const { return m_Chunks[i]; }
		inline uint32_t GetNumChunks() const { return m_Num; }

	private:
		const uint32_t m_Num;
		MemoryChunk* m_Chunks = nullptr;
	};

	// -----------------------------

	class Serializer
	{
	public:
		~Serializer()
		{
			// Delete layouts
			for (auto l : m_Layouts)
				delete l.second;
		}

		inline static uint32_t Serialize(const BaseSerializable& obj, BaseMemoryBuffer& stream) { return Serialize((const MemoryType*)&obj, obj.GetTypeHash(), stream); }

		static uint32_t Serialize(const MemoryType* obj, HashType hash, BaseMemoryBuffer& stream)
		{
			// Store write pointer
			const uint32_t startPointer = stream.GetWritePointer();

			// Find layout from hash
			const Layout& layout = GetLayout(hash);

			// Serialize
			for (uint32_t i = 0; i < layout.GetNumChunks(); ++i)
			{
				void* data = (void*)(obj + layout[i].GetOffset());

				switch (layout[i].GetType())
				{
				case MemoryChunkType::Fundamental8:
				{
					stream.Write(data, 1);
					break;
				}
				case MemoryChunkType::Fundamental16:
				{
					stream.Write(data, 2);
					break;
				}
				case MemoryChunkType::Fundamental32:
				{
					stream.Write(data, 4);
					break;
				}
				case MemoryChunkType::Fundamental64:
				{
					stream.Write(data, 8);
					break;
				}
				case MemoryChunkType::StdString:
				{
					stream.Write(*reinterpret_cast<std::string*>(data));
					break;
				}
				case MemoryChunkType::Serializable:
				{
					Serialize((const MemoryType*)data, layout[i].GetHash(), stream);
					break;
				}
				default:
					break;
				}
			}

			return stream.GetWritePointer() - startPointer;
		}

		inline static uint32_t Deserialize(const BaseSerializable& obj, BaseMemoryBuffer& stream) { return Deserialize((const MemoryType*)&obj, obj.GetTypeHash(), stream); }

		static uint32_t Deserialize(const MemoryType* obj, HashType hash, BaseMemoryBuffer& stream)
		{
			// Store read pointer
			const uint32_t startPointer = stream.GetReadPointer();

			// Get layout from hash
			const auto& layout = GetLayout(hash);

			// Deserialize
			for (uint32_t i = 0; i < layout.GetNumChunks(); ++i)
			{
				void* dst = (void*)(obj + layout[i].GetOffset());

				switch (layout[i].GetType())
				{
				case MemoryChunkType::Fundamental8:
				{
					stream.Read(dst, 1);
					break;
				}
				case MemoryChunkType::Fundamental16:
				{
					stream.Read(dst, 2);
					break;
				}
				case MemoryChunkType::Fundamental32:
				{
					stream.Read(dst, 4);
					break;
				}
				case MemoryChunkType::Fundamental64:
				{
					stream.Read(dst, 8);
					break;
				}
				case MemoryChunkType::StdString:
				{
					stream.Read(*reinterpret_cast<std::string*>(dst));
					break;
				}
				case MemoryChunkType::Serializable:
				{
					Deserialize((const MemoryType*)dst, layout[i].GetHash(), stream);
					break;
				}
				default:
					break;
				}
			}

			return stream.GetReadPointer() - startPointer;
		}

		template <typename T, typename ...Args>
		static void RegisterLayout(const T& obj, Args&& ...members)
		{
			// Look for existing layout for this data type
			const HashType& hash = obj.GetTypeHash();
			auto it = Get().m_Layouts.find(hash);

			if (it == Get().m_Layouts.end())
			{
				// Create new layout
				MemoryChunk chunks[sizeof...(members)] = { members... };
				Get().m_Layouts.emplace(hash, new Layout((intptr_t)&obj, sizeof...(members), chunks));
			}
		}

		inline static const Layout& GetLayout(HashType hash)
		{
			return *Get().m_Layouts.at(hash);
		}

		template <typename T>
		inline static const Layout& GetLayout()
		{
			return *Get().m_Layouts.at(typeid(T).hash_code());
		}


		static Serializer& Get()
		{
			static Serializer instance;
			return instance;
		}

	private:
		Serializer() = default;

		std::unordered_map<HashType, Layout*> m_Layouts;
	};

	// -----------------------------

	inline uint32_t BaseSerializable::Serialize(BaseMemoryBuffer& stream) const { return Serializer::Serialize(*this, stream); }
	inline uint32_t BaseSerializable::Deserialize(BaseMemoryBuffer& stream) const { return Serializer::Deserialize(*this, stream); }

	inline const Layout& BaseSerializable::GetLayout() const { return Serializer::GetLayout(GetTypeHash()); }

	template <typename T>
	template <typename ...Args>
	inline Serializable<T>::Serializable(Args&& ...args) { Serializer::RegisterLayout(static_cast<T&>(*this), args...); }

	// -----------------------------
}
