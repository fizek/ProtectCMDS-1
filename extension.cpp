/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod Sample Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */
#undef DLL_EXPORT
#include "extension.h"
#include "util.h"
#include "CDetour/detours.h"
#include "iclient.h"
#include "igameevents.h"
#include "iserverentity.h"
#include "iserver.h"
#include "iplayerinfo.h"
#include "netadr.h"
#include "steamclientpublic.h"
#include "tier1/checksum_crc.h"
#include "bone_setup.h"
#include "studio.h"
#include "tier1/bitbuf.h"
#include "inetchannel.h"
#include "helper.cpp"
#include "sys/socket.h"
#include "blockingudpsocket.cpp"
#include "tier0/vcrmode.h"
#include <sh_list.h>
#include "inetmsghandler.h"
#include "inetmessage.h"

#define GAMECONFIG_FILE "protectcmds"
#define MAXPLAYERS 65
#define NETMSG_TYPE_BITS 5
#define net_File 2
#define net_NOP 0
#define net_Disconnect 1

typedef struct netpacket_s
{
	netadr_t		from;		// sender IP
	int				source;		// received source 
	double			received;	// received time
	unsigned char	*data;		// pointer to raw packet data
	bf_read			message;	// easy bitbuf data access
	int				size;		// size in bytes
	int				wiresize;   // size in bytes before decompression
	bool			stream;		// was send as stream
	struct netpacket_s *pNext;	// for internal use, should be NULL in public
} netpacket_t;

class CBaseClient : public IGameEventListener2, public IClient {};
class CGameClient : public CBaseClient {};
class CBaseServer;
class CNetMessage : public INetMessage {};


/* Include */
#include "protectcmds/voicefix.cpp"
#include "protectcmds/gameuifix.cpp"
#include "protectcmds/preconnecthook.cpp"
#include "protectcmds/sourcetvfix.cpp"
#include "protectcmds/boneindexbynamefix.cpp"
#include "protectcmds/takedamagefix.cpp"
#include "protectcmds/blockcmds.cpp"
#include "protectcmds/invalidgamepacketfix.cpp"
#include "protectcmds/filterpacket.cpp"
#include "protectcmds/blockbadloadplugin.cpp"
#include "protectcmds/hashingsteamid.cpp"

ProtectCMDS g_ProtectCMDS;		/**< Global singleton for extension's main interface */

SMEXT_LINK(&g_ProtectCMDS);

const char g_cPrefix[] = "[ProtectCMDS]";
CGlobalVars* g_pGlobals = NULL;
IGameConfig *g_pGameConf = NULL;

bool ProtectCMDS::SDK_OnLoad(char *error, size_t maxlength, bool late) {
	
	FilterPacket::SDK_OnLoad();
	
	if (!gameconfs->LoadGameConfigFile(GAMECONFIG_FILE, &g_pGameConf, error, maxlength)) {
		UTIL_Format(error, maxlength, "Could not read " GAMECONFIG_FILE ".txt: %s", error);
		return false;
	}
	
	CDetourManager::Init(g_pSM->GetScriptingEngine(), g_pGameConf);
	
	return true;
}

void ProtectCMDS::SDK_OnAllLoaded() {
	if (!g_pGameConf) return;
	
	VoiceFix::SDK_OnAllLoaded(g_pGameConf);
	GameUIFix::SDK_OnAllLoaded(g_pGameConf);
	PreConnectHook::SDK_OnAllLoaded(g_pGameConf);
	ProcessClientInfoHook::SDK_OnAllLoaded(g_pGameConf);
	BoneIndexByNameFix::SDK_OnAllLoaded(g_pGameConf);
	TakeDamageFix::SDK_OnAllLoaded(g_pGameConf);
	BlockCmds::SDK_OnAllLoaded(g_pGameConf);
	InvalidGamePacketFix::SDK_OnAllLoaded(g_pGameConf);
	BlockBadLoadPlugin::SDK_OnAllLoaded(g_pGameConf);
	HashingSteamID::SDK_OnAllLoaded(g_pGameConf);
}

void ProtectCMDS::SDK_OnUnload() {	
	gameconfs->CloseGameConfigFile(g_pGameConf);	
}

bool ProtectCMDS::SDK_OnMetamodLoad(ISmmAPI* ismm, char* error, size_t maxlen, bool late) {
	//g_pGlobals = ismm->pGlobals();

	return true;
}