#pragma once

#include "Hazel.h"
#include "Hazel/Core/MemoryBuffer.h"

#define HZ_COMPILE_ASSERT(x) int __assertDummy[(int)(x)]

namespace Hazel
{
	enum class MemoryChunkType : uint32_t
	{
		Unknown,
		Char, Short, Int, Long, LongLong,
		UChar, UShort, UInt, ULong, ULongLong,
		Float, Double, LongDouble,
		PtrDiff,
		StdString
	};

	// -----------------------------

	struct DataBlock
	{
#define DATA_BLOCK_CONSTR_SIGNED_UNSIGNED(Typename, DataType)\
		inline DataBlock(const signed Typename& data)\
			: DataBlock((void*)&data, sizeof(Typename), MemoryChunkType::##DataType) {}\
		inline DataBlock(const unsigned Typename& data)\
			: DataBlock((void*)&data, sizeof(Typename), MemoryChunkType::U##DataType) {}

#define DATA_BLOCK_CONSTR(Typename, DataType)\
		inline DataBlock(const Typename& data)\
			: DataBlock((void*)&data, sizeof(Typename), MemoryChunkType::##DataType) {}


		DataBlock() = default;
		DataBlock(DataBlock&&) = default;

		// Constructors
		DATA_BLOCK_CONSTR_SIGNED_UNSIGNED(char, Char)
		DATA_BLOCK_CONSTR_SIGNED_UNSIGNED(short, Short)
		DATA_BLOCK_CONSTR_SIGNED_UNSIGNED(int, Int)
		DATA_BLOCK_CONSTR_SIGNED_UNSIGNED(long, Long)
		DATA_BLOCK_CONSTR_SIGNED_UNSIGNED(long long, LongLong)

		DATA_BLOCK_CONSTR(float, Float)
		DATA_BLOCK_CONSTR(double, Double)
		DATA_BLOCK_CONSTR(long double, LongDouble)

		DataBlock(const std::string& data) :
			DataBlock((void*)&data, (uint32_t)-1, MemoryChunkType::StdString) {}

	public:
		void* Data;
		MemoryChunkType Type;
		uint32_t Size;

	private:
		DataBlock(void* data, uint32_t size, MemoryChunkType type) :
			Data(data),
			Type(type),
			Size(size)
		{
		}
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
		MemoryBuffer<MaxSize> Serialize() const
		{
			MemoryBuffer<MaxSize> buffer;

			for (uint32_t i = 0; i < NumVars; ++i)
			{
				void* data = (MemoryType*)this + m_Layout[i].Offset;

				switch (m_Layout[i].Type)
				{
					case MemoryChunkType::Char:
					case MemoryChunkType::Short:
					case MemoryChunkType::Int:
					case MemoryChunkType::Long:
					case MemoryChunkType::LongLong:
					case MemoryChunkType::UChar:
					case MemoryChunkType::UShort:
					case MemoryChunkType::UInt:
					case MemoryChunkType::ULong:
					case MemoryChunkType::ULongLong:
					case MemoryChunkType::Float:
					case MemoryChunkType::Double:
					case MemoryChunkType::LongDouble:
					case MemoryChunkType::PtrDiff:
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

			HZ_CORE_INFO("Serialized {0} bytes with remaining headroom of {1} bytes", buffer.GetPointer(), buffer.GetHeadroom());

			return buffer;
		}

		void Deserialize(MemoryType* stream) const
		{
			const MemoryType* begin = (MemoryType*)this;

			for (uint32_t i = 0; i < NumVars; ++i)
			{
				void* data = (void*)(begin + m_Layout[i].Offset);

				switch (m_Layout[i].Type)
				{
					case MemoryChunkType::Char:
					case MemoryChunkType::Short:
					case MemoryChunkType::Int:
					case MemoryChunkType::Long:
					case MemoryChunkType::LongLong:
					case MemoryChunkType::UChar:
					case MemoryChunkType::UShort:
					case MemoryChunkType::UInt:
					case MemoryChunkType::ULong:
					case MemoryChunkType::ULongLong:
					case MemoryChunkType::Float:
					case MemoryChunkType::Double:
					case MemoryChunkType::LongDouble:
					case MemoryChunkType::PtrDiff:
					{
						memcpy(data, stream, m_Layout[i].Size);
						break;
					}

					case MemoryChunkType::StdString:
					{
						// Read string length
						const StringLenType len = *((StringLenType*)stream);

						// Reserve space in string
						std::string& s = *((std::string*)data);
						s.resize(len);

						memcpy((void*)s.data(), stream + sizeof(len), len);

						break;
					}

					default:
						HZ_CORE_ASSERT(false, "Unkown memory chunk type!");
				}

				stream += m_Layout[i].Size;
			}
		}

	protected:
		Serializable() = delete;
		Serializable(const Serializable&) = delete;
		Serializable(const Serializable&&) = delete;

		template <typename ...Args>
		inline Serializable(Args&& ...args) { HelpMe({ args... }); }

	private:
		template <uint32_t NumVars2>
		void HelpMe(const DataBlock(&layout)[NumVars2])
		{
			// Assert number of arguments
			//HZ_COMPILE_ASSERT(NumVars == NumVars2);
			HZ_CORE_ASSERT(NumVars == NumVars2, "Number of variables do not match!");

			// Compute offsets
			for (uint32_t i = 0; i < NumVars; i++)
				m_Layout[i] = MemoryChunk(this, layout[i]);
		}

		MemoryChunk m_Layout[NumVars];
	};
}
