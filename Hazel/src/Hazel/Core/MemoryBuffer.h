#pragma once

#include <string>
#include <array>
#include <assert.h>

namespace Hazel
{
	using MemoryType = uint8_t;
	using StringLenType = uint32_t;

	template <uint32_t> class Serializable;

	// ---------------------------------

	template <uint32_t MaxSize>
	class MemoryBuffer
	{
	public:
		MemoryBuffer() = default;

		template <uint32_t _>
		inline MemoryBuffer(const MemoryBuffer<_>& buffer) { Push(buffer); }

		inline MemoryBuffer(void* data, uint32_t size) { Push(data, size); }


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

#define MEMORY_BUFFER_CONSTR(type)\
		inline void Push(const type& data) { Push((void*)&data, sizeof(type)); }

#define MEMORY_BUFFER_CONSTR_SIGNED_UNSIGNED(type)\
		inline void Push(const signed type& data) { Push((void*)&data, sizeof(type)); }\
		inline void Push(const unsigned type& data) { Push((void*)&data, sizeof(type)); }

		MEMORY_BUFFER_CONSTR_SIGNED_UNSIGNED(char)
		MEMORY_BUFFER_CONSTR_SIGNED_UNSIGNED(short)
		MEMORY_BUFFER_CONSTR_SIGNED_UNSIGNED(int)
		MEMORY_BUFFER_CONSTR_SIGNED_UNSIGNED(long)
		MEMORY_BUFFER_CONSTR_SIGNED_UNSIGNED(long long)
		MEMORY_BUFFER_CONSTR(float)
		MEMORY_BUFFER_CONSTR(double)
		MEMORY_BUFFER_CONSTR(long double)

#undef MEMORY_BUFFER_CONSTR
#undef MEMORY_BUFFER_CONSTR_SIGNED_UNSIGNED

		/*template <typename T>
		inline void Push(const T& data)
		{
			HZ_CORE_ASSERT(std::is_fundamental<T>::value, "This function is only safe for primitive data types");
			Push((void*)&data, sizeof(T));
		}*/

		template <uint32_t _>
		inline void Push(const MemoryBuffer<_>& buffer) { Push(buffer.GetStream(), buffer.GetPointer()); }

		template <uint32_t _>
		inline void Push(const Serializable<_>& s);


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

#include "Hazel/Core/Serialize.h"

namespace Hazel
{
	template <uint32_t MaxSize>
	template <uint32_t _>
	inline void MemoryBuffer<MaxSize>::Push(const Serializable<_>& s)
	{
		s.Serialize(*this);
	}
}
