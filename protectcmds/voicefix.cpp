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
 
CDetour *g_ProcessVoiceDataDetour = nullptr;
IForward *g_pVoiceForward = nullptr;

class CLC_VoiceData : public CNetMessage {
public:
	char offs[12];
	int m_nLength;
};

float g_fLastVoice[MAXPLAYERS+1];
int g_iTickVoice[MAXPLAYERS+1];
int g_iDetectVoiceExploit[MAXPLAYERS+1];
int g_iMaxTickVoice = 9;
int g_iMaxDetectVoiceExploit = 3;

DETOUR_DECL_MEMBER1(ProcessVoiceData, bool, CLC_VoiceData *, msg) {
	CBaseClient *CBClient = reinterpret_cast<CBaseClient*>(this);
	int iClient = CBClient->GetPlayerSlot() + 1;
	IGamePlayer *pClient = playerhelpers->GetGamePlayer(iClient);
	
	if (!pClient || !pClient->IsConnected()) return false;
	
	int iLenMsg = msg->m_nLength >> 3;
	
	//if (iLenMsg > 168) {
	if (iLenMsg > 200) {
		
		pClient->Kick("[ProtectCMDS] buffer overflow in net message");
		
		return false;
	}
	
	float fCurrentVoiceTime = engine->Time();
	//printf("%s iTick = %d | %f\n", g_cPrefix, g_iTickVoice[iClient], fCurrentVoiceTime);
	
	if (g_fLastVoice[iClient]+0.1 > fCurrentVoiceTime) {
		g_iTickVoice[iClient]++;
		if (g_iTickVoice[iClient] >= g_iMaxTickVoice) {
			g_iDetectVoiceExploit[iClient]++;
			//printf("%s Detect = %d\n", g_cPrefix, g_iDetectVoiceExploit[iClient]);
			if (g_iDetectVoiceExploit[iClient] > g_iMaxDetectVoiceExploit) pClient->Kick("[ProtectCMDS] buffer overflow in net message");
			
			g_iTickVoice[iClient] = 1;
			
			return false;
		}
	}
	else {
		g_iTickVoice[iClient] = 1;
		g_iDetectVoiceExploit[iClient] = 0;
		g_fLastVoice[iClient] = fCurrentVoiceTime;
	}
	
	g_pVoiceForward->PushCell(iClient);
	
	cell_t result = Pl_Continue;
	g_pVoiceForward->Execute(&result);

	if(result >= Pl_Handled) return false;
	
	return DETOUR_MEMBER_CALL(ProcessVoiceData)(msg);
}

namespace VoiceFix {
	void SDK_OnAllLoaded(IGameConfig *g_pGameConf) {
		if (g_ProcessVoiceDataDetour == nullptr) {
			g_ProcessVoiceDataDetour = DETOUR_CREATE_MEMBER(ProcessVoiceData, "CGameClient::ProcessVoiceData");
			if (g_ProcessVoiceDataDetour) g_ProcessVoiceDataDetour->EnableDetour();
			else g_pSM->LogError(myself, "Failed to setup CGameClient::ProcessVoiceData detour");
		}

		g_pVoiceForward = forwards->CreateForward("OnClientVoice", ET_Event, 1, nullptr, Param_Cell);
	}

	void SDK_OnUnload() {
		forwards->ReleaseForward(g_pVoiceForward);

		if (g_ProcessVoiceDataDetour != nullptr) {
			g_ProcessVoiceDataDetour->Destroy();
			g_ProcessVoiceDataDetour = nullptr;
		}
	}
}