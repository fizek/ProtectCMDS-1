bool Q_IsMeanSpaceW( wchar_t wch ) {
	bool bIsMean = false;

	switch ( wch ) {
	case L'\x0082':	  // BREAK PERMITTED HERE
	case L'\x0083':	  // NO BREAK PERMITTED HERE
	case L'\x00A0':	  // NO-BREAK SPACE
	case L'\x034F':   // COMBINING GRAPHEME JOINER
	case L'\x2000':   // EN QUAD
	case L'\x2001':   // EM QUAD
	case L'\x2002':   // EN SPACE
	case L'\x2003':   // EM SPACE
	case L'\x2004':   // THICK SPACE
	case L'\x2005':   // MID SPACE
	case L'\x2006':   // SIX SPACE
	case L'\x2007':   // figure space
	case L'\x2008':   // PUNCTUATION SPACE
	case L'\x2009':   // THIN SPACE
	case L'\x200A':   // HAIR SPACE
	case L'\x200B':   // ZERO-WIDTH SPACE
	case L'\x200C':   // ZERO-WIDTH NON-JOINER
	case L'\x200D':   // ZERO WIDTH JOINER
	case L'\x200E':	  // LEFT-TO-RIGHT MARK
	case L'\x2028':   // LINE SEPARATOR
	case L'\x2029':   // PARAGRAPH SEPARATOR
	case L'\x202F':   // NARROW NO-BREAK SPACE
	case L'\x2060':   // word joiner
	case L'\xFEFF':   // ZERO-WIDTH NO BREAK SPACE
	case L'\xFFFC':   // OBJECT REPLACEMENT CHARACTER
		bIsMean = true;
		break;
	}

	return bIsMean;
}

bool Q_RemoveAllEvilCharacters( char *pch ) {
	// convert to unicode
	int cch = Q_strlen( pch );
	int cubDest = (cch + 1 ) * sizeof( wchar_t );
	wchar_t *pwch = (wchar_t *)stackalloc( cubDest );
	int cwch = Q_UTF8ToUnicode( pch, pwch, cubDest ) / sizeof( wchar_t );

	bool bStrippedWhitespace = false;

	// Walk through and skip over evil characters
	int nWalk = 0;
	for( int i=0; i<cwch; ++i ) {
		if( !Q_IsMeanSpaceW( pwch[i] ) ) {
			pwch[nWalk] = pwch[i];
			++nWalk;
		}
		else bStrippedWhitespace = true;
	}

	// Null terminate
	pwch[nWalk-1] = L'\0';
	

	// copy back, if necessary
	if ( bStrippedWhitespace ) Q_UnicodeToUTF8( pwch, pch, cch );

	return bStrippedWhitespace;
}

void V_FixDoubleSlashes( char *pStr ) {
	int len = V_strlen( pStr );

	for ( int i=1; i < len-1; i++ ) {
		if ( (pStr[i] == '/' || pStr[i] == '\\') && (pStr[i+1] == '/' || pStr[i+1] == '\\') ) {
			// This means there's a double slash somewhere past the start of the filename. That 
			// can happen in Hammer if they use a material in the root directory. You'll get a filename 
			// that looks like 'materials\\blah.vmt'
			V_memmove( &pStr[i], &pStr[i+1], len - i );
			--len;
		}
	}
}

/*void V_strncpy( char *pDest, char const *pSrc, int maxLen ) {
	Assert( maxLen >= sizeof( *pDest ) );
	AssertValidWritePtr( pDest, maxLen );
	AssertValidStringPtr( pSrc );

	strncpy( pDest, pSrc, maxLen );
	if ( maxLen > 0 )
	{
		pDest[maxLen-1] = 0;
	}
}*/

template <size_t maxLenInChars> void V_strcpy_safe( char (&pDest)[maxLenInChars], const char *pSrc ) 
{ 
	V_strncpy( pDest, pSrc, (int)maxLenInChars ); 
}

bool COM_IsValidPath( const char *pszFilename ) {
	if ( !pszFilename ) return false;

	if ( Q_strlen( pszFilename ) <= 0    ||
		Q_strstr( pszFilename, "\\\\" ) ||	// to protect network paths
		Q_strstr( pszFilename, ":" )    || // to protect absolute paths
		Q_strstr( pszFilename, ".." ) ||   // to protect relative paths
		Q_strstr( pszFilename, "\n" ) ||   // CFileSystem_Stdio::FS_fopen doesn't allow this
		Q_strstr( pszFilename, "\r" ) )    // CFileSystem_Stdio::FS_fopen doesn't allow this
	{
		return false;
	}

	return true;
}

/*bool V_IsAbsolutePath( const char *pStr ) {
	bool bIsAbsolute = ( pStr[0] && pStr[1] == ':' ) || pStr[0] == '/' || pStr[0] == '\\';
	if ( !bIsAbsolute ) bIsAbsolute = ( V_stristr( pStr, ":" ) != NULL );

	return bIsAbsolute;
}*/

void CharToLower(char *CStr) {
	int iLen = strlen(CStr);
	char c;
	for (int i = 0; i < iLen; i++)  CStr[i] = tolower(CStr[i]);
}

bool StrEqual(const char *Str1, const char *Str2, bool bCaseSensitive) {
	char CString1[1024], CString2[1024];
	
	strcpy(CString1, Str1);
	strcpy(CString2, Str2);
	
	if (!bCaseSensitive) {
		CharToLower(CString1);
		CharToLower(CString2);
	}
	
	if ( !strcmp(CString1, CString2) ) return true;
	
	return false;
}

int Contains(const char *Str1, const char *Str2) {
	//ConMsg(0, "Строки: %s и %s\n", Str1, Str2);
	int iResult = -1;
	int iLen = strlen(Str1);
	unsigned int iLenStr2 = strlen(Str2);
	
	unsigned int j = 0; //Количество совпадений
	for (int i = 0; i < iLen; i++) {		
		//ConMsg(0, "Сравнение %c == %c ? || j = %d\n", Str1[i], Str2[j], j);
		
		//Совпадение
		if (Str1[i] == Str2[j]) {
			iResult = i;
			//ConMsg(0, "Сравнение %c == %c ? || j = %d\n", Str1[i], Str2[j], j);
			j++;
			unsigned int jj = j;
			for (int ii = i+1; ii < iLen; ii++) {
				//ConMsg(0, "Сравнение %c == %c ? || jj = %d\n", Str1[ii], Str2[jj], jj);
				
				if (Str1[ii] == Str2[jj]) {
					jj++;
					if (iLenStr2 == jj) return iResult;
				}
				else break;
				
				if (Str2[jj] == '\0') break;
				
				i = ii;
			}
			
			j = 0;
		}
		
		if (Str2[j] == '\0') break;
	}
	
	/*ConMsg(0, "Размеры: strlen(Str2) = %d || j = %d\n", strlen(Str2), j);
	if (strlen(Str2) != j) iResult = -1;
	
	ConMsg(0, "return iResult = %d\n", iResult);
	
	return iResult;*/
	
	return -1;
}

int StrContains(const char *Str1, const char *Str2, bool bCaseSensitive) {
	int iResult = 1;
	if (!bCaseSensitive) {
		char CString1[1024], CString2[1024];
	
		strcpy(CString1, Str1);
		strcpy(CString2, Str2);
		
		CharToLower(CString1);
		CharToLower(CString2);
		
		return Contains(CString1, CString2);
	}
	
	return Contains(Str1, Str2);
}

void TrimString(char *s) {
	int i=0,j;
	int iLen = 0;
	while( (s[i] == ' ') || (s[i] == '\t') || (s[i] == '\n') || (s[i] == '\r')) i++;
	if (i>0) {
		int iLen = strlen(s);
		for(j=0; j < iLen; j++)  s[j]=s[j+i];
		s[j] = '\0';
	}
	iLen = strlen(s);
	i=iLen-1;
	while( (s[i] == ' ') || (s[i] == '\t') || (s[i] == '\n') || (s[i] == '\r'))  i--;
	if (i < (iLen-1)) s[i+1] = '\0';
 }