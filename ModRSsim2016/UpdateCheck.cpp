// UpdateCheck.cpp
// Mod_RSSim (c) Embedded Intelligence Ltd. 1993,2009
// AUTHOR: Conrad Braam.  http://www.plcsimulator.org
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU Affero General Public License as
//    published by the Free Software Foundation, either version 3 of the
//    License, or (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU Affero General Public License for more details.
//
//    You should have received a copy of the GNU Affero General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UpdateCheck.h"
#include "resource.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUpdateCheck::CUpdateCheck()
{

}

CUpdateCheck::~CUpdateCheck()
{
}

BOOL CUpdateCheck::GetFileVersion(DWORD &dwMS, DWORD &dwLS)
{
	TCHAR szModuleFileName[MAX_PATH];

    LPBYTE  lpVersionData; 

	if (GetModuleFileName(AfxGetInstanceHandle(), szModuleFileName, sizeof(szModuleFileName)) == 0) return FALSE;

    DWORD dwHandle;     
    DWORD dwDataSize = ::GetFileVersionInfoSize(szModuleFileName, &dwHandle); 
    if ( dwDataSize == 0 ) 
        return FALSE;

    lpVersionData = new BYTE[dwDataSize]; 
    if (!::GetFileVersionInfo(szModuleFileName, dwHandle, dwDataSize, (void**)lpVersionData) )
    {
		delete [] lpVersionData;
        return FALSE;
    }

    ASSERT(lpVersionData != NULL);

    UINT nQuerySize;
	VS_FIXEDFILEINFO* pVsffi;
    if ( ::VerQueryValue((void **)lpVersionData, _T("\\"),
                         (void**)&pVsffi, &nQuerySize) )
    {
		dwMS = pVsffi->dwFileVersionMS;
		dwLS = pVsffi->dwFileVersionLS;
		delete [] lpVersionData;
        return TRUE;
    }

	delete [] lpVersionData;
    return FALSE;

}

void CUpdateCheck::Check(UINT uiURL)
{
	CString strURL(MAKEINTRESOURCE(uiURL));
	Check(strURL);
}

void CUpdateCheck::Check(const CString& strURL)
{
	DWORD dwMS, dwLS;
	if (!GetFileVersion(dwMS, dwLS))
	{
		ASSERT(FALSE); // Check that application exe has a version resource defined (Open Resource editor, add new version resource)
		return;
	}

	CWaitCursor wait;
	HINTERNET hInet = InternetOpen(UPDATECHECK_BROWSER_STRING, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, NULL);
	HINTERNET hUrl = InternetOpenUrl(hInet, strURL, NULL, -1L,
										 INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE |
										 INTERNET_FLAG_NO_CACHE_WRITE |WININET_API_FLAG_ASYNC, NULL);
	if (hUrl)
	{
		char szBuffer[6*1024];		// Increased Buffer Size from 512 on 2015-07-26 by DL
		DWORD dwRead;
		if (InternetReadWholeFile(hUrl, szBuffer, sizeof(szBuffer), &dwRead))
		{
			if (dwRead > 0)
			{
				szBuffer[dwRead] = 0;
				CString strSubMS1;
				CString strSubMS2;
				CString strSubLS1;	// Added 2014-07-27 by DL storage for the Least Significant Minor Version
				CString strSubLS2;	// Added 2014-07-27 by DL storage for the Least Significant Minor Version
				CString strSub;
				CString buffer(szBuffer);
				DWORD dwMSWeb;
				DWORD dwLSWeb;	// Added 2014-07-27 by DL this gets the Least Significant Version
				int pos=0;
				//strSubMS1 = buffer.Tokenize("|", pos);		// Deleted 2015-07-26 by DL
				//strSubMS2 = buffer.Tokenize("|", pos);		// Deleted 2015-07-26 by DL
				strSubMS1 = buffer.Tokenize("V", pos);	// Added 2014-07-26 by DL this finds the 'V' in 'Version'
				strSubMS1 = buffer.Tokenize(" ", pos);	// Added 2014-07-26 by DL this advances past the 'Version' to the numbers
				strSubMS1 = buffer.Tokenize(".", pos);	// Added 2014-07-26 by DL this gets the Most Significant Major Version
				strSubMS2 = buffer.Tokenize(".", pos);	// Added 2014-07-26 by DL this gets the Most Significant Minor Version
				strSubLS1 = buffer.Tokenize(".", pos);	// Added 2014-07-27 by DL this gets the Least Significant Major Version
				strSubLS2 = buffer.Tokenize(".", pos);	// Added 2014-07-27 by DL this gets the Least Significant Minor Version
				dwMSWeb = MAKELONG((WORD) atol(strSubMS2), (WORD) atol(strSubMS1));
				dwLSWeb = MAKELONG((WORD) atol(strSubLS2), (WORD) atol(strSubLS1));	// Added 2014-07-27 by DL Calc dwLSWeb

				if (dwMSWeb > dwMS)
				{
					//strSub = buffer.Tokenize("|", pos);						// Deleted 2015-07-26 by DL
					strSub = "http://sourceforge.net/projects/modrssim2";		// Added 2015-07-26 by DL
					MsgUpdateAvailable(dwMS, dwLS, dwMSWeb, dwLSWeb, strSub);	// Revised 2015-07-27 by DL for Least Significant digits
				}
				else if ((dwMSWeb == dwMS) && (dwLSWeb > dwLS))	// Added 2015-07-26 by DL to handle only Least Significant digit update
				{																// Added 2015-07-26 by DL
					strSub = "http://sourceforge.net/projects/modrssim2";		// Added 2015-07-27 by DL
					MsgUpdateAvailable(dwMS, dwLS, dwMSWeb, dwLSWeb, strSub);	// Added 2015-07-27 by DL for Least Significant digits
				}																// Added 2015-07-26 by DL
				else
					MsgUpdateNotAvailable(dwMS, dwLS);
			}
			else
				MsgUpdateNoCheck(dwMS, dwLS);

		}
		InternetCloseHandle(hUrl);
	}
	else
		MsgUpdateNoCheck(dwMS, dwLS);

	InternetCloseHandle(hInet);
}

HINSTANCE CUpdateCheck::GotoURL(LPCTSTR url, int showcmd)
{
    TCHAR key[MAX_PATH + MAX_PATH];

    // First try ShellExecute()
    HINSTANCE result = ShellExecute(NULL, _T("open"), url, NULL,NULL, showcmd);

    // If it failed, get the .htm regkey and lookup the program
    if ((UINT)result <= HINSTANCE_ERROR) 
	{

        if (GetRegKey(HKEY_CLASSES_ROOT, _T(".htm"), key) == ERROR_SUCCESS) 
		{
            lstrcat(key, _T("\\shell\\open\\command"));

            if (GetRegKey(HKEY_CLASSES_ROOT,key,key) == ERROR_SUCCESS) 
			{
                TCHAR *pos;
                pos = _tcsstr(key, _T("\"%1\""));
                if (pos == NULL) {                     // No quotes found
                    pos = _tcsstr(key, _T("%1"));      // Check for %1, without quotes 
                    if (pos == NULL)                   // No parameter at all...
                        pos = key+lstrlen(key)-1;
                    else
                        *pos = '\0';                   // Remove the parameter
                }
                else
                    *pos = '\0';                       // Remove the parameter

                lstrcat(pos, _T(" "));
                lstrcat(pos, url);
				CString csKey(key);
                result = (HINSTANCE) WinExec((LPSTR)((LPCTSTR)csKey), showcmd);
            }
        }
    }

    return result;
}

LONG CUpdateCheck::GetRegKey(HKEY key, LPCTSTR subkey, LPTSTR retdata)
{
    HKEY hkey;
    LONG retval = RegOpenKeyEx(key, subkey, 0, KEY_QUERY_VALUE, &hkey);

    if (retval == ERROR_SUCCESS) 
	{
        long datasize = MAX_PATH;
        TCHAR data[MAX_PATH];
        RegQueryValue(hkey, NULL, data, &datasize);
        lstrcpy(retdata,data);
        RegCloseKey(hkey);
    }

    return retval;
}

/**********************************************************/
// this patch provided by 	John-Lucas Brown
bool CUpdateCheck::InternetReadWholeFile(HINTERNET hUrl,LPVOID lpBuffer,DWORD dwNumberOfBytesToRead,LPDWORD lpNumberOfBytesRead)
{
DWORD dwRead=0;
BYTE szBuffer[512];
memset(lpBuffer,0,dwNumberOfBytesToRead);
*lpNumberOfBytesRead = 0;
do{
if (!InternetReadFile(hUrl,szBuffer,sizeof(szBuffer),&dwRead))
dwRead = 0;
if (dwRead!=0){
if (*lpNumberOfBytesRead + dwRead > dwNumberOfBytesToRead) return false;//too much data for the buffer
memcpy(&((char *)lpBuffer)[*lpNumberOfBytesRead],szBuffer,dwRead);
*lpNumberOfBytesRead+=dwRead;
}
}while (dwRead != 0 && strchr((char *)lpBuffer,EOF)==NULL);
return strchr((char *)lpBuffer,EOF)==NULL;
}

// override these methods as needed.
void CUpdateCheck::MsgUpdateAvailable(DWORD dwMSlocal, DWORD dwLSlocal, DWORD dwMSWeb, DWORD dwLSWeb, const CString& strURL)
{
	CString strMessage;
	// Added Least Significant (LS) to both local and Web versionings for statement below on 2015-07-27 by DL
	strMessage.Format(IDS_UPDATE_AVAILABLE, HIWORD(dwMSlocal), LOWORD(dwMSlocal), 
		HIWORD(dwLSlocal), LOWORD(dwLSlocal), HIWORD(dwMSWeb), LOWORD(dwMSWeb),HIWORD(dwLSWeb), LOWORD(dwLSWeb));

	if (AfxMessageBox(strMessage, MB_YESNO|MB_ICONINFORMATION) == IDYES)
		GotoURL(strURL, SW_SHOW);
}

void CUpdateCheck::MsgUpdateNotAvailable(DWORD dwMSlocal, DWORD dwLSlocal)
{
	UNREFERENCED_PARAMETER(dwMSlocal);
	UNREFERENCED_PARAMETER(dwLSlocal);
	AfxMessageBox(IDS_UPDATE_NO, MB_OK|MB_ICONINFORMATION);
}

void CUpdateCheck::MsgUpdateNoCheck(DWORD dwMSlocal, DWORD dwLSlocal)
{
	UNREFERENCED_PARAMETER(dwMSlocal);
	UNREFERENCED_PARAMETER(dwLSlocal);
	AfxMessageBox(IDS_UPDATE_NOCHECK, MB_OK|MB_ICONINFORMATION);
}
