#pragma once

//#include <inttypes.h>

#include "Hazel/Networking/NetAddress.h"
#include "Hazel/Core/Serialize.h"

namespace Hazel
{
	using NetVersion = uint32_t;

	namespace PeerId {
		using Type = uint32_t;

		enum : Type
		{
			Invalid = 0,
			Server, All
		};
	}

	namespace NetMsgType {
		using Type = uint32_t;

		enum : Type
		{
			None = 0xf0000000ui32,
			ServerInformation,
			ClientConnected, ClientDisconnected,
			ServerDisconnected
		};
	}

	enum class NetDisconnectReasons : uint32_t { None = 1, ServerFull, Banned, VersionMismatch };

	enum class NetState
	{
		Disconnected,
		Connecting,
		WaitingForServerInfo,
		Connected, Ready = Connected,
		Disconnecting
	};

	struct NetPeerInfo : public Serializable<NetPeerInfo>
	{
		NetPeerInfo() :
			Serializable(Id, Version, Name)
		{
		}

		//NetPeerInfo(const NetPeerInfo& other) = default;

		/*NetPeerInfo(const NetPeerInfo& other) :
			Id(other.Id),
			Peer(other.Peer),
			Version(other.Version),
			Name(other.Name),
			Address(other.Address),
			UserData(other.UserData)
		{
		}*/

		//NetPeerInfo& operator = (const NetPeerInfo&) = default;

		PeerId::Type Id = PeerId::Invalid;
		ENetPeer* Peer = nullptr;
		uint32_t Version = 0;
		std::string Name;
		NetAddress Address;
		void* UserData = nullptr;
	};
}
