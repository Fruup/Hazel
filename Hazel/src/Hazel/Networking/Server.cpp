#include "hzpch.h"
#include "Networking.h"

#include "Hazel/Events/NetEvent.h"

namespace Hazel
{
	void Networking::Server::Start(uint16_t numClients, enet_uint16 port)
	{
		// Set server address
		s_Data.ServerAddress = NetAddress(ENET_HOST_ANY, port);

		// Create host
		/* Handle one client more than needed to be able to reject it in case of a full server */
		s_Data.Host = enet_host_create(s_Data.ServerAddress, numClients + 1, NumChannels, /* bandwidth */ 0, 0);
		if (!s_Data.Host)
		{
			HZ_CORE_ERROR("Failed to create server host on port {0}!", port);
			return;
		}

		// Load ban list
		LoadBanList();

		// Reserve space for client and server information
		s_Data.MaxPeers = numClients + 1;
		s_Data.Peers.reserve(s_Data.MaxPeers);

		// Set stuff
		s_Data.IsServer = true;
		s_Data.ThisPeerId = PeerId::Server;
		s_Data.Peer = nullptr;

		// Add client info for server
		NetPeerInfo& info = *AddPeer(PeerId::Server);
		info.Address = s_Data.ServerAddress;
		info.Name = "Server";
		info.Version = s_Data.Version;

		// Set state
		s_Data.State = NetState::Ready;

		// Queue event
		QueueEngineEvent(new PeerConnectedEvent(info));

		// Start listening in different thread
		if (s_Data.Thread.joinable())
			s_Data.Thread.join();

		s_Data.Thread = std::thread(ServerProc);


		HZ_CORE_INFO("Started server on port {0} for {1} clients.", port, numClients);
	}

	void Networking::Server::Stop()
	{
		if (!IsServer())
			return;

		// Wait for thread to end
		s_Data.State = NetState::Disconnecting;

		if (s_Data.Thread.joinable())
			s_Data.Thread.join();

		// Disconnect every client
		SendServerDisconnectedMsg();

		// Clean up
		CleanUp();
	}

	void Networking::Server::ServerProc()
	{
		//const enet_uint32 timeout = 16;
		const enet_uint32 timeout = 16 / (enet_uint32)(Networking::GetNumClients() > 0 ? Networking::GetNumClients() : 1);
		int32_t sleepTime, startTime = 0;
		ENetEvent event;
		int serviceResult;

		while (IsConnected())
		{
			int32_t startTime = (int32_t)enet_time_get();

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
						// Check if server is full
						if (IsServerFull())
						{
							RejectClient(event.peer, NetDisconnectReasons::ServerFull);

							HZ_CORE_WARN("Server full, client {0} rejected.", NetAddress(event.peer->address).GetAddress());
						}

						// Check client version
						else if (NetVersion(event.data) != s_Data.Version)
						{
							RejectClient(event.peer, NetDisconnectReasons::VersionMismatch);
						}

						// Check if client ip is banned
						else if (Networking::IsBanned(event.peer->address.host))
						{
							RejectClient(event.peer, NetDisconnectReasons::Banned);
						}

						else
						{
							// Create new client
							const auto clientId = event.peer->connectID;
							NetPeerInfo& client = *AddPeer(clientId);

							client.Id = clientId;
							client.Peer = event.peer;
							client.Address = event.peer->address;
							client.Name = client.Address.GetHostname();
							client.Version = 0;
							client.UserData = nullptr;

							HZ_CORE_INFO("Client \"{0}\" ({1}) with ip {2} connected.", client.Address.GetHostname(), clientId, client.Address.GetAddress());
							
							// Queue event
							QueueEngineEvent(new PeerConnectedEvent(client));

							// Send server info data
							SendServerInformation(client.Id);

							// Let other clients know
							SendClientConnectedMsg(client.Id);
						}

						break;
					}
					case ENET_EVENT_TYPE_DISCONNECT:
					{
						// Delete client info
						const NetPeerInfo& client = *FindClientByPeer(event.peer);
						NetDisconnectReasons reason = (NetDisconnectReasons)event.data;
						auto clientId = client.Id;

						HZ_CORE_WARN("Client \"{0}\" ({1}) with ip {2} disconnected for reason: {3}.", client.Address.GetHostname(), clientId, client.Address.GetAddress(), GetDisconnectReasonString(reason));

						RemovePeer(clientId);

						// Queue event
						QueueEngineEvent(new PeerDisconnectedEvent(clientId/*, reason*/));

						// Let other clients know
						SendClientDisconnectedMsg(clientId, reason);

						break;
					}
					case ENET_EVENT_TYPE_RECEIVE:
					{
						auto packet = std::make_shared<BaseNetPacket>(event.packet);

						// Destroy packet
						enet_packet_destroy(event.packet);

						// Forward packet to recipient if necessary
						if (packet->GetRecipient() != PeerId::Server)
						{
							QueuePacket(*packet, 0, event.packet->flags);
							PushPackets();
						}

						// Queue event
						QueueEngineEvent(new ReceivedNetMessageEvent(packet));

						break;
					}

					default:
						break;
				}
			}
		}

		// Wait if necessary
		if ((sleepTime = (int32_t)timeout - ((int32_t)enet_time_get() - startTime)) > 0)
			std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
	}

	void Networking::Server::RejectClient(ENetPeer* peer, NetDisconnectReasons reason)
	{
		enet_peer_disconnect_now(peer, (enet_uint32)reason);
	}

	void Networking::Server::CleanUp()
	{
		// Delete clients
		s_Data.Peers.clear();
		s_Data.MaxPeers = 0;

		// Destroy host
		if (s_Data.Host)
		{
			enet_host_destroy(s_Data.Host);
			s_Data.Host = nullptr;
		}

		// Set state
		s_Data.State = NetState::Disconnected;
	}

	void Networking::Server::LoadBanList()
	{
		s_Data.BanList.clear();

		/*const FileAsset& banlist = Application::GetAssetsDirectory().Find("banlist.txt");
		if (banlist)
		{
			// ...
		}*/
	}
}
