#pragma once

namespace Hazel
{
	template <size_t Size = 32>
	class MemoryBuffer
	{
	public:
		MemoryBuffer() = default;

		template <size_t SizeB>
		inline MemoryBuffer(const MemoryBuffer<SizeB>& b) { Push(b.m_Stream, b.m_Pointer); }

		template <typename T>
		inline void Push(const T& data) { Push(&data, sizeof(T)); }

		void Push(void* data, size_t size)
		{
			HZ_CORE_ASSERT(GetHeadroom() >= size, "Packet size not sufficient!");
			memcpy(m_Stream + m_Pointer, data, size);
			m_Pointer += size;
		}

		void Push(const char* string)
		{
			const size_t N = strlen(string);
			Push(N); Push(string, N + 1);
		}

		inline size_t GetHeadroom() const { return Size - m_Pointer; }
		inline unsigned char* GetStream() { return m_Stream; }
		inline size_t GetPointer() const { return m_Pointer; }

	private:
		unsigned char m_Stream[Size];
		size_t m_Pointer = 0;
	};

	template <size_t Size>
	class Serializable
	{
	public:
		inline const MemoryBuffer<Size>& Serialize() { SerializeImpl(m_Buffer); return m_Buffer; }
		inline void Deserialize() { DeserializeImpl(m_Buffer); }

	protected:
		Serializable() = default;

		virtual void SerializeImpl(MemoryBuffer<Size>& buffer) = 0;
		virtual void DeserializeImpl(MemoryBuffer<Size>& buffer) = 0;

	private:
		MemoryBuffer<Size> m_Buffer;
	};
}
