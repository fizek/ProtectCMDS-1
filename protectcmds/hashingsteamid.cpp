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

 
CDetour *g_SetSteamIDDetour = nullptr;

DETOUR_DECL_MEMBER1(SetSteamID, void *, CSteamID &, inputSteamID) {
	CSteamID SteamID(inputSteamID);
	
	/*uint32 ulSteamID = ullSteamID & 0xFFFFFFFF;
	printf("Pre m_unAccountInstance = %u\n", SteamID.GetUnAccountInstance());
	printf("Pre m_EAccountType = %u\n", SteamID.GetEAccountType());
	printf("Pre m_EUniverse = %d\n", SteamID.GetEUniverse());
	
	printf("Pre GetAccountID = %u\n", SteamID.GetAccountID());*/
	
	char szSteamID_old[64], szSteamID_new[64];
	
	g_pSM->Format(szSteamID_old, sizeof(szSteamID_old), "STEAM_0:%u:%u", (SteamID.GetAccountID() % 2) ? 1 : 0, (int32)(SteamID.GetAccountID()/2));
	
	SteamID.SetAccountID(SteamID.GetAccountID() + 3);
	
	g_pSM->Format(szSteamID_new, sizeof(szSteamID_new), "STEAM_0:%u:%u", (SteamID.GetAccountID() % 2) ? 1 : 0, (int32)(SteamID.GetAccountID()/2));
	
	printf("%s STEAM: [%s] changed to [%s]\n", g_cPrefix, szSteamID_old, szSteamID_new);
	
	inputSteamID = SteamID;
	
	return DETOUR_MEMBER_CALL(SetSteamID)(inputSteamID);
}

namespace HashingSteamID {
	void SDK_OnAllLoaded(IGameConfig *g_pGameConf) {
		if (g_SetSteamIDDetour == nullptr) {
			g_SetSteamIDDetour = DETOUR_CREATE_MEMBER(SetSteamID, "CBaseServer::SetSteamID");
			if (g_SetSteamIDDetour) g_SetSteamIDDetour->EnableDetour();
			else g_pSM->LogError(myself, "Failed to setup CBaseServer::SetSteamID detour");
			
			
		}
	}

	void SDK_OnUnload() {
		if (g_SetSteamIDDetour != nullptr) {
			g_SetSteamIDDetour->Destroy();
			g_SetSteamIDDetour = nullptr;
		}
	}
}