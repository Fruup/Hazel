#include "hzpch.h"
#include "Networking.h"

#include "Hazel/Events/NetEvent.h"
#include "Hazel/Core/Application.h"

namespace Hazel
{
	Networking::NetworkingData Networking::s_Data;

	// -------------------------------------------------------

#ifdef HZ_DEBUG
	static size_t g_ENetMemory = 0;
	static std::unordered_map<intptr_t, size_t> g_ENetMemoryMap;

	void* NetMalloc(size_t size)
	{
		void* memory = malloc(size);

		g_ENetMemoryMap[(intptr_t)memory] = size;
		g_ENetMemory += size;

		return memory;
	}

	void NetFree(void* memory)
	{
		g_ENetMemory -= g_ENetMemoryMap[(intptr_t)memory];
		free(memory);
	}

	void NetNoMemory()
	{
	}
#endif

	// -------------------------------------------------------

	void Networking::Init(bool reliablePackets)
	{
		if (IsInitialized())
			return;

		if (reliablePackets)
			s_Data.PacketFlags |= ENET_PACKET_FLAG_RELIABLE;

		// Init enet
#ifdef HZ_DEBUG
		ENetCallbacks callbacks = { NetMalloc, NetFree, NetNoMemory };
		if (enet_initialize_with_callbacks(ENET_VERSION, &callbacks))
#else
		if (enet_initialize())
#endif
		{
			HZ_CORE_ERROR("Failed to initialize enet module");
			return;
		}


		s_Data.Initialized = true;
	}

	void Networking::Shutdown()
	{
		if (!s_Data.Initialized)
			return;

		// Stop server or disconnect client
		if (IsServer())
			Server::Stop();
		else if (IsClient())
			Client::Disconnect(0);

		s_Data.Peers.clear();
		
		// Deinit enet
		enet_deinitialize();

		HZ_CORE_ASSERT(g_ENetMemory == 0, "There is still ENet related memory allocated!");


		s_Data.Initialized = false;
	}

	void Networking::QueuePacket(const BaseNetPacket& packet, enet_uint8 channel, enet_uint32 additionalFlags)
	{
		// Set packet sender
		if (packet.GetSender() == PeerId::Invalid)
			packet.m_From = s_Data.ThisPeerId;

		// Create packet
		auto enetpacket = enet_packet_create(packet.GetPackedData(), packet.GetPackedSize(), s_Data.PacketFlags | additionalFlags);

		// Send packet
		if (IsServer())
		{
			if (packet.GetRecipient() == PeerId::All)
				enet_host_broadcast(s_Data.Host, channel, enetpacket);
			else if (enet_peer_send(s_Data.Peers.at(packet.GetRecipient()).Peer, channel, enetpacket))
				HZ_CORE_ERROR("Failed to queue network packet of type {0} and size {1}!", packet.GetType(), packet.GetPackedSize());
		}
		else
		{
			if (enet_peer_send(s_Data.Peer, channel, enetpacket))
				HZ_CORE_ERROR("Failed to queue network packet of type {0} and size {1}!", packet.GetType(), packet.GetPackedSize());
		}
	}

	void Networking::PushPackets()
	{
		// Flush packet queue
		if (s_Data.Host)
			enet_host_flush(s_Data.Host);
	}

	void Networking::PushEngineEvents()
	{
		// Lock event queue and push events to application
		s_Data.EngineEventQueueMutex.lock();

		while (!s_Data.EngineEventQueue.empty())
		{
			Application::PushEvent(*s_Data.EngineEventQueue.front());
			s_Data.EngineEventQueue.pop();
		}

		s_Data.EngineEventQueueMutex.unlock();
	}

	NetPeerInfo* Networking::FindClientById(PeerId::Type id)
	{
		const auto it = s_Data.Peers.find(id);
		if (it == s_Data.Peers.end())
			return nullptr;
		return &it->second;
	}

	NetPeerInfo* Networking::FindClientByPeer(ENetPeer* peer)
	{
		for (auto& [id, client] : s_Data.Peers)
			if (client.Peer == peer)
				return &client;
		return nullptr;
	}

	const char* Networking::GetStateString()
	{
		switch (s_Data.State)
		{
		case NetState::Connected: return "Connected, ready for use.";
		case NetState::Connecting: return "Connecting.";
		case NetState::Disconnected: return "Disconnected.";
		case NetState::Disconnecting: return "Disconnecting.";
		case NetState::WaitingForServerInfo: return "Waiting for server info...";
		}

		return nullptr;
	}

	const char* Networking::GetDisconnectReasonString(NetDisconnectReasons r)
	{
		switch (r)
		{
		case NetDisconnectReasons::None: return "None";
		case NetDisconnectReasons::Banned: return "Banned from server";
		case NetDisconnectReasons::ServerFull: return "Server full";
		case NetDisconnectReasons::VersionMismatch: return "Version mismatch";
		}
		return nullptr;
	}

	void Networking::QueueEngineEvent(Event* event)
	{
		s_Data.EngineEventQueueMutex.lock();
		s_Data.EngineEventQueue.push(Ref<Event>(event));
		s_Data.EngineEventQueueMutex.unlock();
	}

	void Networking::SendServerInformation(PeerId::Type recipient)
	{
		NetPacket<256> packet(NetMsgType::ServerInformation, recipient);

		// Serialize client info
		packet.Write(s_Data.MaxPeers);

		for (const auto&[id, peer] : s_Data.Peers)
			packet.Write(peer);

		// Send packet immediately
		QueuePacket(packet, 0, ENET_PACKET_FLAG_RELIABLE);
		PushPackets();
	}

	void Networking::DispatchServerInformation(BaseMemoryBuffer& buffer)
	{
		// Read number of clients
		buffer.Read(&s_Data.MaxPeers);

		// Allocate space
		s_Data.Peers.reserve(s_Data.MaxPeers);

		// Deserialize peer info
		NetPeerInfo peer;

		while (!buffer.IsEOF())
		{
			peer.Deserialize(buffer);
			AddPeer(peer);
		}
	}

	void Networking::SendClientConnectedMsg(PeerId::Type id)
	{
		NetPacket<64> packet(NetMsgType::ClientConnected, PeerId::All);
		packet.Write(s_Data.Peers.at(id));

		QueuePacket(packet, 0, ENET_PACKET_FLAG_RELIABLE);
	}

	void Networking::SendClientDisconnectedMsg(PeerId::Type id, NetDisconnectReasons reason)
	{
		NetPacket<64> packet(NetMsgType::ClientDisconnected, PeerId::All);
		packet.Write(id);
		packet.Write(reason);

		QueuePacket(packet, 0, ENET_PACKET_FLAG_RELIABLE);
	}

	void Networking::SendServerDisconnectedMsg()
	{
		NetPacket<32> packet(NetMsgType::ServerDisconnected, PeerId::All);

		QueuePacket(packet, 0, ENET_PACKET_FLAG_RELIABLE);
		PushPackets();
	}

	NetPeerInfo* Networking::AddPeer(PeerId::Type id)
	{
		HZ_CORE_ASSERT(GetNumPeers() < s_Data.MaxPeers, "About to exceed max peer count!");
		HZ_CORE_ASSERT(FindClientById(id) == nullptr, "Client with id {0} already in use!", id);

		auto& peer = s_Data.Peers[id];
		peer.Id = id;
		return &peer;
	}

	NetPeerInfo* Networking::AddPeer(const NetPeerInfo& peer)
	{
		auto newPeer = AddPeer(peer.Id);
		*newPeer = peer;
		return newPeer;
	}

	void Networking::RemovePeer(PeerId::Type id)
	{
		HZ_CORE_ASSERT(FindClientById(id) != nullptr, "Client with id {0} not found!", id);
		s_Data.Peers.erase(id);
	}
}
