#pragma once

#include <enet/enet.h>
#include <string>

namespace Hazel
{
	class NetAddress
	{
	public:
		NetAddress() :
			m_InternalAddress({ 0, 0 }),
			m_Hostname("empty"),
			m_Address("empty")
		{
		}

		NetAddress(const NetAddress& b) :
			m_InternalAddress(b.m_InternalAddress)
		{
			strcpy_s(m_Hostname, b.m_Hostname);
			strcpy_s(m_Address, b.m_Address);
		}

		inline NetAddress(enet_uint32 host, enet_uint16 port) :
			NetAddress(ENetAddress({ host, port })) {}

		NetAddress(const ENetAddress& address) :
			m_InternalAddress(address)
		{
			enet_address_get_host(&address, m_Hostname, 32);
			enet_address_get_host_ip(&address, m_Address, 32);
		}

		inline NetAddress(const char* address) :
			NetAddress(std::string(address)) {}

		NetAddress(const std::string& address)
		{
			// Copy full string
			strcpy_s(m_Address, address.c_str());

			// Split into ip and port
			size_t m = address.find_first_of(':');

			std::string ip = address.substr(0, m);
			std::string port = address.substr(m + 1);

			// Set ip and port
			enet_address_set_host_ip(&m_InternalAddress, ip.c_str());
			m_InternalAddress.port = (enet_uint32)std::stoi(port);

			enet_address_get_host(&m_InternalAddress, m_Hostname, 32);
		}

		inline const char* GetHostname() const { return m_Hostname; }
		inline const char* GetAddress() const { return m_Address; }
		inline operator const ENetAddress& () const { return m_InternalAddress; }
		inline operator const ENetAddress* () const { return &m_InternalAddress; }
		inline const ENetAddress* operator -> () { return &m_InternalAddress; }

	private:
		char m_Hostname[32];
		char m_Address[32];

		ENetAddress m_InternalAddress;
	};
}
