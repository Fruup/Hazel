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

		s_Data.Clients.clear();
		
		// Deinit enet
		enet_deinitialize();

		HZ_CORE_ASSERT(g_ENetMemory == 0, "There is still ENet related memory allocated!");


		s_Data.Initialized = false;
	}

	void Networking::OnEvent(Event& event)
	{
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

	NetClientInfo* Networking::FindClientById(ClientId::Type id)
	{
		const auto it = s_Data.Clients.find(id);
		if (it == s_Data.Clients.end())
			return nullptr;
		return &it->second;
	}

	NetClientInfo* Networking::FindClientByPeer(ENetPeer* peer)
	{
		for (auto& [id, client] : s_Data.Clients)
			if (client.Peer == peer)
				return &client;
		return nullptr;
	}

	const char* Networking::GetStateString()
	{
		switch (s_Data.State)
		{
		case NetState::Connected: return "Connected.";
		case NetState::Connecting: return "Connecting.";
		case NetState::Disconnected: return "Disconnected.";
		case NetState::Disconnecting: return "Disconnecting.";
		case NetState::WaitingForServerInfo: return "Waiting for server info...";
		}

		return nullptr;
	}

	void Networking::QueueEngineEvent(Event* event)
	{
		s_Data.EngineEventQueueMutex.lock();
		s_Data.EngineEventQueue.push(Ref<Event>(event));
		s_Data.EngineEventQueueMutex.unlock();
	}

	void Networking::SendServerInformation(ClientId::Type recipient)
	{
		NetPacket<256> packet(NetMsgType::ServerInformation, recipient);

		// Serialize client info
		packet.Push(s_Data.MaxClients);

		for (const auto&[id, client] : s_Data.Clients)
			packet.Push(client);

		// Send packet immediately
		QueuePacket(packet, 0, ENET_PACKET_FLAG_RELIABLE);
		PushPackets();
	}

	void Networking::RecvServerInformation(MemoryType* data, uint32_t size)
	{
		ptrdiff_t offset = sizeof(s_Data.MaxClients);

		// Read number of clients
		memcpy(&s_Data.MaxClients, data, offset);

		// Allocate space
		s_Data.Clients.reserve(s_Data.MaxClients);

		// Deserialize clients
		NetClientInfo client;

		while (offset < size)
		{
			offset += client.Deserialize(data + offset);
			s_Data.Clients[client.Id] = client;
		}
	}

	void Networking::SendClientConnectedMsg(ClientId::Type id)
	{
		NetPacket<64> packet(NetMsgType::ClientConnected, ClientId::All);
		packet.Push(s_Data.Clients[id]);

		QueuePacket(packet, 0, ENET_PACKET_FLAG_RELIABLE);
	}

	void Networking::SendClientDisconnectedMsg(ClientId::Type id)
	{
		NetPacket<64> packet(NetMsgType::ClientDisconnected, ClientId::All);
		packet.Push(id);

		QueuePacket(packet, 0, ENET_PACKET_FLAG_RELIABLE);
	}

	void Networking::SendServerDisconnectedMsg()
	{
		NetPacket<32> packet(NetMsgType::ServerDisconnected, ClientId::All);

		QueuePacket(packet, 0, ENET_PACKET_FLAG_RELIABLE);
		PushPackets();
	}
}
