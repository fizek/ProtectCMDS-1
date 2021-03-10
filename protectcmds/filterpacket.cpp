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

#define QUERY_HEADER "\xFF\xFF\xFF\xFF"
#define NORMAL_SEQ_PACKET "\x00"

int (*g_real_recvfrom_ptr)(int , char *, int , int , struct sockaddr *, int *);
bool g_brecvfrom_hooked = false;

CDetour *g_FilterPacketDetour = nullptr;

void UnHookRecvFrom() {
	if(g_brecvfrom_hooked) {
		g_pVCR->Hook_recvfrom = g_real_recvfrom_ptr;
		g_brecvfrom_hooked = false;
	}
}

int MyRecvFromHook(int s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen) {
	int ret = g_real_recvfrom_ptr(s,buf,len,flags,from,fromlen);
	
	if (ret >= 0) {
		if ( !memcmp(buf+3, NORMAL_SEQ_PACKET, 1) ) return ret;
		else if ( !memcmp(buf, QUERY_HEADER, 4) ) return ret;
		
		buf[0] = '\x00';
		
		return 1;
	}
	
	return -1;
}

void ReHookRecvFrom() {
	if(!g_brecvfrom_hooked) {
		g_real_recvfrom_ptr = g_pVCR->Hook_recvfrom;
		g_pVCR->Hook_recvfrom = &MyRecvFromHook;
		g_brecvfrom_hooked = true;
	}
}

namespace FilterPacket {
	void SDK_OnLoad() {
		ReHookRecvFrom();
	}

	void SDK_OnUnload() {
		UnHookRecvFrom();
	}
}