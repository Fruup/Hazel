#pragma once

#include "Hazel.h"
#include "Hazel/Core/MemoryBuffer.h"

namespace Hazel
{
	enum class MemoryChunkType : uint32_t
	{
		Unknown, Fundamental, StdString
	};

	// -----------------------------

	struct DataBlock
	{
		DataBlock() = default;
		DataBlock(DataBlock&&) = default;

		template <typename T>
		inline DataBlock(const T& data) :
			Data((void*)&data), Size(sizeof(T)), Type(MemoryChunkType::Fundamental)
		{
			static_assert(std::is_fundamental<T>());
		}

		DataBlock(const std::string& data) :
			Data((void*)&data), Size((uint32_t)-1), Type(MemoryChunkType::StdString)
		{
		}

	public:
		void* Data;
		MemoryChunkType Type;
		uint32_t Size;
	};

	struct MemoryChunk
	{
		MemoryChunk() = default;
		MemoryChunk(void* begin, const DataBlock& block) :
			Type(block.Type), Size(block.Size),
			Offset(((intptr_t)block.Data) - ((intptr_t)begin)) {}

		ptrdiff_t Offset;
		MemoryChunkType Type;
		uint32_t Size;
	};

	// -----------------------------

	template <uint32_t NumVars>
	class Serializable
	{
	public:
		template <uint32_t MaxSize>
		MemoryBuffer<MaxSize>& Serialize(MemoryBuffer<MaxSize>& buffer) const
		{
			for (uint32_t i = 0; i < NumVars; ++i)
			{
				void* data = (MemoryType*)this + m_Layout[i].Offset;

				switch (m_Layout[i].Type)
				{
				case MemoryChunkType::Fundamental:
				{
					buffer.Push(data, m_Layout[i].Size);
					break;
				}

				case MemoryChunkType::StdString:
				{
					buffer.Push(*((std::string*)data));
					break;
				}

				default:
					HZ_CORE_ASSERT(false, "Unkown memory chunk type!");
				}
			}

			if (buffer.GetHeadroom() > 32)
				HZ_CORE_WARN("Serialized {0} bytes with remaining headroom of {1} bytes", buffer.GetPointer(), buffer.GetHeadroom());

			return buffer;
		}

		template <uint32_t MaxSize>
		inline MemoryBuffer<MaxSize> Serialize() const
		{
			MemoryBuffer<MaxSize> buffer;
			Serialize(buffer);
			return buffer;
		}

		ptrdiff_t Deserialize(const MemoryType* stream) const
		{
			MemoryType* pointer = (MemoryType*)stream;
			const MemoryType* begin = (MemoryType*)this;

			for (uint32_t i = 0; i < NumVars; ++i)
			{
				void* data = (void*)(begin + m_Layout[i].Offset);

				switch (m_Layout[i].Type)
				{
					case MemoryChunkType::Fundamental:
					{
						memcpy(data, pointer, m_Layout[i].Size);
						break;
					}

					case MemoryChunkType::StdString:
					{
						// Read string length
						const StringLenType len = *((StringLenType*)pointer);

						// Reserve space in string
						std::string& s = *((std::string*)data);
						s.resize(len);

						memcpy((void*)s.data(), pointer + sizeof(len), len);

						break;
					}

					default:
						HZ_CORE_ASSERT(false, "Unkown memory chunk type!");
				}

				pointer += m_Layout[i].Size;
			}

			return (ptrdiff_t)pointer - (ptrdiff_t)stream;
		}

	protected:
		Serializable() = delete;
		//Serializable(const Serializable&) = delete;
		Serializable(const Serializable&&) = delete;

		template <typename ...Args>
		Serializable(Args&& ...args) { HelpMe({ args... }); }

	private:
		template <uint32_t NumArgs>
		inline void HelpMe(const DataBlock(&layout)[NumArgs])
		{
			// Assert number of arguments
			static_assert(NumVars == NumArgs, "Number of variables does not match!");

			// Compute offsets
			for (uint32_t i = 0; i < NumVars; i++)
				m_Layout[i] = MemoryChunk(this, layout[i]);
		}

		MemoryChunk m_Layout[NumVars];
	};
}
