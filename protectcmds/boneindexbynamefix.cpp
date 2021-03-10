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

CDetour *g_BoneIndexByNameDetour = nullptr;

DETOUR_DECL_MEMBER2(BoneIndexByName, int, CStudioHdr *, pStudioHdr, const char *, szName) {
	register int EAX asm("eax");
	register int ECX asm("ecx");

	if (!EAX || !ECX || !pStudioHdr) {
		ConMsg(0, "%s prevented server crash with CBaseAnimating::Studio_BoneIndexByName\n", g_cPrefix);
		return -1;
	}
	
	return DETOUR_MEMBER_CALL(BoneIndexByName)(pStudioHdr, szName);
}

namespace BoneIndexByNameFix {
	void SDK_OnAllLoaded(IGameConfig *g_pGameConf) {
		if (g_BoneIndexByNameDetour == nullptr) {
			g_BoneIndexByNameDetour = DETOUR_CREATE_MEMBER(BoneIndexByName, "CBaseAnimating::Studio_BoneIndexByName");
			if (g_BoneIndexByNameDetour) g_BoneIndexByNameDetour->EnableDetour();
			else g_pSM->LogError(myself, "Failed to setup CBaseAnimating::BoneIndexByName detour");
		}
	}

	void SDK_OnUnload() {
		if (g_BoneIndexByNameDetour != nullptr) {
			g_BoneIndexByNameDetour->Destroy();
			g_BoneIndexByNameDetour = nullptr;
		}
	}
}