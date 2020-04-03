#pragma once

#include <enet/enet.h>
#include <queue>

#include "Hazel/Networking/NetworkingDefs.h"
#include "Hazel/Networking/NetAddress.h"
#include "Hazel/Networking/NetPacket.h"
#include "Hazel/Events/Event.h"

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
		public:
			NetState State = NetState::Disconnected;
			bool Initialized = false;
			bool IsServer = false;
			NetVersion Version = 0;
			uint16_t MaxPeers = 0;
			std::unordered_map<PeerId::Type, NetPeerInfo> Peers;
			PeerId::Type ThisPeerId = PeerId::Invalid;
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

		static void QueuePacket(const BaseNetPacket& packet, enet_uint8 channel = 0, enet_uint32 additionalFlags = 0);

		static void PushPackets();

		static void PushEngineEvents();

		static NetPeerInfo* FindClientById(PeerId::Type id);
		static NetPeerInfo* FindClientByPeer(ENetPeer* peer);

		inline static bool IsInitialized() { return s_Data.Initialized; }
		inline static bool IsServer() { return s_Data.IsServer; }
		inline static bool IsClient() { return !IsServer(); }

		inline static bool IsConnected() { return s_Data.State == NetState::Connected; }
		inline static bool IsReady() { return IsConnected(); }

		inline static bool IsServerFull() { return GetNumPeers() == s_Data.MaxPeers; }
		
		inline static bool IsBanned(enet_uint32 ip) { return std::find(s_Data.BanList.begin(), s_Data.BanList.end(), ip) != s_Data.BanList.end(); }

		inline static size_t GetNumPeers() { return s_Data.Peers.size(); }
		inline static size_t GetNumClients() { return GetNumPeers() - 1; }
		inline static PeerId::Type GetThisPeerId() { return s_Data.ThisPeerId; }
		inline static NetState GetState() { return s_Data.State; }

		static const char* GetStateString();
		static const char* GetDisconnectReasonString(NetDisconnectReasons r);

		inline static const NetworkingData& GetData() { return s_Data; }


		inline static void SetVersion(NetVersion version) { s_Data.Version = version; }

	private:
		Networking() = default;

		static void QueueEngineEvent(Event* event);

		static void SendServerInformation(PeerId::Type recipient);
		static void DispatchServerInformation(BaseMemoryBuffer& buffer);

		static void SendClientConnectedMsg(PeerId::Type id);
		static void SendClientDisconnectedMsg(PeerId::Type id, NetDisconnectReasons reason);

		static void SendServerDisconnectedMsg();

		static NetPeerInfo* AddPeer(PeerId::Type id);
		static NetPeerInfo* AddPeer(const NetPeerInfo& peer);
		static void RemovePeer(PeerId::Type id);

		static NetworkingData s_Data;
	};
}
