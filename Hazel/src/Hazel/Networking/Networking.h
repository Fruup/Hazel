#pragma once

#include <enet/enet.h>
#include <queue>
#include <unordered_map>

#include "Hazel/Networking/NetworkingDefs.h"
#include "Hazel/Networking/NetAddress.h"
#include "Hazel/Networking/NetPacket.h"

#include "Hazel/Events/Event.h"
#include "Hazel/Core/Serialize.h"

namespace Hazel
{
	class Networking
	{
	public:
		class Server
		{
			friend Networking;
		public:
			static void Start(uint16_t numClients, enet_uint16 port);
			static void Stop();
		private:
			static void ServerProc();

			static void LoadBanList();
			static void RejectClient(ENetPeer* peer, NetDisconnectReasons reason);

			static void CleanUp();

			Server() = default;
		};

		class Client
		{
			friend Networking;
		public:
			static void Connect(const NetAddress& serverAddress, enet_uint32 timeout);
			static void Disconnect(int32_t timeout = 2000, NetDisconnectReasons reason = NetDisconnectReasons::None);
		private:
			static void ClientProc(enet_uint32 connectTimeout);

			static bool ConnectProc(enet_uint32 timeout);
			static bool WaitForServerInfoProc(int32_t timeout);
			static bool ListenProc();

			static void CleanUp();

			Client() = default;
		};

		friend Server;
		friend Client;

	public:
		static constexpr int NumChannels = 2;

		struct NetworkingData
		{
			NetState State = NetState::Disconnected;
			
			bool Initialized = false;
			bool IsServer = false;

			NetVersion Version = 0;

			uint16_t MaxClients;
			std::unordered_map<ClientId::Type, NetClientInfo> Clients;

			ClientId::Type ThisClientId = ClientId::Invalid;

			NetAddress ServerAddress;

			enet_uint32 PacketFlags = 0;

			ENetHost* Host = nullptr;
			ENetPeer* Peer = nullptr;

			std::thread Thread;

			std::queue<Ref<Event>> EngineEventQueue;
			std::mutex EngineEventQueueMutex;

			std::vector<enet_uint32> BanList;
		};

	public:
		static void Init(bool reliablePackets = false);
		static void Shutdown();

		static void OnEvent(Event& event);

		template <uint32_t MaxSize>
		static void QueuePacket(NetPacket<MaxSize>& packet, enet_uint8 channel = 0, enet_uint32 additionalFlags = 0)
		{
			// Set packet sender
			packet.m_From = s_Data.ThisClientId;

			// Set peer
			ENetPeer* peer = s_Data.Peer;

			if (IsServer())
			{
				HZ_CORE_ASSERT(packet.GetRecipient() != ClientId::Server, "Server can not send to itself");
				peer = s_Data.Clients[packet.GetRecipient()].Peer;
			}

			// Send
			auto enetpacket = enet_packet_create(packet.GetPackedData(), packet.GetPackedSize(), s_Data.PacketFlags | additionalFlags);

			if (packet.GetRecipient() == ClientId::All)
				enet_host_broadcast(s_Data.Host, channel, enetpacket);
			else
			{
				if (enet_peer_send(peer, channel, enetpacket))
					HZ_CORE_ERROR("Failed to queue network packet of type {0} and size {1}", packet.GetPackedSize(), packet.GetType());
			}
		}

		static void PushPackets();

		static void PushEngineEvents();

		static NetClientInfo* FindClientById(ClientId::Type id);
		static NetClientInfo* FindClientByPeer(ENetPeer* peer);

		inline static bool IsInitialized() { return s_Data.Initialized; }
		inline static bool IsServer() { return s_Data.IsServer; }
		inline static bool IsClient() { return !IsServer(); }

		inline static bool IsConnected() { return s_Data.State == NetState::Connected; }
		inline static bool IsReady() { return IsConnected(); }

		inline static bool IsServerFull() { return GetNumClients() == s_Data.MaxClients; }
		
		inline static bool IsBanned(enet_uint32 ip) { return std::find(s_Data.BanList.begin(), s_Data.BanList.end(), ip) != s_Data.BanList.end(); }

		inline static size_t GetNumClients() { return s_Data.Clients.size(); }
		inline static NetState GetState() { return s_Data.State; }

		static const char* GetStateString();

		inline static const NetworkingData& GetData() { return s_Data; }


		inline static void SetVersion(NetVersion version) { s_Data.Version = version; }

	private:
		Networking() = default;

		static void QueueEngineEvent(Event* event);

		static void SendServerInformation(ClientId::Type recipient);
		static void RecvServerInformation(MemoryType* data, uint32_t size);

		static void SendClientConnectedMsg(ClientId::Type id);
		static void SendClientDisconnectedMsg(ClientId::Type id);

		static void SendServerDisconnectedMsg();

		static NetworkingData s_Data;
	};
}
