#pragma once

#include <enet/enet.h>
#include <queue>

#include "Hazel/Events/Event.h"

namespace Hazel
{
	using ClientId = long;
	using NetMsgType = unsigned long;

	enum ClientIdSpecifiers : ClientId
	{
		All = -10000,	// TODO: min number, i'm too dumb right now
		Server,
		Invalid = -1
	};

	struct NetClientInfo
	{
		ClientId Id = -1;
		bool Used = false;

		ENetPeer* Peer = nullptr;
		enet_uint32 Version;
		std::string Name;
		std::string HostName;
		std::string Address;

		void* UserData = nullptr;
	};

	class NetPacket
	{
	public:
		NetPacket
		();
		NetPacket(NetMsgType type, ClientId to);
		NetPacket(void* data, size_t size, NetMsgType type, ClientId to);
		
		template <typename T>
		inline void Push(const T& data) { Push(&data, sizeof(T)); }
		
		void Push(void* data, size_t size)
		{
			HZ_CORE_ASSERT(GetHeadroom() >= size, "Packet size not sufficient!");
			memcpy(m_Stream + m_StreamPointer, data, size);
			m_StreamPointer += size;
		}

		size_t GetPackedSize() const { return sizeof(enet_uint16) + sizeof(NetMsgType) + sizeof(ClientId) + m_StreamPointer; }
		inline size_t GetHeadroom() const { return StreamSize - m_StreamPointer; }
		inline void* GetPackedData() const { return (void*)this; }
		inline NetMsgType GetType() const { return m_Type; }
		inline ClientId GetRecipient() const { return m_To; }
		inline ClientId GetSender() const { return m_From; }

		static constexpr int StreamSize = 128;

	private:
		NetMsgType m_Type;
		ClientId m_To, m_From;

		enet_uint8 m_Stream[StreamSize];
		size_t m_StreamPointer = 0;
	};

	class NetAddress
	{
	public:
		NetAddress() = default;

		NetAddress(const ENetAddress& address) :
			m_InternalAddress(address)
		{
			enet_address_get_host(&address, m_Hostname, 32);
			enet_address_get_host_ip(&address, m_Address, 32);
		}

		NetAddress(const std::string& address)
		{
			strcpy_s(m_Address, address.c_str());

			const size_t m = address.find_first_of(':');

			m_InternalAddress.host = (enet_uint32)std::stoi(address.substr(0, m - 1));
			m_InternalAddress.port = (enet_uint32)std::stoi(address.substr(m + 1));

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
	
	class Networking
	{
	public:
		enum SocketType { None, SocketTypeClient, SocketTypeServer };

		struct NetworkingData
		{
			SocketType SocketType;
			ClientId ClientId;

			NetAddress ServerAddress;

			bool Listening = false;
			enet_uint32 PacketFlags = 0;

			ENetHost* Host = nullptr;
			ENetPeer* Peer = nullptr;

			std::thread Thread;

			std::queue<Ref<Event>> EngineEventQueue;
			std::mutex EngineEventQueueMutex;
		};

	public:
		static void Init(bool reliablePackets = false);
		static void Shutdown();

		static void OnEvent(Event& event);

		static void Server(const ENetAddress& serverAddress);
		static void Client(const ENetAddress& serverAddress, enet_uint32 timeout);
		
		static void Disconnect();

		static void ListenAsync();

		static void QueuePacket(const NetPacket& packet, enet_uint8 channel = 0, enet_uint32 additionalFlags = 0);
		static void PushPackets();

		static void PushEngineEvents();

		static std::string AddressToString(const ENetAddress* address);

		inline static const NetworkingData& GetData() { return s_Data; }

		static constexpr int StreamSize = 128;
		static constexpr int NumClients = 8;
		static constexpr int NumChannels = 1;

	private:
		Networking() = default;

		static void ListenServer();
		static void ListenClient();
		static void ClientConnectAsync(enet_uint32 timeout);
		static void QueueEngineEvent(Event* event);

		static NetworkingData s_Data;
	};
}
