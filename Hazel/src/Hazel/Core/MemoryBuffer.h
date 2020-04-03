#pragma once

#include <string>

namespace Hazel
{
	class BaseSerializable;

	using MemoryType = uint8_t;

	// ------------------------------------------

	class BaseMemoryBuffer
	{
	public:
		BaseMemoryBuffer() = delete;
		BaseMemoryBuffer(BaseMemoryBuffer&) = delete;
		BaseMemoryBuffer(BaseMemoryBuffer&&) = delete;

		BaseMemoryBuffer(uint32_t capacity) :
			m_Delete(true),
			m_Capacity(capacity),
			m_Data(new MemoryType[capacity])
		{
		}

		BaseMemoryBuffer(MemoryType* data, uint32_t capacity, bool delete_ = false) :
			m_Capacity(capacity), m_Data(data), m_Delete(delete_)
		{
		}

		virtual ~BaseMemoryBuffer()
		{
			// Delete data
			if (m_Delete)
				delete[] m_Data;
		}

		// Write
		void Write(void* data, size_t size)
		{
			HZ_CORE_ASSERT(m_WritePointer + size <= m_Capacity, "About to exceed buffer limit!");

			memcpy((void*)(m_Data + m_WritePointer), data, size);
			m_WritePointer += (uint32_t)size;

			UpdateSize();
		}

		template <typename T>
		inline std::enable_if_t<std::is_fundamental_v<T>> Write(const T& data)
		{
			Write((void*)&data, sizeof(T));
		}
		template <typename T>
		inline std::enable_if_t<std::is_enum_v<T>> Write(const T& data)
		{
			Write((void*)&data, sizeof(std::underlying_type_t<T>));
		}
		void Write(const BaseSerializable& s);
		inline void Write(const BaseMemoryBuffer& buffer)
		{
			Write((void*)buffer.m_Data, buffer.m_Size);
		}
		inline void Write(const std::string& s)
		{
			uint32_t len = (uint32_t)(s.length() + 1);

			Write(len);
			Write((void*)s.data(), len);
		}

		// Read
		void Read(void* out, size_t size)
		{
			HZ_CORE_ASSERT(m_ReadPointer + size <= m_Size, "About to exceed buffer limit!");

			memcpy(out, m_Data + m_ReadPointer, size);
			m_ReadPointer += (uint32_t)size;
		}

		template <typename T>
		inline void Read(const T* data)
		{
			static_assert(std::is_trivially_copyable<T>(), "T not trivial!");
			Read((void*)data, sizeof(T));
		}
		inline void Read(std::string& s)
		{
			// Read string length
			uint32_t len;
			Read(&len);

			// Reserve space and copy data to string
			s.resize(len);
			Read((void*)s.data(), len);
		}
		void Read(BaseSerializable& s);

		inline void UpdateSize()
		{
			m_Size = m_WritePointer > m_Size ? m_WritePointer : m_Size;
		}

		// Setters
		inline void SetWritePointer(uint32_t p)
		{
			HZ_CORE_ASSERT(p >= 0 && p <= m_Capacity, "About to exceed buffer limit!");
			m_WritePointer = p;
			UpdateSize();
		}
		inline void SetReadPointer(uint32_t p)
		{
			HZ_CORE_ASSERT(p >= 0 && p <= m_Size, "About to exceed buffer limit!");
			m_ReadPointer = p;
		}

		// Getters
		inline MemoryType* GetData() const { return m_Data; }
		inline bool ShouldDelete() const { return m_Delete; }
		inline uint32_t GetReadPointer() const { return m_ReadPointer; }
		inline uint32_t GetWritePointer() const { return m_WritePointer; }
		inline uint32_t GetSize() const { return m_Size; }
		inline uint32_t GetCapacity() const { return m_Capacity; }
		inline uint32_t GetHeadroom() const { return GetCapacity() - GetSize(); }
		inline bool IsEOF() const { return GetReadPointer() >= GetCapacity(); }

	protected:
		uint32_t m_ReadPointer = 0, m_WritePointer = 0;

		uint32_t m_Size = 0, m_Capacity;
		MemoryType* m_Data;
		bool m_Delete;
	};

	// ------------------------------------------

	// Stack memory buffer
	template <uint32_t Capacity>
	class MemoryBuffer : public BaseMemoryBuffer
	{
	public:
		MemoryBuffer() : BaseMemoryBuffer(m_Data, Capacity, false) {}

	private:
		MemoryType m_Data[Capacity];
	};
}
