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

				// Allow up to <timeout> milliseconds for the disconnect to succeed and drop any packets received
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
		// First connect, then retrieve server info, then start listening
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
		CleanUp();
	}

	bool Networking::Client::ConnectProc(enet_uint32 timeout)
	{
		// Create client host
		HZ_CORE_ASSERT(s_Data.Host == nullptr, "Host already in use!");

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
					HZ_CORE_INFO("Successfully connected to server {0}. Waiting for server info data...", s_Data.ServerAddress.GetHostname());

					// Set client id
					s_Data.ThisPeerId = s_Data.Peer->connectID;

					// Set state
					s_Data.State = NetState::WaitingForServerInfo;

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
		else HZ_CORE_WARN("Unable to connect client host.");

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

					HZ_CORE_WARN("The client was disconnected. Reason: {0}.", GetDisconnectReasonString(reason));
					return false;
				}
				case ENET_EVENT_TYPE_RECEIVE:
				{
					BaseNetPacket packet(event.packet);

					// Destroy packet
					enet_packet_destroy(event.packet);

					// Check if the server info arrived
					if (packet.GetType() == NetMsgType::ServerInformation)
					{
						// Read information
						DispatchServerInformation(packet.GetBuffer());

						// Queue events
						for (const auto&[id, peer] : s_Data.Peers)
							QueueEngineEvent(new PeerConnectedEvent(peer));

						// Set state and start listening loop
						s_Data.State = NetState::Ready;

						HZ_CORE_INFO("Successfully retrieved server information data ({0}/{1} peers). Client ready.", GetNumPeers(), s_Data.MaxPeers);
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
						// The server disconnected
						HZ_CORE_WARN("The server connection timed out.");

						// Queue event
						QueueEngineEvent(new PeerDisconnectedEvent(PeerId::Server));

						return false;
					}
					case ENET_EVENT_TYPE_RECEIVE:
					{
						auto packet = std::make_shared<BaseNetPacket>(event.packet);

						// Destroy packet
						enet_packet_destroy(event.packet);

						// Ignore own packets
						if (packet->GetSender() == s_Data.ThisPeerId)
							break;

						// Handle event engine side
						if (packet->GetType() == NetMsgType::ClientConnected)
						{
							NetPeerInfo newClient;
							newClient.Deserialize(packet->GetBuffer());

							// Ignore if it was this client
							if (newClient.Id != s_Data.ThisPeerId)
							{
								// Add new client
								NetPeerInfo& tmp = *AddPeer(newClient);

								// Queue event
								QueueEngineEvent(new PeerConnectedEvent(tmp));

								HZ_CORE_INFO("Client ({0}, {1}) connected.", tmp.Id, tmp.Name);
							}
						}
						else if (packet->GetType() == NetMsgType::ClientDisconnected)
						{
							// Dispatch
							PeerId::Type clientId;
							NetDisconnectReasons reason;

							packet->Read(&clientId);
							packet->Read(&reason);

							const std::string& name = FindClientById(clientId)->Name;

							// Delete client
							RemovePeer(clientId);

							// Queue event
							QueueEngineEvent(new PeerDisconnectedEvent(clientId/*, reason*/));

							HZ_CORE_INFO("Client ({0}, {1}) disconnected for reason: {2}.", clientId, name, GetDisconnectReasonString(reason));
						}
						else if (packet->GetType() == NetMsgType::ServerDisconnected)
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
		s_Data.Peers.clear();
		s_Data.MaxPeers = 0;

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
