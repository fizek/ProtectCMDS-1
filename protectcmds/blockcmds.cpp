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
extern const char g_cPrefix[];
 
CDetour *g_ExecuteStringCommandDetour = nullptr;

int g_iMaxUserCmds = 30;
float g_fLastUserCmds[MAXPLAYERS+1];
int g_iUserCmds[MAXPLAYERS+1];

DETOUR_DECL_MEMBER1(ExecuteStringCommand, bool, char *, pCommand) {
	if (strlen(pCommand) > 1024) return false;
	
	CBaseClient *CBClient = reinterpret_cast<CBaseClient*>(this);
	int iClient = CBClient->GetPlayerSlot() + 1;
	IGamePlayer *pClient = playerhelpers->GetGamePlayer(iClient);
	
	if (!pClient || !pClient->IsConnected()) return false;
	
	float fCurrentUserCmdsTime = engine->Time();
	if (g_fLastUserCmds[iClient]+1.0 > fCurrentUserCmdsTime) {
		g_iUserCmds[iClient]++;
		if (g_iUserCmds[iClient] >= g_iMaxUserCmds) {
			pClient->Kick("[ProtectCMDS] buffer overflow in net message");
			return false;
		}
	}
	else {
		g_iUserCmds[iClient] = 1;
		g_fLastUserCmds[iClient] = fCurrentUserCmdsTime;
	}
	if ( StrContains(pCommand, "sm ", false) == 0 || StrContains(pCommand, "meta ", false) == 0) {
		char Msg[1100];
		TrimString(pCommand);
		UTIL_Format(Msg, sizeof(Msg), "%s the command \"%s\" is blocked\n", g_cPrefix, pCommand);
		pClient->PrintToConsole(Msg);
		return false;
	}
	
	return DETOUR_MEMBER_CALL(ExecuteStringCommand)(pCommand);
}

namespace BlockCmds {
	void SDK_OnAllLoaded(IGameConfig *g_pGameConf) {
		if (g_ExecuteStringCommandDetour == nullptr) {
			g_ExecuteStringCommandDetour = DETOUR_CREATE_MEMBER(ExecuteStringCommand, "CGameClient::ExecuteStringCommand");
			if (g_ExecuteStringCommandDetour) g_ExecuteStringCommandDetour->EnableDetour();
			else g_pSM->LogError(myself, "Failed to setup CGameClient::ExecuteStringCommand detour");
		}
	}

	void SDK_OnUnload() {
		if (g_ExecuteStringCommandDetour != nullptr) {
			g_ExecuteStringCommandDetour->Destroy();
			g_ExecuteStringCommandDetour = nullptr;
		}
	}
}