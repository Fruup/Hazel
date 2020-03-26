#include "hzpch.h"
#include "Networking.h"

#include "Hazel/Events/NetEvent.h"
#include "Hazel/Core/Application.h"

namespace Hazel
{
	Networking::NetworkingData Networking::s_Data;

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

	NetPacket::NetPacket() :
		m_Type(0), m_To(Invalid), m_From(Invalid) {}

	NetPacket::NetPacket(const ENetPacket* packet)
	{
		m_Type = *(NetMsgType*)(packet->data + 0);
		m_To = *(ClientId*)(packet->data + sizeof(NetMsgType));
		m_From = *(ClientId*)(packet->data + sizeof(NetMsgType) + sizeof(ClientId));

		// Copy data, if existent
		m_StreamPointer = packet->dataLength - GetPackedSize();
		if (m_StreamPointer > 0)
			memcpy(m_Stream, packet->data, m_StreamPointer);
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
		if (reliablePackets)
			s_Data.PacketFlags |= ENET_PACKET_FLAG_RELIABLE;

		// Init enet
		ENetCallbacks callbacks = { NetMalloc, NetFree, NetNoMemory };
		if (enet_initialize_with_callbacks(ENET_VERSION, &callbacks))
		{
			HZ_CORE_ERROR("Failed to initialize enet module");
			return;
		}
	}

	void Networking::Shutdown()
	{
		if (IsConnected())
			Disconnect();
		
		// Deinit enet
		enet_deinitialize();
	}

	void Networking::OnEvent(Event& event)
	{
		if (event.GetEventType() == ConnectedToServerEvent::GetStaticType())
		{
			// Start listening thread
			ListenAsync();
			s_Data.Thread = std::thread(ListenAsync);
		}
	}

	void Networking::Server(const ENetAddress& serverAddress)
	{
		// Set server address
		s_Data.ServerAddress = serverAddress;

		// Create host
		s_Data.Host = enet_host_create(s_Data.ServerAddress, NumClients, NumChannels, /* bandwidth */ 0, 0);

		if (!s_Data.Host)
		{
			HZ_CORE_ERROR("Failed to create host ");
		}
	}

	void Networking::Client(const ENetAddress& serverAddress, enet_uint32 timeout)
	{
		// Set server address
		s_Data.ServerAddress = serverAddress;

		// Try connecting in different thread
		s_Data.Thread = std::thread(ClientConnectAsync, timeout);
	}

	void Networking::Disconnect()
	{
		if (!IsConnected())
			return;

		// End thread and wait
		s_Data.Listening = false;
		s_Data.Thread.join();

		// Destroy host
		enet_host_destroy(s_Data.Host);

		// Push event
		Application::PushEvent(DisconnectedFromServerEvent());
		
		s_Data.SocketType = None;
	}

	void Networking::ClientConnectAsync(enet_uint32 timeout)
	{
		auto startTime = enet_time_get();

		do
		{
			s_Data.Peer = enet_host_connect(s_Data.Host, s_Data.ServerAddress, NumChannels, /* data sent to server */ 0);

			if (s_Data.Peer)
			{
				// Connected successfully
				HZ_CORE_INFO("Successfully connected to server");

				// Push engine event to start listening on next occasion
				QueueEngineEvent(new ConnectedToServerEvent());
				return;
			}

			// Otherwise wait and try again
			else { std::this_thread::sleep_for(std::chrono::milliseconds(250)); }

		} while (enet_time_get() - startTime < timeout);

		// Not connected...
		HZ_CORE_WARN("Server connection timeout ({0}ms)", timeout);
	}

	void Networking::ListenAsync()
	{
		// Start listen thread
		s_Data.Listening = true;

		if (s_Data.SocketType == SocketTypeServer)
			s_Data.Thread = std::thread(Networking::ListenServer);
		else if (s_Data.SocketType == SocketTypeClient)
			s_Data.Thread = std::thread(Networking::ListenClient);
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

	void Networking::ListenServer()
	{
		constexpr enet_uint32 timeout = 1000;
		ENetEvent event;
		int serviceResult;

		while (s_Data.Listening)
		{
			serviceResult = enet_host_service(s_Data.Host, &event, timeout);

			if (serviceResult < 0)
			{
				HZ_CORE_ERROR("Failed to listen for events on host {0}", s_Data.Host->address.host);
			}
			else if (serviceResult > 0)
			{
				switch (event.type)
				{
				case ENET_EVENT_TYPE_CONNECT:
				{
					break;
				}
				case ENET_EVENT_TYPE_DISCONNECT:
				{
					break;
				}
				case ENET_EVENT_TYPE_RECEIVE:
				{
					NetPacket* packet = (NetPacket*)event.packet->data;

					HZ_CORE_WARN("Net message from {0} to {1} saying \"{2}\"", packet->GetSender(), packet->GetRecipient(), (char*)packet->GetStream());

					break;
				}
				default:
					break;
				}
			}
		}
	}

	void Networking::ListenClient()
	{
	}

	void Networking::QueueEngineEvent(Event* event)
	{
		s_Data.EngineEventQueueMutex.lock();
		s_Data.EngineEventQueue.push(Ref<Event>(event));
		s_Data.EngineEventQueueMutex.unlock();
	}
}
