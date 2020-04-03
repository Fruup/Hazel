#include "hzpch.h"
#include "MemoryBuffer.h"

#include "Serialize.h"

namespace Hazel
{
	void BaseMemoryBuffer::Write(const BaseSerializable& s)
	{
		s.Serialize(*this);
	}

	void BaseMemoryBuffer::Read(BaseSerializable& s)
	{
		s.Deserialize(*this);
	}
}
