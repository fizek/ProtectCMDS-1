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
 
CDetour *g_GameUIHookDetour = nullptr;

DETOUR_DECL_MEMBER1(GameUI, void *, CBaseEntity*, pActivator) {
	CBaseHandle *m_player_hndl = (CBaseHandle*)( (uintptr_t)this + 1316);

	if(!m_player_hndl->IsValid()) {
		ConMsg(0, "%s prevented server crash with CGameUI::Deactivate\n", g_cPrefix);
		return nullptr;
	}
	
	return DETOUR_MEMBER_CALL(GameUI)(pActivator);
}

namespace GameUIFix {
	void SDK_OnAllLoaded(IGameConfig *g_pGameConf) {
		if (g_GameUIHookDetour == nullptr) {
			g_GameUIHookDetour = DETOUR_CREATE_MEMBER(GameUI, "CGameUI::Deactivate");
			if (g_GameUIHookDetour) g_GameUIHookDetour->EnableDetour();
			else g_pSM->LogError(myself, "Failed to setup GameUI::Deactivate detour");
		}
	}

	void SDK_OnUnload() {
		if (g_GameUIHookDetour != nullptr) {
			g_GameUIHookDetour->Destroy();
			g_GameUIHookDetour = nullptr;
		}
	}
}