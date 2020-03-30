#include "hzpch.h"
#include "Networking.h"

#include "Hazel/Events/NetEvent.h"

namespace Hazel
{
	void Networking::Client::Connect(const NetAddress& serverAddress, enet_uint32 timeout)
	{
		// Set server address
		s_Data.ServerAddress = serverAddress;
		s_Data.IsServer = false;

		// Set state
		s_Data.State = NetState::Connecting;

		// Try connecting in different thread
		//HZ_CORE_ASSERT(!s_Data.Thread.joinable(), "Networking thread is already in use!");

		if (s_Data.Thread.joinable())
			s_Data.Thread.join();

		s_Data.Thread = std::thread(ClientProc, timeout);
	}

	void Networking::Client::Disconnect(int32_t timeout, NetDisconnectReasons reason)
	{
		if (!IsClient())
			return;

		// Wait for thread to end
		s_Data.State = NetState::Disconnecting;

		if (s_Data.Thread.joinable())
			s_Data.Thread.join();

		// Try soft disconnect from server
		if (s_Data.Peer)
		{
			if (timeout == 0)
			{
				HZ_CORE_INFO("Disconnecting client immediately...");
				enet_peer_disconnect_now(s_Data.Peer, (enet_uint32)reason);
			}
			else
			{
				HZ_CORE_INFO("Waiting for disconnect acknowledgement ({0}ms)...", timeout);


				bool acknowlegded = false;
				ENetEvent event;

				enet_peer_disconnect(s_Data.Peer, (enet_uint32)reason);

				/* Allow up to <timeout> milliseconds for the disconnect to succeed
					* and drop any packets received packets.
					*/
				auto startTime = enet_time_get();
				int32_t timeLeft = timeout;
				int result;

				while ((timeLeft = (timeout - (int32_t)(enet_time_get() - startTime))) > 0 && !acknowlegded)
				{
					result = enet_host_service(s_Data.Host, &event, timeLeft);
					if (result < 0)
					{
						HZ_CORE_ERROR("Enet error in \"enet_host_service\" (WSA last error: {0})", WSAGetLastError());
						break;
					}

					switch (event.type)
					{
						case ENET_EVENT_TYPE_RECEIVE:		enet_packet_destroy(event.packet); break;
						case ENET_EVENT_TYPE_DISCONNECT:	acknowlegded = true; break;
					}
				}

				// Forcefully reset peer if necessary
				if (!acknowlegded)
					HZ_CORE_WARN("Unable to receive disconnection acknowledgement. Resetting peer forcefully.");
				else
					HZ_CORE_INFO("Successfully disconnected from server.");

				enet_peer_reset(s_Data.Peer);
			}

			s_Data.Peer = nullptr;
		}

		// Clean up
		CleanUp();

		// Push event
		//Application::PushEvent(DisconnectedFromServerEvent());
	}

	void Networking::Client::ClientProc(enet_uint32 connectTimeout)
	{
		enet_uint32 startTime = enet_time_get();

		if (ConnectProc(connectTimeout))
		{
			if (WaitForServerInfoProc(connectTimeout - (enet_time_get() - startTime)))
			{
				if (ListenProc())
					return;
			}
		}

		// Set state
		s_Data.State = NetState::Disconnected;

		// Error while connecting or while waiting for server info, OR the server sent a disconnect msg
		/*if (s_Data.Peer)
		{
			enet_peer_reset(s_Data.Peer);
			s_Data.Peer = nullptr;
		}

		if (s_Data.Host)
		{
			enet_host_destroy(s_Data.Host);
			s_Data.Host = nullptr;
		}*/

		CleanUp();
	}

	bool Networking::Client::ConnectProc(enet_uint32 timeout)
	{
		// Create client host
		HZ_CORE_ASSERT(s_Data.Host == nullptr, "Host already in use");

		s_Data.Host = enet_host_create(nullptr, 1, NumChannels, 0, 0);
		if (!s_Data.Host)
		{
			HZ_CORE_ERROR("Failed to create client host");
			return false;
		}

		// Connect host
		s_Data.Peer = enet_host_connect(s_Data.Host, s_Data.ServerAddress, NumChannels, /* data sent to server */ s_Data.Version);

		if (s_Data.Peer)
		{
			HZ_CORE_TRACE("Connecting to server {0}...", s_Data.ServerAddress.GetHostname());

			// Try connecting
			ENetEvent event;
			int result = enet_host_service(s_Data.Host, &event, timeout);

			if (result < 0)
			{
				HZ_CORE_ERROR("Enet error in \"enet_host_service\" (WSA last error: {0})", WSAGetLastError());
				return false;
			}
			else if (result > 0)
			{
				if (event.type == ENET_EVENT_TYPE_CONNECT)
				{
					// Connected successfully
					HZ_CORE_INFO("Successfully connected to server {0}.", s_Data.ServerAddress.GetHostname());

					// Set client id
					s_Data.ThisClientId = s_Data.Peer->connectID;

					// Set state
					s_Data.State = NetState::WaitingForServerInfo;

					// Push engine event to start listening on next occasion
					//QueueEngineEvent(new ConnectedToServerEvent());

					return true;
				}
				else
				{
					HZ_CORE_ASSERT(false, "Something went wrong here...");
				}
			}

			// Not connected...
			HZ_CORE_WARN("Server connection timeout ({0}ms)", timeout);
		}

		HZ_CORE_WARN("Unable to connect client host.");

		return false;
	}

	bool Networking::Client::WaitForServerInfoProc(int32_t timeout)
	{
		auto startTime = enet_time_get();
		int32_t timeLeft = timeout;
		ENetEvent event;

		while (s_Data.State == NetState::WaitingForServerInfo && (timeLeft = (timeout - (int32_t)(enet_time_get() - startTime))) > 0)
		{
			if (enet_host_service(s_Data.Host, &event, timeLeft) > 0)
			{
				switch (event.type)
				{
				case ENET_EVENT_TYPE_DISCONNECT:
				{
					// The client has been disconnected from the server
					NetDisconnectReasons reason = (NetDisconnectReasons)event.data;

					// ...

					return false;
					//break;
				}
				case ENET_EVENT_TYPE_RECEIVE:
				{
					NetPacket<256> packet(event.packet);

					// Destroy packet
					enet_packet_destroy(event.packet);

					// Check if the server info arrived
					if (packet.GetType() == NetMsgType::ServerInformation)
					{
						// Read information
						RecvServerInformation((MemoryType*)packet.GetData(), packet.GetBuffer().GetPointer());

						// Set state and start listening loop
						s_Data.State = NetState::Ready;

						return true;
					}

					break;
				}
				default:
					HZ_CORE_ASSERT(false, "Something went wrong here...");
				}
			}
		}

		HZ_CORE_ERROR("Timed out waiting for server information data ({0}ms)!", timeout);

		return false;
	}

	bool Networking::Client::ListenProc()
	{
		constexpr enet_uint32 timeout = 16;
		int32_t sleepTime, startTime;
		ENetEvent event;
		int serviceResult;

		while (IsConnected())
		{
			startTime = (int32_t)enet_time_get();

			// Process
			serviceResult = enet_host_service(s_Data.Host, &event, timeout);

			if (serviceResult < 0)
			{
				HZ_CORE_ERROR("Failed to listen for events on host {0}. Aborting listen process.", s_Data.Host->address.host);
				return false;
			}
			else if (serviceResult > 0)
			{
				switch (event.type)
				{
					case ENET_EVENT_TYPE_CONNECT:
					{
						HZ_CORE_CRITICAL("CLIENT: ENET_EVENT_TYPE_CONNECT");
						break;
					}
					case ENET_EVENT_TYPE_DISCONNECT:
					{
						HZ_CORE_CRITICAL("CLIENT: ENET_EVENT_TYPE_DISCONNECT");
						break;
					}
					case ENET_EVENT_TYPE_RECEIVE:
					{
						auto packet = std::make_shared<NetPacket<64>>(event.packet);

						// Destroy packet
						enet_packet_destroy(event.packet);


						// Handle event engine side
						if (packet->GetType() == NetMsgType::ClientConnected)
						{
							NetClientInfo newClient;
							newClient.Deserialize(packet->GetData());

							// Ignore if it was this client
							if (newClient.Id != s_Data.ThisClientId)
							{
								// Add new client
								s_Data.Clients[newClient.Id] = newClient;

								// Queue event
								//QueueEngineEvent(new NetClientConnected());
							}
						}
						if (packet->GetType() == NetMsgType::ClientDisconnected)
						{
							// Delete client
							s_Data.Clients.erase(*(ClientId::Type*)packet->GetData());
						}
						if (packet->GetType() == NetMsgType::ServerDisconnected)
						{
							// Destroy packet
							packet = nullptr;

							HZ_CORE_WARN("The server disconnected.");
							return false;
						}
						else
						{
							// Queue event for application use
							QueueEngineEvent(new ReceivedNetMessageEvent(packet));
						}

						break;
					}
					default:
						assert(0);
						break;
				}
			}

			// Wait if necessary
			if ((sleepTime = (int32_t)timeout - ((int32_t)enet_time_get() - startTime)) > 0)
				std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
		}

		return true;
	}

	void Networking::Client::CleanUp()
	{
		// Set state
		s_Data.State = NetState::Disconnected;

		// Delete clients
		s_Data.Clients.clear();

		// Destroy peer
		if (s_Data.Peer)
		{
			enet_peer_reset(s_Data.Peer);
			s_Data.Peer = nullptr;
		}

		// Destroy host
		if (s_Data.Host)
		{
			enet_host_destroy(s_Data.Host);
			s_Data.Host = nullptr;
		}
	}
}
