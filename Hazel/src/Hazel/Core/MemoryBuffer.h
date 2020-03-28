#pragma once

#include <string>
#include <array>
#include <assert.h>

namespace Hazel
{
	using MemoryType = uint8_t;
	using StringLenType = uint32_t;


	template <uint32_t MaxSize>
	class MemoryBuffer
	{
	public:
		MemoryBuffer() = default;

		template <uint32_t _>
		inline MemoryBuffer(const MemoryBuffer<_>& buffer) { Push(buffer); }


		// Push data to the buffer
		void Push(void* data, uint32_t size)
		{
			assert(GetHeadroom() >= size);

			memcpy(m_Stream + m_Pointer, data, size);
			m_Pointer += size;
		}

		inline void Push(const std::string& string)
		{
			StringLenType n = (StringLenType)(string.length() + 1);
			Push(n);
			Push((void*)string.data(), n);
		}

		template <typename T>
		inline void Push(const T& data) { Push((void*)&data, sizeof(T)); }

		template <uint32_t SizeB>
		inline void Push(const MemoryBuffer<SizeB>& buffer) { Push(buffer.GetStream(), buffer.GetPointer()); }


		// Getters
		inline uint32_t GetHeadroom() const { return MaxSize - m_Pointer; }
		inline const MemoryType* GetStream() const { return m_Stream; }
		inline uint32_t GetPointer() const { return m_Pointer; }
		inline uint32_t GetMaxSize() const { return MaxSize; }

	private:
		MemoryType m_Stream[MaxSize];
		uint32_t m_Pointer = 0;
	};
}
