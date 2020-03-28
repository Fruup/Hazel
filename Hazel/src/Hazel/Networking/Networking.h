#pragma once

#include <enet/enet.h>
#include <queue>

#include "Hazel/Events/Event.h"

#include "Hazel/Core/Serialize.h"

namespace Hazel
{
	using ClientId = long;
	using NetMsgType = enet_uint32;

	enum ClientIdSpecifiers : ClientId
	{
		All = -10000,	// TODO: min number, i'm too dumb right now
		Server,
		Invalid = -1
	};

	enum
	{
		ServerInformationMsgType = 0xf0000000ui32,
		ClientInformationMsgType
	};

	class NetAddress
	{
	public:
		NetAddress() = default;

		inline NetAddress(enet_uint32 host, enet_uint16 port) :
			NetAddress(ENetAddress({ host, port })) {}

		NetAddress(const ENetAddress& address) :
			m_InternalAddress(address)
		{
			enet_address_get_host(&address, m_Hostname, 32);
			enet_address_get_host_ip(&address, m_Address, 32);
		}

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

	struct NetClientInfo : public Serializable<3>
	{
		NetClientInfo() :
			Serializable(Id, Version, Name)
		{
		}

		ClientId Id = -1;
		bool Used = false;

		ENetPeer* Peer = nullptr;
		enet_uint32 Version;
		std::string Name;
		NetAddress Address;

		void* UserData = nullptr;
	};

	class NetPacket
	{
	public:
		NetPacket() = delete;
		NetPacket(const NetPacket&) = delete;
		NetPacket(const NetPacket&&) = delete;

		NetPacket(const ENetPacket* packet);
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
		inline const enet_uint8* GetStream() const { return m_Stream; }
		inline size_t GetStreamPointer() const { return m_StreamPointer; }

		static constexpr int StreamSize = 128;
		static constexpr size_t StreamStart = sizeof(ClientId) * 2 + sizeof(NetMsgType);

	private:
		NetMsgType m_Type;
		ClientId m_To, m_From;

		enet_uint8 m_Stream[StreamSize];
		size_t m_StreamPointer = 0;
	};

	class Networking
	{
	public:
		static constexpr int NumClients = 8;
		static constexpr int NumChannels = 2;

		enum NetworkingState
		{
			None				= 0,
			InitializedState	= BIT(1),
			ClientState			= BIT(2),
			ServerState			= BIT(3),
			ListeningState		= BIT(4),
			ConnectedState		= BIT(5),
			ReadyState			= BIT(6)
		};

		struct NetworkingData
		{
			enet_uint32 State = None;
			
			NetClientInfo Clients[NumClients];

			ClientId ClientId = Invalid;

			NetAddress ServerAddress;

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

		static void Server(enet_uint16 port);
		static void Client(const ENetAddress& serverAddress, enet_uint32 timeout);
		
		static void Disconnect(enet_uint32 timeout = 2000);

		static void QueuePacket(const NetPacket& packet, enet_uint8 channel = 0, enet_uint32 additionalFlags = 0);
		static void PushPackets();

		static void PushEngineEvents();

		static const NetClientInfo& FindClientInfo(ClientId id);

		inline static bool CheckState(NetworkingState s) { return s_Data.State & s; }
		
		inline static const NetworkingData& GetData() { return s_Data; }

	private:
		Networking() = default;

		static void ListenAsync();
		static void ListenProc();
		static void ClientConnectAsync(enet_uint32 timeout);
		static void ClientConnectProc(enet_uint32 timeout);
		static void QueueEngineEvent(Event* event);

		static void SendClientInformation(ClientId recipient);

		inline static void SetState(NetworkingState s) { s_Data.State |= s; }
		inline static void UnsetState(NetworkingState s) { s_Data.State ^= s; }

		static NetworkingData s_Data;
	};
}
