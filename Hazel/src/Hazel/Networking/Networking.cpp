#include "hzpch.h"
#include "Networking.h"

#include "Hazel/Events/NetEvent.h"
#include "Hazel/Core/Application.h"

namespace Hazel
{
	Networking::NetworkingData Networking::s_Data;

	inline NetClientInfo& GetPeerInfo(ENetPeer* peer) { return *((NetClientInfo*)peer->data); }

	void* NetMalloc(size_t size)
	{
		return malloc(size);
	}

	void NetFree(void* memory)
	{
		free(memory);
	}

	void NetNoMemory()
	{
	}

	/*NetPacket::NetPacket() :
		m_Type(0), m_To(Invalid), m_From(Invalid) {}*/

	NetPacket::NetPacket(const ENetPacket* packet)
	{
		m_Type = *(NetMsgType*)(packet->data + 0);
		m_To = *(ClientId*)(packet->data + sizeof(NetMsgType));
		m_From = *(ClientId*)(packet->data + sizeof(NetMsgType) + sizeof(ClientId));

		// Copy data, if existent
		int dataLength = (signed)packet->dataLength - (signed)StreamStart;
		if (dataLength > 0)
		{
			m_StreamPointer = dataLength;
			memcpy(m_Stream, packet->data + StreamStart, m_StreamPointer);
		}
	}

	NetPacket::NetPacket(NetMsgType type, ClientId to) :
		m_Type(type), m_To(to), m_From(Networking::GetData().ClientId) {}

	NetPacket::NetPacket(void* data, size_t size, NetMsgType type, ClientId to) :
		m_Type(type), m_To(to), m_From(Networking::GetData().ClientId)
	{
		Push(data, size);
	}

	void Networking::Init(bool reliablePackets)
	{
		if (CheckState(InitializedState))
			return;

		if (reliablePackets)
			s_Data.PacketFlags |= ENET_PACKET_FLAG_RELIABLE;

		// Init enet
		ENetCallbacks callbacks = { NetMalloc, NetFree, NetNoMemory };
		if (enet_initialize_with_callbacks(ENET_VERSION, &callbacks))
		{
			HZ_CORE_ERROR("Failed to initialize enet module");
			return;
		}

		SetState(InitializedState);
	}

	void Networking::Shutdown()
	{
		if (!CheckState(InitializedState))
			return;

		// Disconnect
		Disconnect();
		
		// Deinit enet
		enet_deinitialize();

		UnsetState(InitializedState);
	}

	void Networking::OnEvent(Event& event)
	{
		if (event.GetEventType() == ConnectedToServerEvent::GetStaticType())
		{
			// Start listening thread
			ListenAsync();
		}
	}

	void Networking::Server(enet_uint16 port)
	{
		// Set server address
		s_Data.ServerAddress = NetAddress(ENET_HOST_ANY, port);

		SetState(ServerState);
		SetState(ConnectedState);
		SetState(ReadyState);

		// Create host
		s_Data.Host = enet_host_create(s_Data.ServerAddress, NumClients, NumChannels, /* bandwidth */ 0, 0);

		if (!s_Data.Host)
		{
			HZ_CORE_ERROR("Failed to create server host on port {0}", port);
			return;
		}

		// Start listening
		ListenAsync();
	}

	void Networking::Client(const ENetAddress& serverAddress, enet_uint32 timeout)
	{
		// Create client host
		s_Data.Host = enet_host_create(nullptr, 1, NumChannels, 0, 0);
		if (!s_Data.Host)
		{
			HZ_CORE_ERROR("Failed to create client host");
			return;
		}

		// Set server address
		s_Data.ServerAddress = serverAddress;

		SetState(ClientState);

		// Try connecting in different thread
		ClientConnectAsync(timeout);
	}

	void Networking::Disconnect(enet_uint32 timeout)
	{
		if (!CheckState(ConnectedState))
			return;

		// End thread and wait
		UnsetState(ListeningState);

		if (s_Data.Thread.joinable())
			s_Data.Thread.join();

		// Try soft disconnect from server
		if (CheckState(ClientState))
		{
			bool acknowlegded = false;
			ENetEvent event;

			enet_peer_disconnect(s_Data.Peer, /* sent data */ 0);

			/* Allow up to 3 seconds for the disconnect to succeed
			 * and drop any packets received packets.
			 */
			while (enet_host_service(s_Data.Host, &event, timeout) > 0)
			{
				switch (event.type)
				{
					case ENET_EVENT_TYPE_RECEIVE:
					{
						enet_packet_destroy(event.packet);
						break;
					}
					case ENET_EVENT_TYPE_DISCONNECT:
					{
						HZ_CORE_INFO("Successfully disconnected from server");

						acknowlegded = true;

						break;
					}
				}
			}

			// Forcefully reset peer if necessary
			if (!acknowlegded)
				enet_peer_reset(s_Data.Peer);
		}
		
		// Destroy host
		enet_host_destroy(s_Data.Host);

		// Push event
		Application::PushEvent(DisconnectedFromServerEvent());
		
		s_Data.State = InitializedState;
	}

	void Networking::ClientConnectAsync(enet_uint32 timeout)
	{
		// Start thread
		if (s_Data.Thread.joinable())
			s_Data.Thread.join();

		s_Data.Thread = std::thread(Networking::ClientConnectProc, timeout);
	}

	void Networking::ClientConnectProc(enet_uint32 timeout)
	{
		// Connect host
		s_Data.Peer = enet_host_connect(s_Data.Host, s_Data.ServerAddress, NumChannels, /* data sent to server */ 0);

		if (s_Data.Peer)
		{
			HZ_CORE_TRACE("Connecting to server {0}...", s_Data.ServerAddress.GetHostname());

			// Try connecting
			ENetEvent event;
			while (enet_host_service(s_Data.Host, &event, timeout))
			{
				HZ_CORE_TRACE("Service: {0}", event.type);

				if (event.type == ENET_EVENT_TYPE_CONNECT)
				{
					// Connected successfully
					HZ_CORE_INFO("Successfully connected to server {0}.", s_Data.ServerAddress.GetHostname());

					// Set state
					SetState(ConnectedState);

					// Push engine event to start listening on next occasion
					QueueEngineEvent(new ConnectedToServerEvent());
					return;
				}
			}

			// Not connected...
			HZ_CORE_WARN("Server connection timeout ({0}ms)", timeout);
		}
		else
		{
			HZ_CORE_WARN("Unable to connect client host.");
		}

		// Reset peer
		enet_peer_reset(s_Data.Peer);
	}

	void Networking::ListenAsync()
	{
		// Start listen thread
		SetState(ListeningState);

		if (s_Data.Thread.joinable())
			s_Data.Thread.join();

		s_Data.Thread = std::thread(Networking::ListenProc);
	}

	void Networking::QueuePacket(const NetPacket& packet, enet_uint8 channel, enet_uint32 additionalFlags)
	{
		// Send
		auto enetpacket = enet_packet_create(packet.GetPackedData(), packet.GetPackedSize(), s_Data.PacketFlags | additionalFlags);
		if (enet_peer_send(s_Data.Peer, channel, enetpacket))
		{
			HZ_CORE_ERROR("Failed to queue network packet of type {0} and size {1}", packet.GetPackedSize(), packet.GetType());
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

	const NetClientInfo& Networking::FindClientInfo(ClientId id)
	{
		return s_Data.Clients[id];
	}

	void Networking::ListenProc()
	{
		constexpr enet_uint32 timeout = 100;
		ENetEvent event;
		int serviceResult;

		while (CheckState(ListeningState))
		{
			// Process
			serviceResult = enet_host_service(s_Data.Host, &event, timeout);

			if (serviceResult < 0)
			{
				HZ_CORE_ERROR("Failed to listen for events on host {0}. Aborting listen process.", s_Data.Host->address.host);
				return;
			}
			else if (serviceResult > 0)
			{
				switch (event.type)
				{
				case ENET_EVENT_TYPE_CONNECT:
				{
					HZ_CORE_TRACE("Connect id: ", event.peer->connectID);

					// Setup
					NetClientInfo* client = s_Data.Clients + event.peer->connectID;
					client->Used = true;
					client->Id = event.peer->connectID;
					event.peer->data = client;

					HZ_CORE_WARN("Client with ip {0} connected", client->Address.GetAddress());

					// Queue event
					//QueueEngineEvent(new PeerConnectedEvent(client));

					// Send client info data
					SendClientInformation(client->Id);

					break;
				}
				case ENET_EVENT_TYPE_DISCONNECT:
				{
					// Delete client info
					NetClientInfo* client = s_Data.Clients + event.peer->connectID;
					client->Used = false;

					HZ_CORE_WARN("Client with ip {0} disconnected", client->Address.GetAddress());

					// Queue event
					//QueueEngineEvent(new PeerDisconnectedEvent());

					break;
				}
				case ENET_EVENT_TYPE_RECEIVE:
				{
					auto packet = std::make_shared<NetPacket>(event.packet);

					// Check for client information data
					if (CheckState(ClientState))
					{
						if (packet->GetType() == ClientInformationMsgType)
						{
							// Read data...

							// Mark as ready
							SetState(ReadyState);
						}
					}

					// Queue event
					QueueEngineEvent(new ReceivedNetMessageEvent(packet));

					// Destroy packet
					enet_packet_destroy(event.packet);

					break;
				}

				default:
					break;
				}
			}
		}
	}

	void Networking::QueueEngineEvent(Event* event)
	{
		s_Data.EngineEventQueueMutex.lock();
		s_Data.EngineEventQueue.push(Ref<Event>(event));
		s_Data.EngineEventQueueMutex.unlock();
	}

	void Networking::SendClientInformation(ClientId recipient)
	{
		NetPacket packet(ClientInformationMsgType, recipient);

		for (int i = 0; i < NumClients; i++)
		{
			const auto& client = s_Data.Clients[i];

			if (client.Used)
			{
				// Serlialize client info
				client.Serialize<100>();
				client.Deserialize(0);
			}
		}

		QueuePacket(packet, 0, ENET_PACKET_FLAG_RELIABLE);
		PushPackets();
	}
}
