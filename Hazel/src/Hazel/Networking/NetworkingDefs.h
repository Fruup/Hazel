#pragma once

#include <inttypes.h>
#include <enet/types.h>

#include "Hazel/Networking/NetAddress.h"

#include "Hazel/Core/Serialize.h"

namespace Hazel
{
	using NetVersion = enet_uint32;

	namespace ClientId {
		using Type = enet_uint32;

		enum : Type
		{
			Invalid = 0,
			Server, All
		};
	}

	namespace NetMsgType {
		using Type = enet_uint32;

		enum : Type
		{
			ServerInformation = 0xf0000000ui32,
			ClientConnected, ClientDisconnected,
			ServerDisconnected
		};
	}

	enum class NetDisconnectReasons : enet_uint32 { None = 1, ServerFull, Banned, VersionMismatch };

	enum class NetState
	{
		Disconnected,
		Connecting,
		WaitingForServerInfo,
		Connected, Ready = Connected,
		Disconnecting
	};

	struct NetClientInfo : public Serializable<3>
	{
		NetClientInfo() :
			Serializable(Id, Version, Name)
		{
		}

		void operator = (const NetClientInfo& b)
		{
			Id = b.Id;
			Peer = b.Peer;
			Version = b.Version;
			Name = b.Name;
			Address = b.Address;
			UserData = b.UserData;
		}

		ClientId::Type Id = ClientId::Invalid;
		ENetPeer* Peer = nullptr;
		enet_uint32 Version = 0;
		std::string Name;
		NetAddress Address;
		void* UserData = nullptr;
	};
}
