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

 
CDetour *g_ConnectClientDetour = nullptr;
IForward *g_pConnectForward = nullptr;

void (*RejectConnection)(CBaseServer *, const netadr_t &addr, const char *format, ...);

DETOUR_DECL_MEMBER8(ConnectClient, void *, netadr_t &, addr, int, protocol, int, challenge, int, nAuthProtocol, char const *, name, char const *, password, char *, hashedCDkey, int, cdKeyLen)
{
	if(nAuthProtocol == 3) // k_EAuthProtocolSteam = 3
	{
		char rejectReason[255] = "";
		char szSteamID[64];
		uint64 ullSteamID = *(uint64 *)hashedCDkey;
		CSteamID SteamID = CSteamID(ullSteamID);
		g_pSM->Format(szSteamID, sizeof(szSteamID), "STEAM_0:%u:%u", (SteamID.GetAccountID() % 2) ? 1 : 0, (int32)(SteamID.GetAccountID()/2));
		g_pConnectForward->PushString(addr.ToString(true));
		g_pConnectForward->PushString(szSteamID);
		g_pConnectForward->PushString(name);
		g_pConnectForward->PushStringEx(rejectReason, sizeof(rejectReason), SM_PARAM_STRING_UTF8 | SM_PARAM_STRING_COPY, SM_PARAM_COPYBACK);

		cell_t result = Pl_Continue;
		g_pConnectForward->Execute(&result);

		if(result >= Pl_Handled)
		{
			RejectConnection((CBaseServer*)this, addr, rejectReason);
			return nullptr;
		}
		
	}

	return DETOUR_MEMBER_CALL(ConnectClient)(addr, protocol, challenge, nAuthProtocol, name, password, hashedCDkey, cdKeyLen);
}

namespace PreConnectHook {
	void DestroyDetour() {
		forwards->ReleaseForward(g_pConnectForward);

		if (g_ConnectClientDetour != nullptr) {
			g_ConnectClientDetour->Destroy();
			g_ConnectClientDetour = nullptr;
		}
	}
	
	void SDK_OnAllLoaded(IGameConfig *g_pGameConf) {
		if (g_ConnectClientDetour == nullptr) {
			g_ConnectClientDetour = DETOUR_CREATE_MEMBER(ConnectClient, "CBaseServer::ConnectClient");
			if (g_ConnectClientDetour) g_ConnectClientDetour->EnableDetour();
			else g_pSM->LogError(myself, "Failed to setup CBaseServer::ConnectClient detour");
			
			if (!g_pGameConf->GetMemSig("CBaseServer::RejectConnection", (void **)(&RejectConnection)) || !RejectConnection) {
				g_pSM->LogError(myself, "Failed to get structure CBaseServer::RejectConnection");
				DestroyDetour();
			}
		}

		g_pConnectForward = forwards->CreateForward("OnClientPreConnect", ET_Event, 4, nullptr, Param_String, Param_String, Param_String, Param_String);
	}

	void SDK_OnUnload() {
		DestroyDetour();
	}
}