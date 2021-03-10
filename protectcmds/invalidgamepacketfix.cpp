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
 
class CNetChan : public INetMessage {};
 
CDetour *g_ProcessMessagesDetour = nullptr;

/*Start Function Pointer*/
//bool (__cdecl* ProcessControlMessage)(void *CNetChanThis, int cmd, bf_read &buf);
INetMessage *(__cdecl* FindMessage)(void *CNetChanThis, int cmd);
void (__cdecl* UpdateMessageStats)(void *CNetChanThis, int msgroup, int bits);
bool (__cdecl* IsOverflowed)(void *CNetChanThis);
/*End Function Pointer*/

bool IsValidFileForTransfer( const char *pszFilename )
{
	if ( !pszFilename || !pszFilename[ 0 ] )
		return false;

	// No absolute paths or weaseling up the tree with ".." allowed.
	if ( !COM_IsValidPath( pszFilename ) || V_IsAbsolutePath( pszFilename ) )
		return false;

	int len = V_strlen( pszFilename );
	if ( len >= MAX_PATH )
		return false;

	char szTemp[ MAX_PATH ];
	V_strcpy_safe( szTemp, pszFilename );

	// Convert so we've got all forward slashes in the path.
	V_FixSlashes( szTemp, '/' );
	V_FixDoubleSlashes( szTemp );
	if ( szTemp[ len - 1 ] == '/' )
		return false;

	int slash_count = 0;
	for ( const char *psz = szTemp; *psz; psz++ )
	{
		if ( *psz == '/' )
			slash_count++;
	}
	// Really no reason to have deeper directory than this?
	if ( slash_count >= 32 )
		return false;

	// Don't allow filenames with unicode whitespace in them.
	if ( Q_RemoveAllEvilCharacters( szTemp ) )
		return false;

	if ( V_stristr( szTemp, "lua/" ) ||
	     V_stristr( szTemp, "gamemodes/" ) ||
	     V_stristr( szTemp, "addons/" ) ||
	     V_stristr( szTemp, "~/" ) ||
	     // V_stristr( szTemp, "//" ) || 		// Don't allow '//'. TODO: Is this check ok?
	     V_stristr( szTemp, "./././" ) ||	// Don't allow folks to make crazy long paths with ././././ stuff.
	     V_stristr( szTemp, "   " ) ||		// Don't allow multiple spaces or tab (was being used for an exploit).
	     V_stristr( szTemp, "\t" ) )
	{
		return false;
	}

	// If .exe or .EXE or these other strings exist _anywhere_ in the filename, reject it.
	if ( V_stristr( szTemp, ".cfg" ) ||
	     V_stristr( szTemp, ".lst" ) ||
	     V_stristr( szTemp, ".exe" ) ||
	     V_stristr( szTemp, ".vbs" ) ||
	     V_stristr( szTemp, ".com" ) ||
	     V_stristr( szTemp, ".bat" ) ||
	     V_stristr( szTemp, ".cmd" ) ||
	     V_stristr( szTemp, ".dll" ) ||
	     V_stristr( szTemp, ".so" ) ||
	     V_stristr( szTemp, ".dylib" ) ||
	     V_stristr( szTemp, ".ini" ) ||
	     V_stristr( szTemp, ".log" ) ||
	     V_stristr( szTemp, ".lua" ) ||
	     V_stristr( szTemp, ".vdf" ) ||
	     V_stristr( szTemp, ".smx" ) ||
	     V_stristr( szTemp, ".gcf" ) ||
	     V_stristr( szTemp, ".lmp" ) ||
	     V_stristr( szTemp, ".sys" ) )
	{
		return false;
	}

	// Search for the first . in the base filename, and bail if not found.
	// We don't want people passing in things like 'cfg/.wp.so'...
	const char *basename = strrchr( szTemp, '/' );
	if ( !basename )
		basename = szTemp;
	const char *extension = strchr( basename, '.' );
	if ( !extension )
		return false;

	// If the extension is not exactly 3 or 4 characters, bail.
	int extension_len = V_strlen( extension );
	if ( ( extension_len != 3 ) &&
	     ( extension_len != 4 ) &&
	     V_stricmp( extension, ".bsp.bz2" ) &&
	     V_stricmp( extension, ".xbox.vtx" ) &&
	     V_stricmp( extension, ".dx80.vtx" ) &&
	     V_stricmp( extension, ".dx90.vtx" ) &&
	     V_stricmp( extension, ".sw.vtx" ) )
	{
		return false;
	}

	// If there are any spaces in the extension, bail. (Windows exploit).
	if ( strchr( extension, ' ' ) )
		return false;

	return true;
}

/*static bool IsSafeFileToDownload( const char *pFilename ) {
	// No absolute paths or weaseling up the tree with ".." allowed.
	if ( V_strstr( pFilename, ":" ) || V_strstr( pFilename, ".." ) ) return false;		

	// Only files with 3-letter extensions allowed.
	const char *pExt = V_strrchr( pFilename, '.' );
	if ( !pExt || V_strlen( pExt ) != 4 ) return false;

	// Don't allow any of these extensions.
	if ( V_stricmp( pExt, ".cfg" ) == 0
		|| V_stricmp( pExt, ".lst" ) == 0
		|| V_stricmp( pExt, ".exe" ) == 0
		|| V_stricmp( pExt, ".vbs" ) == 0
		|| V_stricmp( pExt, ".com" ) == 0
		|| V_stricmp( pExt, ".bat" ) == 0
		|| V_stricmp( pExt, ".dll" ) == 0
		|| V_stricmp( pExt, ".ini" ) == 0
		|| V_stricmp( pExt, ".log" ) == 0 )
	{
		return false;
	}

	// Word.
	return true;
}*/

bool ProcessControlMessage( void *CNetChanThis, int cmd, bf_read &buf) {
	char string[1024];
	
	if (cmd == net_NOP) return true;
	
	if (cmd == net_Disconnect) {
		//buf.ReadString( string, sizeof(string) );
		
		INetChannelHandler *m_MessageHandler = *(INetChannelHandler **)( (uintptr_t)CNetChanThis + 7648 );
		m_MessageHandler->ConnectionCrashed( "Disconnected" );
		
		return false;
	}
	
	if (cmd == net_File) {
		unsigned int transferID = buf.ReadUBitLong( 32 );

		buf.ReadString( string, sizeof(string) );

		//if ( buf.ReadOneBit() != 0 && IsSafeFileToDownload( string ) ) {
		if ( buf.ReadOneBit() != 0 && IsValidFileForTransfer( string ) ) {
			INetChannelHandler *m_MessageHandler = *(INetChannelHandler **)( (uintptr_t)CNetChanThis + 7648 );
			m_MessageHandler->FileRequested( string, transferID );
		}
		else {
			INetChannelHandler *m_MessageHandler = *(INetChannelHandler **)( (uintptr_t)CNetChanThis + 7648 );
			m_MessageHandler->FileDenied( string, transferID );
		}
		
		return true;
	}
	
	netadr_s remote_address = *(netadr_s *)( (uintptr_t)CNetChanThis + 116 );
	ConMsg(0, "Netchannel: received bad control cmd %i from %s.\n", cmd, remote_address.ToString() );
	return false;
}

DETOUR_DECL_MEMBER1(ProcessMessages, bool, bf_read &, buf) {
	int startbit = buf.GetNumBitsRead();
	while ( true ) {
		if ( buf.IsOverflowed() ) {
			INetChannelHandler *m_MessageHandler = *(INetChannelHandler **)( (uintptr_t)this + 7648 );
			m_MessageHandler->ConnectionCrashed( "[ProtectCMDS] Buffer overflow in net message" );
			
			return false;
		}	

		if ( buf.GetNumBitsLeft() < NETMSG_TYPE_BITS ) break;
		unsigned char cmd = buf.ReadUBitLong( NETMSG_TYPE_BITS );
		if ( cmd <= net_File ) {
			if ( !ProcessControlMessage( this, cmd, buf ) ) return false; // disconnect or error

			continue;
		}

		INetMessage	* netmsg = FindMessage( this, cmd );
		if ( netmsg ) {
			const char *msgname = netmsg->GetName();
			
			int startbit = buf.GetNumBitsRead();

			if ( !netmsg->ReadFromBuffer( buf ) ) {
				/*netadr_s remote_address = *(netadr_s *)( (uintptr_t)this + 116 );
				ConMsg(0, "Netchannel: failed reading message %s from %s.\n", msgname, remote_address.ToString() );*/
				
				INetChannelHandler *m_MessageHandler = *(INetChannelHandler **)( (uintptr_t)this + 7648 );
				
				if (m_MessageHandler) m_MessageHandler->ConnectionCrashed( "2[ProtectCMDS] Buffer overflow in net message" );
			
				return false;
			}

			UpdateMessageStats( this, netmsg->GetGroup(), buf.GetNumBitsRead() - startbit );
			
			bool *m_bProcessingMessages = (bool*)((uintptr_t)this + 4);
			*m_bProcessingMessages = true;
			bool bRet = netmsg->Process();
			*m_bProcessingMessages = false;

			bool m_bShouldDelete = *(bool*)((uintptr_t)this + 5);
			if ( m_bShouldDelete ) {
				delete this;
				return false;
			}

			if ( !bRet ) {
				//ConDMsg(0, "Netchannel: failed processing message %s.\n", msgname );
				return false;
			}

			if ( IsOverflowed(this) ) return false;
		}
		else {
			/*netadr_s remote_address = *(netadr_s *)( (uintptr_t)this + 116 );
			ConMsg(0, "Netchannel: unknown net message (%i) from %s.\n", cmd, remote_address.ToString() );*/
			
			INetChannelHandler *m_MessageHandler = *(INetChannelHandler **)( (uintptr_t)this + 7648 );
			
			if (m_MessageHandler) m_MessageHandler->ConnectionCrashed( "[ProtectCMDS] Buffer overflow in net message" );
			
			return false;
		}
	}
	
	return true;
	
	//return DETOUR_MEMBER_CALL(ProcessMessages)(buf);
}

namespace InvalidGamePacketFix {
	void DestroyDetour() {
		if (g_ProcessMessagesDetour != nullptr) {
			g_ProcessMessagesDetour->Destroy();
			g_ProcessMessagesDetour = nullptr;
		}
	}
	
	void SDK_OnAllLoaded(IGameConfig *g_pGameConf) {
		if (g_ProcessMessagesDetour == nullptr) {
			g_ProcessMessagesDetour = DETOUR_CREATE_MEMBER(ProcessMessages, "CNetChan::ProcessMessages");
			if (g_ProcessMessagesDetour) g_ProcessMessagesDetour->EnableDetour();
			else g_pSM->LogError(myself, "Failed to setup CNetChan::ProcessMessages detour");
			
			bool bErrorGetStructure = false;
				
			if (!g_pGameConf->GetMemSig("CNetChan::FindMessage", (void**)&FindMessage) || FindMessage == nullptr) {
				g_pSM->LogError(myself, "Failed to get structure CNetChan::FindMessage detour");
				bErrorGetStructure = true;
			}
				
			if (!g_pGameConf->GetMemSig("CNetChan::UpdateMessageStats", (void**)&UpdateMessageStats) || UpdateMessageStats == nullptr) {
				g_pSM->LogError(myself, "Failed to get structure CNetChan::UpdateMessageStats detour");
				bErrorGetStructure = true;
			}
				
			if (!g_pGameConf->GetMemSig("CNetChan::IsOverflowed", (void**)&IsOverflowed) || IsOverflowed == nullptr) {
				g_pSM->LogError(myself, "Failed to get structure CNetChan::IsOverflowed detour");
				bErrorGetStructure = true;
			}
				
			if (bErrorGetStructure) {
				DestroyDetour();
				g_pSM->LogError(myself, "Invalid Game Packet Fix not worked!");
			}
		}
	}

	void SDK_OnUnload() {
		DestroyDetour();
	}
}