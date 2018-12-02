#include "stdafx.h"

using namespace std;

// This function is called when REAPER loads or unloads the extension.
// If rec !- nil, the extenstion is being loaded.  If rec == nil, the extension is being UNloaded.
extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec)
{
	if (rec)
	{
		// First, import REAPER's C++ API functions for use in this extension.
		//		Load all listed API functions at once.
		if (REAPERAPI_LoadAPI(rec->GetFunc) != 0)
		{
			// This is the WIN32 / swell MessageBox, not REAPER's API MB.  This should create a separate window that is listed in the taskbar,
			//		and more easily visible behind REAPER's splash screen.
			MessageBox(NULL, "Unable to import default API functions.\n\nNOTE:\nThis extension requires REAPER v5.97 or later.", "ERROR: js_ReaScriptAPI extension", 0);  //fprintf(stderr, "Unable to import API functions.\n");
			return 0;
		}
		//		Load each of the undocumented functions.
		if (!((*(void **)&CoolSB_GetScrollInfo) = (void *)rec->GetFunc("CoolSB_GetScrollInfo")))
		{
			MessageBox(NULL, "Unable to import CoolSB_GetScrollInfo function.", "ERROR: js_ReaScriptAPI extension", 0);
			return 0;
		}
		if (!((*(void **)&AttachWindowTopmostButton) = (void *)rec->GetFunc("AttachWindowTopmostButton")))
		{
			MessageBox(NULL, "Unable to import AttachWindowTopmostButton function.", "ERROR: js_ReaScriptAPI extension", 0);
			return 0;
		}
		if (!((*(void **)&AttachWindowResizeGrip) = (void *)rec->GetFunc("AttachWindowResizeGrip")))
		{
			MessageBox(NULL, "Unable to import AttachWindowResizeGrip function.", "ERROR: js_ReaScriptAPI extension", 0);
			return 0;
		}
		if (!((*(void **)&CoolSB_SetScrollPos) = (void *)rec->GetFunc("CoolSB_SetScrollPos")))
		{
			MessageBox(NULL, "Unable to import CoolSB_SetScrollPos function.", "ERROR: js_ReaScriptAPI extension", 0);
			return 0;
		}

		// Don't know what this does, but apparently it's necessary for the localization functions.
		IMPORT_LOCALIZE_RPLUG(rec);

		// Functions imported, continue initing plugin...
		// Second, export this extension's functions to ReaScript API.

		// Each function's defstring will temporarily be contructed in temp[]
		char temp[10000];
		int i, countZeroes;

		for (APIdef& f : aAPIdefs)
		{
			// REAPER uses a \0-separated string. sprintf cannot print \0, so must temporarily print \r and replace later.
			snprintf(temp, sizeof(temp), "%s\n%s\n%s\n%s", f.ret_val, f.parm_types, f.parm_names, f.help);
			temp[sizeof(temp) - 1] = '\0';
			// Create permanent copy of temp string, so that REAPER can access it later again.
			f.defstring = strdup(temp);
			// Replace the three \r with \0.
			i = 0; countZeroes = 0; while (countZeroes < 3) { if (f.defstring[i] == '\n') { f.defstring[i] = 0; countZeroes++; } i++; }
			// Each function must be registered in three ways:
			// APIdef_... provides for converting parameters to vararg format, and for help text in API
			plugin_register(f.regkey_def, (void*)f.defstring);
			// API_... for exposing to other extensions, and for IDE to recognize and color functions while typing 
			plugin_register(f.regkey_func, f.func);
			// APIvarag_... for exporting to ReaScript API
			plugin_register(f.regkey_vararg, f.func_vararg);
		}

		// Construct mapMsgToWM_ as inverse of mapWM_ToMsg
		for (auto& i : Julian::mapWM_toMsg)
			Julian::mapMsgToWM_.emplace(i.second, i.first);

		return 1; // success
	}
	// Does an extension need to do anything when unloading?  
	// To prevent memort leaks, perhaps try to delete any stuff that may remain in memory?
	// On Windows, LICE bitmaps are automatically destroyed when REAPER quits, but to make extra sure, this function will destroy them explicitly.
	else
		for (LICE_IBitmap* bm : Julian::LICEBitmaps)
			LICE__Destroy(bm);
		return 0;
}


void JS_ReaScriptAPI_Version(double* versionOut)
{
	*versionOut = 0.96;
}


void JS_Localize(const char* USEnglish, const char* LangPackSection, char* translationOut, int translationOut_sz)
{
	const char* trans = __localizeFunc(USEnglish, LangPackSection, 0);
	// Why not use NeedBig in this function?  Because seldom necessary, and I want to inform users about the "trick" to customize buffer size.
	/*
	size_t transLen = strlen(trans);
	bool reallocOK = false;
	if (realloc_cmd_ptr(&translationOutNeedBig, &translationOutNeedBig_sz, transLen))
		if (translationOutNeedBig && translationOutNeedBig_sz == transLen)
			reallocOK = true;
	if (reallocOK)
		memcpy(translationOutNeedBig, trans, transLen);
	else if (translationOutNeedBig && translationOutNeedBig_sz > 0)
		translationOutNeedBig[0] = 0;
	return;
	*/
	strncpy(translationOut, trans, translationOut_sz);
	translationOut[translationOut_sz - 1] = 0;
}

///////////////////////////////////////////////////////////////////////////


int JS_Dialog_BrowseForSaveFile(const char* windowTitle, const char* initialFolder, const char* initialFile, const char* extensionList, char* fileNameOutNeedBig, int fileNameOutNeedBig_sz)
{			
	// NeedBig buffers should be 2^15 chars by default
	if (fileNameOutNeedBig_sz < 32000) return -1;

	// Set default extension and filter
	const char* newExtList = ((strlen(extensionList) > 0) ? extensionList : "All files (*.*)\0*.*\0\0");

#ifdef _WIN32
	strncpy(fileNameOutNeedBig, initialFile, fileNameOutNeedBig_sz);
	fileNameOutNeedBig[fileNameOutNeedBig_sz - 1] = 0;

	OPENFILENAME info{
		sizeof(OPENFILENAME),	//DWORD         lStructSize;
		NULL,					//HWND          hwndOwner;
		NULL,					//HINSTANCE     hInstance;
		newExtList,				//----LPCSTR lpstrFilter;
		NULL,					//LPSTR         lpstrCustomFilter;
		NULL,					//DWORD         nMaxCustFilter;
		0,						//DWORD         nFilterIndex;
		fileNameOutNeedBig,			//LPSTR         lpstrFile;
		(DWORD)fileNameOutNeedBig_sz,	//DWORD         nMaxFile;
		NULL,					//LPSTR         lpstrFileTitle;
		0,						//DWORD         nMaxFileTitle;
		initialFolder,			//LPCSTR        lpstrInitialDir;
		windowTitle,			//LPCSTR        lpstrTitle;
		OFN_EXPLORER | OFN_LONGNAMES | OFN_PATHMUSTEXIST, //DWORD         Flags; -- 
		0,						//WORD          nFileOffset;
		0,						//WORD          nFileExtension;
		NULL,					//LPCSTR        lpstrDefExt;
		NULL,					//LPARAM        lCustData;
		NULL,					//LPOFNHOOKPROC lpfnHook;
		NULL,					//LPCSTR        lpTemplateName;
		NULL,					//void          *pvReserved;
		NULL,					//DWORD         dwReserved;
		0						//DWORD         FlagsEx;
	};
	BOOL gotFile = GetSaveFileName(&info);
#else
	// returns TRUE if file was chosen. 
	BOOL gotFile = (BOOL)BrowseForSaveFile(windowTitle, initialFolder, initialFile, newExtList, fileNameOutNeedBig, fileNameOutNeedBig_sz);
#endif
	if(!gotFile)
		fileNameOutNeedBig[0] = '\0';
	return gotFile;
}

int JS_Dialog_BrowseForOpenFiles(const char* windowTitle, const char* initialFolder, const char* initialFile, const char* extensionList, bool allowMultiple, char* fileNamesOutNeedBig, int fileNamesOutNeedBig_sz)
{
	// Set default extension and filter
	const char* newExtList = ((strlen(extensionList) > 0) ? extensionList : "All files (*.*)\0*.*\0\0");

	// GetOpenFileName returns the required buffer length in a mere 2 bytes, so a beffer size of 1024*1024 should be more than enough.
	constexpr uint32_t LONGLEN = 1024 * 1024;

	// If buffer is succesfully created, this will not be NULL anymore, and must be freed.
	char* fileNames = NULL;

	// Default retval is -1.  Will only become true once all the steps are completed succesfully.
	int retval = -1;

#ifdef _WIN32
	// The potentially very long return string will be stored in here
	fileNames = (char*) malloc(LONGLEN);
	if (fileNames) {
		strncpy(fileNames, initialFile, LONGLEN);
		fileNames[LONGLEN - 1] = 0;

		DWORD flags = allowMultiple ? (OFN_EXPLORER | OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_ALLOWMULTISELECT)
									: (OFN_EXPLORER | OFN_LONGNAMES | OFN_PATHMUSTEXIST);

		OPENFILENAME info{
			sizeof(OPENFILENAME),	//DWORD         lStructSize;
			NULL,					//HWND          hwndOwner;
			NULL,					//HINSTANCE     hInstance;
			newExtList,				//----LPCSTR lpstrFilter;
			NULL,					//LPSTR         lpstrCustomFilter;
			NULL,					//DWORD         nMaxCustFilter;
			0,						//DWORD         nFilterIndex;
			fileNames,				//LPSTR         lpstrFile;
			LONGLEN,				//DWORD         nMaxFile;
			NULL,					//LPSTR         lpstrFileTitle;
			0,						//DWORD         nMaxFileTitle;
			initialFolder,			//LPCSTR        lpstrInitialDir;
			windowTitle,			//LPCSTR        lpstrTitle;
			flags,					//DWORD         Flags; -- 
			0,						//WORD          nFileOffset;
			0,						//WORD          nFileExtension;
			NULL,					//LPCSTR        lpstrDefExt;
			NULL,					//LPARAM        lCustData;
			NULL,					//LPOFNHOOKPROC lpfnHook;
			NULL,					//LPCSTR        lpTemplateName;
			NULL,					//void          *pvReserved;
			NULL,					//DWORD         dwReserved;
			0						//DWORD         FlagsEx;
		};
		retval = (GetOpenFileName(&info) ? -1 : 0); // If return value is TRUE, retval remains -1 
#else
	// free() the result of this, if non-NULL.
	// if allowmul is set, the multiple files are specified the same way GetOpenFileName() returns.
	fileNames = BrowseForFiles(windowTitle, initialFolder, initialFile, allowMultiple, newExtList);
	retval = (fileNames ? -1 : 0);
	{
#endif
		if (retval) { 
			// If allowMulti, filenames will be 0-separated, and entire string will end in a double \0\0.
			size_t fileLen = strlen(fileNames);
			if (allowMultiple) {
				for (; fileLen < LONGLEN; fileLen++) {
					if (fileNames[fileLen] == 0 && fileNames[fileLen + 1] == 0)
						break;
				}
			}
			if (fileLen < LONGLEN) {
				if (realloc_cmd_ptr(&fileNamesOutNeedBig, &fileNamesOutNeedBig_sz, fileLen)) {
					if (fileNamesOutNeedBig && fileNamesOutNeedBig_sz == fileLen) {
						memcpy(fileNamesOutNeedBig, fileNames, fileLen);
						retval = TRUE;
					}
				}
			}
		}
	}
	if (fileNames) free(fileNames);
	return retval;
}


// Browsing for folders is quite a mess in WIN32 (compared to swell).
// Must use this weird callback function merely to set initial folder.
#ifdef _WIN32
INT CALLBACK BrowseForFolder_CallBack(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM pData)
{
	if (uMsg == BFFM_INITIALIZED)
	{
		string initialDir = (const char*)pData;
		SendMessage(hwnd, BFFM_SETSELECTION, TRUE, pData);
	}
	return 0;
}
#endif

int JS_Dialog_BrowseForFolder(const char* caption, const char* initialFolder, char* folderOutNeedBig, int folderOutNeedBig_sz)
{
	if (folderOutNeedBig_sz < 32000) return -1;

#ifdef _WIN32
	_browseinfoA info{
		NULL,					//HWND        hwndOwner;
		NULL,					//PCIDLIST_ABSOLUTE pidlRoot;
		folderOutNeedBig,		//pszDisplayName;       // Return display name of item selected.
		caption,				//LPCSTR       lpszTitle; // text to go in the banner over the tree.
		BIF_NEWDIALOGSTYLE | BIF_BROWSEINCLUDEURLS | BIF_RETURNONLYFSDIRS , //UINT         ulFlags;                       // Flags that control the return stuff
		BrowseForFolder_CallBack,		//BFFCALLBACK  lpfn;
		(LPARAM)initialFolder,	//LPARAM       lParam;	// extra info that's passed back in callbacks
		0						//int          iImage;	// output var: where to return the Image index.
	};

	PIDLIST_ABSOLUTE folderID = SHBrowseForFolder(&info);
	if (folderID)
		SHGetPathFromIDList(folderID, folderOutNeedBig);
	ILFree(folderID);

	return (BOOL)!!folderID;

#else
	// returns TRUE if path was chosen.
	return BrowseForDirectory(caption, initialFolder, folderOutNeedBig, folderOutNeedBig_sz);
#endif
}


//////////////////////////////////////////////////////////////////////////////////////////////////

bool JS_Window_GetRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut)
{
	HWND hwnd = (HWND)windowHWND;
	RECT r{ 0, 0, 0, 0 };
	bool isOK = !!GetWindowRect(hwnd, &r);
	*leftOut   = (int)r.left;
	*rightOut  = (int)r.right;
	*topOut	   = (int)r.top;
	*bottomOut = (int)r.bottom;
	return (isOK);
}

void JS_Window_ScreenToClient(void* windowHWND, int x, int y, int* xOut, int* yOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	POINT p{ x, y };
	HWND hwnd = (HWND)windowHWND;
	ScreenToClient(hwnd, &p);
	*xOut = (int)p.x;
	*yOut = (int)p.y;
}

void JS_Window_ClientToScreen(void* windowHWND, int x, int y, int* xOut, int* yOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	POINT p{ x, y };
	HWND hwnd = (HWND)windowHWND;
	ClientToScreen(hwnd, &p);
	*xOut = (int)p.x;
	*yOut = (int)p.y;
}


bool JS_Window_GetClientRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	// However, if hwnd is not a true hwnd, SWELL will return a {0,0,0,0} rect.
	HWND hwnd = (HWND)windowHWND;
	RECT r{ 0, 0, 0, 0 };
#ifdef _WIN32
	bool isOK = !!GetClientRect(hwnd, &r);
#else
	GetClientRect(hwnd, &r);
	bool isOK = (r.bottom != 0 || r.right != 0);
#endif
	if (isOK)
	{
		POINT p{ 0, 0 };
		ClientToScreen(hwnd, &p);
		*leftOut = (int)p.x;
		*rightOut = (int)p.x + (int)r.right;
		*topOut = (int)p.y;
		*bottomOut = (int)p.y + (int)r.bottom;
	}
	return (isOK);
}


/*
bool JS_Window_GetClientRect(void* windowHWND, int* widthOut, int* heightOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	// However, if hwnd is not a true hwnd, SWELL will return a {0,0,0,0} rect.
	RECT r;
#ifdef _WIN32
	bool isOK = !!GetClientRect((HWND)windowHWND, &r);
#else
	GetClientRect((HWND)windowHWND, &r);
	bool isOK = (r.bottom != 0 || r.right != 0);
#endif
	*widthOut = r.right;
	*heightOut = r.bottom;
	return (isOK);
}
*/

void* JS_Window_FromPoint(int x, int y)
{
	POINT p{ x, y };
	return WindowFromPoint(p);
}

void* JS_Window_GetParent(void* windowHWND)
{
	return GetParent((HWND)windowHWND);
}

bool  JS_Window_IsChild(void* parentHWND, void* childHWND)
{
	return !!IsChild((HWND)parentHWND, (HWND)childHWND);
}

void* JS_Window_GetRelated(void* windowHWND, const char* relation)
{
	/*
	#define GW_HWNDFIRST        0
	#define GW_HWNDLAST         1
	#define GW_HWNDNEXT         2
	#define GW_HWNDPREV         3
	#define GW_OWNER            4
	#define GW_CHILD            5
	*/
	int intMode;
	if		(strstr(relation, "FIRST"))	intMode = GW_HWNDFIRST;
	else if (strstr(relation, "LAST"))	intMode = GW_HWNDLAST;
	else if (strstr(relation, "NEXT"))	intMode = GW_HWNDNEXT;
	else if (strstr(relation, "PREV"))	intMode = GW_HWNDPREV;
	else if (strstr(relation, "OWNER"))	intMode = GW_OWNER;
	else if (strstr(relation, "CHILD"))	intMode = GW_CHILD;
	else return nullptr;
	return GetWindow((HWND)windowHWND, intMode);
}



void  JS_Window_SetFocus(void* windowHWND)
{
	// SWELL returns different types than Win32, so this function won't return anything.
	SetFocus((HWND)windowHWND);
}

void  JS_Window_SetForeground(void* windowHWND)
{
	// SWELL returns different types than Win32, so this function won't return anything.
	SetForegroundWindow((HWND)windowHWND);
}

void* JS_Window_GetFocus()
{
	return GetFocus();
}

void* JS_Window_GetForeground()
{
	return GetForegroundWindow();
}



void  JS_Window_Enable(void* windowHWND, bool enable)
{
	EnableWindow((HWND)windowHWND, (BOOL)enable); // (enable ? (int)1 : (int)0));
}

void  JS_Window_Destroy(void* windowHWND)
{
	DestroyWindow((HWND)windowHWND);
}

void  JS_Window_Show(void* windowHWND, const char* state)
{
	/*
	#define SW_HIDE 0
	#define SW_SHOWNA 1        // 8 on win32
	#define SW_SHOW 2          // 1 on win32
	#define SW_SHOWMINIMIZED 3 // 2 on win32

	// aliases (todo implement these as needed)
	#define SW_SHOWNOACTIVATE SW_SHOWNA
	#define SW_NORMAL SW_SHOW
	#define SW_SHOWNORMAL SW_SHOW
	#define SW_SHOWMAXIMIZED SW_SHOW
	#define SW_SHOWDEFAULT SW_SHOWNORMAL
	#define SW_RESTORE SW_SHOWNA
	*/
	int intState;
	if		(strstr(state, "SHOWNA"))	intState = SW_SHOWNA;
	else if (strstr(state, "MINI"))		intState = SW_SHOWMINIMIZED;
	else if (strstr(state, "HIDE"))		intState = SW_HIDE;
	else intState = SW_SHOW;
	ShowWindow((HWND)windowHWND, intState);
}

bool JS_Window_IsVisible(void* windowHWND)
{
	return !!IsWindowVisible((HWND)windowHWND);
}



void* JS_Window_SetCapture(void* windowHWND)
{
	return SetCapture((HWND)windowHWND);
}

void* JS_Window_GetCapture()
{
	return GetCapture();
}

void  JS_Window_ReleaseCapture()
{
	ReleaseCapture();
}


void* JS_Window_GetLongPtr(void* windowHWND, const char* info)
{
	int intMode;
#ifdef _WIN32
	if (strstr(info, "USER"))			intMode = GWLP_USERDATA;
	else if (strstr(info, "WND"))		intMode = GWLP_WNDPROC;
	else if (strstr(info, "ID"))		intMode = GWL_ID;
	else if (strstr(info, "EXSTYLE"))	intMode = GWL_EXSTYLE;
	else if (strstr(info, "STYLE"))		intMode = GWL_STYLE;
	else return nullptr;

	return (void*)GetWindowLongPtr((HWND)windowHWND, intMode);

#else 
	if (strstr(info, "USER"))			intMode = GWL_USERDATA;
	else if (strstr(info, "WND"))		intMode = GWL_WNDPROC;
	else if (strstr(info, "DLG"))		intMode = DWL_DLGPROC;
	else if (strstr(info, "ID"))		intMode = GWL_ID;
	else if (strstr(info, "EXSTYLE"))	intMode = GWL_EXSTYLE;
	else if (strstr(info, "STYLE"))		intMode = GWL_STYLE;
	else return nullptr;

	return (void*)GetWindowLong((HWND)windowHWND, intMode);
#endif
}


HWND JS_Window_FindEx(HWND parentHWND, HWND childHWND, const char* className, const char* title)
{
	// REAPER API cannot pass null pointers, so must do another way:
	HWND		c = ((parentHWND == childHWND) ? nullptr : childHWND);
	const char* t = ((strlen(title) == 0) ? nullptr : title);
	return FindWindowEx(parentHWND, c, className, t);
}


HWND JS_Window_FindChildByID(HWND parent, int ID)
{
	return GetDlgItem(parent, ID);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

// All the ArrayFind functions use the same code to convert a std::set<HWND> to an array,
//		so instead of repeating the code in every function, they call this helper function:
int ConvertSetHWNDToArray(std::set<HWND>& foundHWNDs, double* reaperarray)
{
	// Is space available in the reaper.array?
	uint32_t& currentArraySize = ((uint32_t*)reaperarray)[0];
	uint32_t& maximumArraySize = ((uint32_t*)reaperarray)[1];
	if (maximumArraySize < (4 * 1024 * 1024) && currentArraySize + foundHWNDs.size() <= maximumArraySize) { // Sanity check: 4*1024*1024-1 is the maximum size of reaper.arrays
		// Space is available, so copy HWNDs into array
		for (const HWND& hwnd : foundHWNDs) {
			reaperarray[currentArraySize + 1] = (double)((intptr_t)hwnd);
			currentArraySize++;
		}
		return (int)foundHWNDs.size();
	}
	else
		return -(int)foundHWNDs.size();
}

// All the ListFind functions use the same code to convert a std::set<HWND> to a string,
//		so instead of repeating the code in every function, they call this helper function:
int ConvertSetHWNDToString(std::set<HWND>& foundHWNDs, char*& reaperBufNeedBig, int& reaperBufNeedBig_sz)
{
	using namespace Julian;

	// By default, the function will return an error and 0-length string. Only if everything goes well, will retval be flipped positive.
	int retval = -(int)foundHWNDs.size();
	reaperBufNeedBig[0] = 0;

	// If windows have been found, convert foundHWNDs into a comma-separated string.
	if (retval) {
		// REAPER's realloc_cmd_ptr deletes the contents of the previous buffer,
		//		so must first print into a large, temporary buffer to determine the exact length of the list string,
		//		then call realloc_cmd_ptr once and copy the string to itemsOutNeedBig. 
		char* itemBuf = (char*)malloc(foundHWNDs.size() * MAX_CHARS_PER_HWND); // This buffer is large enough to hold entire list string.
		if (itemBuf) {
			int totalLen = 0;  // Total length of list of items (minus terminating \0)
			int tempLen = 0;  // Length of last item printed
			for (const HWND& hwnd : foundHWNDs) {
				tempLen = snprintf(itemBuf + totalLen, MAX_CHARS_PER_HWND, "0x%llX,", (unsigned long long int)hwnd); // As long as < MAX_CHARS, there will not be buffer overflow.
				if (tempLen < 4 || tempLen > MAX_CHARS_PER_HWND) { // Whoops, something went wrong. 
					totalLen = 0;
					break;
				}
				else
					totalLen += tempLen;
			}
			if (totalLen) {
				totalLen--; // If any numbers were printed, remove final comma
							// Copy exact number of printed characters to itemsOut buffer 
				if (realloc_cmd_ptr(&reaperBufNeedBig, &reaperBufNeedBig_sz, totalLen)) {	// Was realloc successful?
					if (reaperBufNeedBig && reaperBufNeedBig_sz == totalLen) {	// Was realloc really successful?
						memcpy(reaperBufNeedBig, itemBuf, totalLen);
						retval = -retval; // Finally, retval gets an OK value
					}
				}
			}
			free(itemBuf);
		}
	}
	return retval;
}


/////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK JS_Window_Find_Callback_Child(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows& s = *(reinterpret_cast<sEnumWindows*>(structPtr));
	int len = GetWindowText(hwnd, s.temp, s.tempLen);
	s.temp[s.tempLen - 1] = '\0'; // Make sure that loooong titles are properly terminated.
	for (int i = 0; (s.temp[i] != '\0') && (i < len); i++) s.temp[i] = (char)tolower(s.temp[i]); // FindWindow is case-insensitive, so this implementation is too
	if (     (s.exact  && (strcmp(s.temp, s.target) == 0)    )
		|| (!(s.exact) && (strstr(s.temp, s.target) != NULL)))
	{
		s.foundHWNDs->insert(hwnd);
		return FALSE;
	}
	else
		return TRUE;
}

BOOL CALLBACK JS_Window_Find_Callback_Top(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows& s = *(reinterpret_cast<sEnumWindows*>(structPtr));
	int len = GetWindowText(hwnd, s.temp, s.tempLen);
	s.temp[s.tempLen-1] = '\0'; // Make sure that loooong titles are properly terminated.
	for (int i = 0; (s.temp[i] != '\0') && (i < len); i++) s.temp[i] = (char)tolower(s.temp[i]); // FindWindow is case-insensitive, so this implementation is too
	if (     (s.exact  && (strcmp(s.temp, s.target) == 0)    )
		|| (!(s.exact) && (strstr(s.temp, s.target) != NULL)))
	{
		s.foundHWNDs->insert(hwnd);
		return FALSE;
	}
	else
	{
		EnumChildWindows(hwnd, JS_Window_Find_Callback_Child, structPtr);
		if (s.foundHWNDs->size()) return FALSE;
		else return TRUE;
	}
}

// Cockos SWELL doesn't provide FindWindow, and FindWindowEx doesn't provide the NULL, NULL top-level mode,
//		so must code own implementation...
// This implemetation adds two features:
//		* Searches child windows as well, so that script GUIs can be found even if docked.
//		* Optionally matches substrings.
void* JS_Window_Find(const char* title, bool exact)
{
	using namespace Julian;

	// FindWindow is case-insensitive, so this implementation is too. 
	// Must first convert title to lowercase:
	char titleLower[API_LEN];
	int i = 0;
	for (; (title[i] != '\0') && (i < API_LEN - 1); i++) titleLower[i] = (char)tolower(title[i]); // Convert to lowercase
	titleLower[i] = '\0';

	// To communicate with callback functions, use an sEnumWindows:
	std::set<HWND> foundHWNDs;
	char temp[API_LEN] = ""; // Will temprarily store titles as well as pointer string, so must be longer than TEMP_LEN.
	sEnumWindows e{ titleLower, exact, temp, sizeof(temp), &foundHWNDs };
	EnumWindows(JS_Window_Find_Callback_Top, reinterpret_cast<LPARAM>(&e));
	if (foundHWNDs.size())
		return *(foundHWNDs.begin());
	else
		return NULL;
}

////////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK JS_Window_FindChild_Callback(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows& s = *(reinterpret_cast<sEnumWindows*>(structPtr));
	int len = GetWindowText(hwnd, s.temp, s.tempLen);
	for (int i = 0; (s.temp[i] != '\0') && (i < len); i++) s.temp[i] = (char)tolower(s.temp[i]); // Convert to lowercase
	if (     (s.exact  && (strcmp(s.temp, s.target) == 0))
		|| (!(s.exact) && (strstr(s.temp, s.target) != NULL)))
	{
		s.foundHWNDs->insert(hwnd);
		return FALSE;
	}
	else
		return TRUE;
}

// Cockos SWELL doesn't provide fully-functional FindWindowEx, so rather code own implementation.
// This implemetation adds two features:
//		* Searches child windows as well, so that script GUIs can be found even if docked.
//		* Optionally matches substrings.
void* JS_Window_FindChild(void* parentHWND, const char* title, bool exact)
{
	using namespace Julian;

	// FindWindow is case-insensitive, so this implementation is too. 
	// Must first convert title to lowercase:
	char titleLower[API_LEN]; 
	for (int i = 0; i < API_LEN; i++) {
		if (title[i] == '\0')	{ titleLower[i] = '\0'; break; }
		else					{ titleLower[i] = (char)tolower(title[i]); }
	}
	titleLower[API_LEN - 1] = '\0';

	// To communicate with callback functions, use an sEnumWindows struct:
	std::set<HWND> foundHWNDs;
	char temp[TEMP_LEN];
	sEnumWindows e{ titleLower, exact, temp, sizeof(temp), &foundHWNDs };
	EnumChildWindows((HWND)parentHWND, JS_Window_FindChild_Callback, reinterpret_cast<LPARAM>(&e));
	if (foundHWNDs.size())
		return *(foundHWNDs.begin());
	else
		return NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK JS_Window_ListFind_Callback_Child(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows& s = *(reinterpret_cast<sEnumWindows*>(structPtr));
	set<HWND>& foundHWNDs = *(s.foundHWNDs);

	int len = GetWindowText(hwnd, s.temp, s.tempLen);
	// Make sure that loooong titles are properly terminated.
	s.temp[s.tempLen - 1] = '\0';
	// FindWindow is case-insensitive, so this implementation is too. Convert to lowercase.
	for (int i = 0; (s.temp[i] != '\0') && (i < len); i++) s.temp[i] = (char)tolower(s.temp[i]);
	// If exact, match entire title, otherwise substring
	if ((	  s.exact  && (strcmp(s.temp, s.target) == 0))
		|| (!(s.exact) && (strstr(s.temp, s.target) != NULL)))
		foundHWNDs.insert(hwnd);

	return TRUE;
}

BOOL CALLBACK JS_Window_ListFind_Callback_Top(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows& s = *(reinterpret_cast<sEnumWindows*>(structPtr));
	set<HWND>& foundHWNDs = *(s.foundHWNDs);

	int len = GetWindowText(hwnd, s.temp, s.tempLen);
	// Make sure that loooong titles are properly terminated.
	s.temp[s.tempLen - 1] = '\0'; 
	// FindWindow is case-insensitive, so this implementation is too. Convert to lowercase.
	for (int i = 0; (s.temp[i] != '\0') && (i < len); i++) s.temp[i] = (char)tolower(s.temp[i]);
	// If exact, match entire title, otherwise substring
	if (	 (s.exact  && (strcmp(s.temp, s.target) == 0))
		|| (!(s.exact) && (strstr(s.temp, s.target) != NULL)))
		foundHWNDs.insert(hwnd);

	// Now search all child windows before returning
	EnumChildWindows(hwnd, JS_Window_ListFind_Callback_Child, structPtr);
	return TRUE;
}

// Cockos SWELL doesn't provide FindWindow, and FindWindowEx doesn't provide the NULL, NULL top-level mode,
//		so must code own implementation...
// This implemetation adds three features:
//		* Searches child windows as well, so that script GUIs can be found even if docked.
//		* Optionally matches substrings.
//		* Finds ALL windows that match title.
int JS_Window_ListFind(const char* title, bool exact, char* listOutNeedBig, int listOutNeedBig_sz)
{
	using namespace Julian;

	// FindWindow is case-insensitive, so this implementation is too. 
	// Must first convert title to lowercase:
	char titleLower[API_LEN];
	for (int i = 0; i < API_LEN; i++) {
		if (title[i] == '\0') { titleLower[i] = '\0'; break; }
		else				  { titleLower[i] = (char)tolower(title[i]); }
	}
	// Make sure that loooong titles are properly terminated.
	titleLower[API_LEN - 1] = '\0'; 

	// To communicate with callback functions, use an sEnumWindows struct:
	std::set<HWND> foundHWNDs;
	char temp[API_LEN] = ""; // Will temporarily store pointer strings, as well as titles.
	sEnumWindows e{ titleLower, exact, temp, sizeof(temp), &foundHWNDs }; // All the info that will be passed to the Enum callback functions.

	// ********** Enumerate through windows, searching for matching title **********
	EnumWindows(JS_Window_ListFind_Callback_Top, reinterpret_cast<LPARAM>(&e));

	return ConvertSetHWNDToString(foundHWNDs, listOutNeedBig, listOutNeedBig_sz);
}

// Cockos SWELL doesn't provide FindWindow, and FindWindowEx doesn't provide the NULL, NULL top-level mode,
//		so must code own implementation...
// This implemetation adds three features:
//		* Searches child windows as well, so that script GUIs can be found even if docked.
//		* Optionally matches substrings.
//		* Finds ALL windows that match title.
int JS_Window_ArrayFind(const char* title, bool exact, double* reaperarray)
{
	using namespace Julian;

	// FindWindow is case-insensitive, so this implementation is too. 
	// Must first convert title to lowercase:
	char titleLower[API_LEN];
	for (int i = 0; i < API_LEN; i++) {
		if (title[i] == '\0')	{ titleLower[i] = '\0'; break; }
		else					{ titleLower[i] = (char)tolower(title[i]); }
	}
	// Make sure that loooong titles are properly terminated.
	titleLower[API_LEN - 1] = '\0'; 

	// To communicate with callback functions, use an sEnumWindows struct:
	std::set<HWND> foundHWNDs;
	char temp[API_LEN] = ""; // Will temporarily store pointer strings, as well as titles.
	sEnumWindows e{ titleLower, exact, temp, sizeof(temp), &foundHWNDs }; // All the info that will be passed to the Enum callback functions.

	// ********** Enumerate through windows, searching for matching title **********
	EnumWindows(JS_Window_ListFind_Callback_Top, reinterpret_cast<LPARAM>(&e));

	return ConvertSetHWNDToArray(foundHWNDs, reaperarray);
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////


BOOL CALLBACK JS_Window_ListAllChild_Callback(HWND hwnd, LPARAM lParam)
{
	set<HWND>& foundHWNDs = *(reinterpret_cast<set<HWND>*>(lParam));
	foundHWNDs.insert(hwnd);
	return TRUE;
}

int JS_Window_ListAllChild(void* parentHWND, char* listOutNeedBig, int listOutNeedBig_sz)
{
	// Enumerate through all child windows and store ther HWNDs in a set. (Sets are nice, since will automatically check for uniqueness.)
	std::set<HWND> foundHWNDs;
	EnumChildWindows((HWND)parentHWND, JS_Window_ListAllChild_Callback, reinterpret_cast<LPARAM>(&foundHWNDs));

	return ConvertSetHWNDToString(foundHWNDs, listOutNeedBig, listOutNeedBig_sz);
}

int JS_Window_ArrayAllChild(void* parentHWND, double* reaperarray)
{
	// Enumerate through all child windows and store ther HWNDs in a set. (Sets are nice, since will automatically check for uniqueness.)
	std::set<HWND> foundHWNDs;
	EnumChildWindows((HWND)parentHWND, JS_Window_ListAllChild_Callback, reinterpret_cast<LPARAM>(&foundHWNDs));

	return ConvertSetHWNDToArray(foundHWNDs, reaperarray);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////


BOOL CALLBACK JS_Window_ListAllTop_Callback(HWND hwnd, LPARAM lParam)
{
	set<HWND>& foundHWNDs = *(reinterpret_cast<set<HWND>*>(lParam));
	foundHWNDs.insert(hwnd);
	return TRUE;
}

int JS_Window_ListAllTop(char* listOutNeedBig, int listOutNeedBig_sz)
{
	// Enumerate through all top-level windows and store ther HWNDs in a set. (Sets are nice, since will automatically check for uniqueness.)
	set<HWND> foundHWNDs;
	EnumWindows(JS_Window_ListAllTop_Callback, reinterpret_cast<LPARAM>(&foundHWNDs));

	return ConvertSetHWNDToString(foundHWNDs, listOutNeedBig, listOutNeedBig_sz);
}

int JS_Window_ArrayAllTop(double* reaperarray)
{
	// Enumerate through all top-level windows and store ther HWNDs in a set. (Sets are nice, since will automatically check for uniqueness.)
	set<HWND> foundHWNDs;
	EnumWindows(JS_Window_ListAllTop_Callback, reinterpret_cast<LPARAM>(&foundHWNDs));;

	return ConvertSetHWNDToArray(foundHWNDs, reaperarray);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK JS_MIDIEditor_ListAll_Callback_Child(HWND hwnd, LPARAM lParam)
{
	if (MIDIEditor_GetMode(hwnd) != -1) // Is MIDI editor?
	{
		set<HWND>& foundHWNDs = *(reinterpret_cast<set<HWND>*>(lParam));
		foundHWNDs.insert(hwnd);
	}
	return TRUE;
}

BOOL CALLBACK JS_MIDIEditor_ListAll_Callback_Top(HWND hwnd, LPARAM lParam)
{
	if (MIDIEditor_GetMode(hwnd) != -1) // Is MIDI editor?
	{
		set<HWND>& foundHWNDs = *(reinterpret_cast<set<HWND>*>(lParam));
		foundHWNDs.insert(hwnd);
	}
	else // If top level window is not a MIDI editor, check child windows. For example if docked in docker or main window.
	{
		EnumChildWindows(hwnd, JS_MIDIEditor_ListAll_Callback_Child, lParam);
	}
	return TRUE;
}

int JS_MIDIEditor_ListAll(char* listOutNeedBig, int listOutNeedBig_sz)
{
	// Enumerate through all windows and store any MIDI editor HWNDs in a set. (Sets are nice, since will automatically check for uniqueness.)
	// To find docked editors, must also enumerate child windows.
	std::set<HWND> foundHWNDs;
	EnumWindows(JS_MIDIEditor_ListAll_Callback_Top, reinterpret_cast<LPARAM>(&foundHWNDs));
	
	return ConvertSetHWNDToString(foundHWNDs, listOutNeedBig, listOutNeedBig_sz);
}

int JS_MIDIEditor_ArrayAll(double* reaperarray)
{
	// Enumerate through all windows and store any MIDI editor HWNDs in a set. (Sets are nice, since will automatically check for uniqueness.)
	// To find docked editors, must also enumerate child windows.
	std::set<HWND> foundHWNDs;
	EnumWindows(JS_MIDIEditor_ListAll_Callback_Top, reinterpret_cast<LPARAM>(&foundHWNDs));

	return ConvertSetHWNDToArray(foundHWNDs, reaperarray);
}

/////////////////////////////////////////////////////////////////////////////////////////////

/*
Some functions using SetWindowLongPtr...

These flags are avilable in WDL/swell:

BEWARE, THESE NUMBERS ARE DIFFERENT FROM WIN32!!!

#define SWP_NOMOVE 1
#define SWP_NOSIZE 2
#define SWP_NOZORDER 4
#define SWP_NOACTIVATE 8
#define SWP_SHOWWINDOW 16
#define SWP_FRAMECHANGED 32
#define SWP_NOCOPYBITS 0
#define HWND_TOP        ((HWND)0)
#define HWND_BOTTOM     ((HWND)1)
#define HWND_TOPMOST    ((HWND)-1)
#define HWND_NOTOPMOST  ((HWND)-2)
*/
#ifndef SWP_NOOWNERZORDER
#define SWP_NOOWNERZORDER 0 // In WIN32, this would be 0x200, but in swell, use 0 so that no change when OR'ing.
#endif

// This function moves windows without resizing or requiring any info about window size.
void JS_Window_Move(void* windowHWND, int left, int top)
{
	SetWindowPos((HWND)windowHWND, NULL, left, top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSIZE);
}

// This function resizes windows without moving or requiring any info about window position.
void JS_Window_Resize(void* windowHWND, int width, int height)
{
	SetWindowPos((HWND)windowHWND, NULL, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE);
}

void JS_Window_SetPosition(void* windowHWND, int left, int top, int width, int height)
{
	SetWindowPos((HWND)windowHWND, NULL, left, top, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER );
}

void JS_Window_SetZOrder(void* windowHWND, const char* ZOrder, void* insertAfterHWND)
{
	HWND insertAfter;
	// Search for single chars that can distinguish the different ZOrder strings.
	if      (strstr(ZOrder, "IN"))		insertAfter = (HWND)insertAfterHWND; // insertAfter
	else if (strstr(ZOrder, "BO"))		insertAfter = HWND_BOTTOM;
	else if (strstr(ZOrder, "NOT"))		insertAfter = HWND_NOTOPMOST;
	else if (strstr(ZOrder, "TOPM"))	insertAfter = HWND_TOPMOST;
	else								insertAfter = HWND_TOP; // Top

	SetWindowPos((HWND)windowHWND, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

bool JS_Window_SetOpacity(HWND windowHWND, const char* mode, double value)
{
	// Opacity can only be applied to top-level framed windows, AFAIK, and in Linux, REAPER crashes if opacity is applied to a child window.
	// So must check that style is WS_THICKFRAME.
	bool OK = false;
	if (IsWindow(windowHWND))
	{
#ifdef _WIN32
		if (GetWindowLongPtr(windowHWND, GWL_STYLE) & WS_THICKFRAME)
		{
			if (SetWindowLongPtr(windowHWND, GWL_EXSTYLE, GetWindowLongPtr(windowHWND, GWL_EXSTYLE) | WS_EX_LAYERED))
			{
				if (strchr(mode, 'A'))
					OK = !!(SetLayeredWindowAttributes(windowHWND, 0, (BYTE)(value * 255), LWA_ALPHA));
				else
				{
					UINT v = (UINT)value;
					OK = !!(SetLayeredWindowAttributes(windowHWND, (COLORREF)(((v & 0xFF0000) >> 16) | (v & 0x00FF00) | ((v & 0x0000FF) << 16)), 0, LWA_COLORKEY));
				}
#elif __linux__
		if (GetWindowLong(windowHWND, GWL_STYLE) & WS_THICKFRAME)
		{
			if (strchr(mode, 'A') || (!IsWindow(windowHWND)))
			{
				GdkWindow* w = (GdkWindow*)windowHWND->m_oswindow;
				if (w)
				{
					gdk_window_set_opacity(w, value);
					OK = true;
				}
#elif __APPLE__
		if (GetWindowLong(windowHWND, GWL_STYLE) & WS_THICKFRAME)
		{
			if (strchr(mode, 'A'))
			{
				OK = JS_Window_SetOpacity_ObjC((void*)windowHWND, value);
#endif
			}
		}
	}
	return OK;
}


bool JS_Window_SetTitle(void* windowHWND, const char* title)
{
	return !!SetWindowText((HWND)windowHWND, title);
}

void JS_Window_GetTitle(void* windowHWND, char* titleOut, int titleOut_sz)
{
	GetWindowText((HWND)windowHWND, titleOut, titleOut_sz);
}

void JS_Window_GetClassName(HWND windowHWND, char* classOut, int classOut_sz)
{
	GetClassName(windowHWND, classOut, classOut_sz);
}

void* JS_Window_HandleFromAddress(double address)
{
	// Casting to intptr_t removes fractions and, in case of 32-bit systems, truncates too large addresses.
	//		This provides easy way to check whether address was valid.
	intptr_t intAddress = (intptr_t)address;
	if ((double)intAddress == address)
		return (void*)intAddress;
	else
		return nullptr;
}

void JS_Window_AddressFromHandle(void* handle, double* addressOut)
{
	*addressOut = (double)(intptr_t)handle;
}

bool  JS_Window_IsWindow(void* windowHWND)
{
	return !!IsWindow((HWND)windowHWND);
}




bool JS_WindowMessage_ListIntercepts(void* windowHWND, char* listOutNeedBig, int listOutNeedBig_sz)
{
	using namespace Julian;
	listOutNeedBig[0] = '\0';
	
	if (mapWindowToData.count((HWND)windowHWND))
	{
		auto& messages = mapWindowToData[(HWND)windowHWND].messages;
		for (const auto& it : messages)
		{
			if (strlen(listOutNeedBig) < (UINT)listOutNeedBig_sz - 32)
			{
				if (mapMsgToWM_.count(it.first))
					sprintf(strchr(listOutNeedBig, 0), "%s:", mapMsgToWM_[it.first].c_str());
				else
					sprintf(strchr(listOutNeedBig, 0), "0x%04X:", it.first);
				if ((it.second).passthrough)
					strcat(listOutNeedBig, "passthrough,\0");
				else
					strcat(listOutNeedBig, "block,\0");
			}
			else
				return false;
		}
		
		char* lastComma{ strrchr(listOutNeedBig, ',') }; // Remove final comma
		if (lastComma)
			*lastComma = '\0';
		return true;
	}
	else
		return true;
		
}

bool JS_WindowMessage_Post(void* windowHWND, const char* message, int wParamLow, int wParamHigh, int lParamLow, int lParamHigh)
{
	using namespace Julian;

	std::string msgString = message;
	UINT uMsg = 0;
	if (mapWM_toMsg.count(msgString))
		uMsg = mapWM_toMsg[msgString];
	else
	{
		errno = 0;
		char* endPtr;
		uMsg = strtoul(message, &endPtr, 16);
		if (endPtr == message || errno != 0) // 0x0000 is a valid message type, so cannot assume 0 is error.
			return false;
	}

	WPARAM wParam = MAKEWPARAM(wParamLow, wParamHigh);
	LPARAM lParam = MAKELPARAM(lParamLow, lParamHigh);
	HWND hwnd = (HWND)windowHWND;

	// Is this window currently being intercepted?
	if (mapWindowToData.count(hwnd)) {
		sWindowData& w = mapWindowToData[hwnd];
		if (w.messages.count(uMsg)) {
			w.origProc(hwnd, uMsg, wParam, lParam); // WindowProcs usually return 0 if message was handled.  But not always, 
			return true;
		}
	}
	return !!PostMessage(hwnd, uMsg, wParam, lParam);
}

bool JS_Window_OnCommand(void* windowHWND, int commandID)
{
	return JS_WindowMessage_Post(windowHWND, "WM_COMMAND", commandID, 0, 0, 0);
}

int JS_WindowMessage_Send(void* windowHWND, const char* message, int wParamLow, int wParamHigh, int lParamLow, int lParamHigh)
{
	using namespace Julian;

	std::string msgString = message;
	UINT uMsg = 0;
	if (mapWM_toMsg.count(msgString))
		uMsg = mapWM_toMsg[msgString];
	else
	{
		errno = 0;
		char* endPtr;
		uMsg = strtoul(message, &endPtr, 16);
		if (endPtr == message || errno != 0) // 0x0000 is a valid message type, so cannot assume 0 is error.
			return FALSE;
	}

	WPARAM wParam = MAKEWPARAM(wParamLow, wParamHigh);
	LPARAM lParam = MAKELPARAM(lParamLow, lParamHigh);

	return (int)SendMessage((HWND)windowHWND, uMsg, wParam, lParam);
}

// swell does not define these macros:
#ifndef GET_KEYSTATE_WPARAM
#define GET_KEYSTATE_WPARAM(wParam) LOWORD(wParam)
#endif
#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wParam) ((short)HIWORD(wParam))
#endif



bool JS_WindowMessage_Peek(void* windowHWND, const char* message, bool* passedThroughOut, double* timeOut, int* wParamLowOut, int* wParamHighOut, int* lParamLowOut, int* lParamHighOut)
{
	// lParamLow, lParamHigh, and wParamHigh are signed, whereas wParamLow is unsigned.
	using namespace Julian;

	std::string msgString = message;
	UINT uMsg = 0;
	if (mapWM_toMsg.count(msgString))
		uMsg = mapWM_toMsg[msgString];
	else
	{
		errno = 0;
		char* endPtr;
		uMsg = strtoul(message, &endPtr, 16);
		if (endPtr == message || errno != 0) // 0x0000 is a valid message type, so cannot assume 0 is error.
			return false;
	}

	if (mapWindowToData.count((HWND)windowHWND))
	{
		sWindowData& w = mapWindowToData[(HWND)windowHWND];

		if (w.messages.count(uMsg))
		{
			sMsgData& m = w.messages[uMsg];

			*passedThroughOut = m.passthrough;
			*timeOut		= m.time;
			*lParamLowOut	= GET_X_LPARAM(m.lParam);
			*lParamHighOut	= GET_Y_LPARAM(m.lParam);
			*wParamLowOut	= GET_KEYSTATE_WPARAM(m.wParam);
			*wParamHighOut	= GET_WHEEL_DELTA_WPARAM(m.wParam);

			return true;
		}
	}
	return false;
}

LRESULT CALLBACK JS_WindowMessage_Intercept_Callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	using namespace Julian;

	// If not in map, we don't know how to call original process.
	if (mapWindowToData.count(hwnd) == 0)
		return 1;

	sWindowData& windowData = mapWindowToData[hwnd]; // Get reference/alias because want to write into existing struct.

	// Always get KEYDOWN and KEYUP, to construct keyboard bitfields
	/*
	if (uMsg == WM_KEYDOWN)
	{
		if ((wParam >= 0x41) && (wParam <= VK_SLEEP)) // A to SLEEP in virtual key codes
			windowData.keysBitfieldAtoSLEEP |= (1 << (wParam - 0x41));
		else if ((wParam >= VK_LEFT) && (wParam <= 0x39)) // LEFT to 9 in virtual key codes
			windowData.keysBitfieldLEFTto9 |= (1 << (wParam - VK_LEFT));
		else if ((wParam >= VK_BACK) && (wParam <= VK_HOME)) // BACKSPACE to HOME in virtual key codes
			windowData.keysBitfieldLEFTto9 |= (1 << (wParam - VK_BACK));
	}

	else if (uMsg == WM_KEYUP)
	{
		if ((wParam >= 0x41) && (wParam <= VK_SLEEP)) // A to SLEEP in virtual key codes
			windowData.keysBitfieldAtoSLEEP &= !(1 << (wParam - 0x41));
		else if ((wParam >= VK_LEFT) && (wParam <= 0x39)) // LEFT to 9 in virtual key codes
			windowData.keysBitfieldLEFTto9 &= !(1 << (wParam - VK_LEFT));
		else if ((wParam >= VK_BACK) && (wParam <= VK_HOME)) // BACKSPACE to HOME in virtual key codes
			windowData.keysBitfieldBACKtoHOME &= !(1 << (wParam - VK_BACK));
	}
	*/

	// Event that should be intercepted? 
	if (windowData.messages.count(uMsg)) // ".contains" has only been implemented in more recent C++ versions
	{
		windowData.messages[uMsg].time = time_precise();
		windowData.messages[uMsg].wParam = wParam;
		windowData.messages[uMsg].lParam = lParam;

		// If event will not be passed through, can quit here.
		if (windowData.messages[uMsg].passthrough == false)
		{
			// Most WM_ messages return 0 if processed, with only a few exceptions:
			switch (uMsg)
			{
			case WM_SETCURSOR:
			case WM_DRAWITEM:
			case WM_COPYDATA:
				return 1;
			case WM_MOUSEACTIVATE:
				return 3;
			default:
				return 0;
			}
		}
	}

	// Any other event that isn't intercepted.
	return windowData.origProc(hwnd, uMsg, wParam, lParam);
}

// Intercept a single message type
int JS_WindowMessage_Intercept(void* windowHWND, const char* message, bool passthrough)
{
	using namespace Julian;
	HWND hwnd = (HWND)windowHWND;
	UINT uMsg;

	// Convert string to UINT
	string msgString = message;
	if (mapWM_toMsg.count(msgString))
		uMsg = mapWM_toMsg[msgString];
	else
	{
		errno = 0;
		uMsg = strtoul(message, nullptr, 16);
		if (errno != 0 || (uMsg == 0 && !(strstr(message, "0x0000")))) // 0x0000 is a valid message type, so cannot assume 0 is error.
			return ERR_PARSING;
	}

	// Is this window already being intercepted?

	// Not yet intercepted, so create new sWindowdata map
	if (Julian::mapWindowToData.count(hwnd) == 0) 
	{
		// Try to get the original process.
		WNDPROC origProc = nullptr;
		#ifdef _WIN32
				origProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
		#else
				origProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
		#endif
		if (!origProc)
			return ERR_ORIGPROC;

		Julian::mapWindowToData.emplace(hwnd, sWindowData{ origProc, map<UINT,sMsgData>{ } }); // Insert empty map
	}

	// Window already intercepted.  So try to add to existing maps.
	else
	{
		// Check that no overlaps: only one script may intercept each message type
		if (Julian::mapWindowToData[hwnd].messages.count(uMsg))
			return ERR_ALREADY_INTERCEPTED;
	}

	Julian::mapWindowToData[hwnd].messages.emplace(uMsg, sMsgData{ passthrough, 0, 0, 0 });
	return 1;
}

int JS_WindowMessage_PassThrough(void* windowHWND, const char* message, bool passThrough)
{
	using namespace Julian;
	HWND hwnd = (HWND)windowHWND;
	UINT uMsg;

	// Is this window already being intercepted?
	if (Julian::mapWindowToData.count(hwnd) == 0)
		return ERR_ALREADY_INTERCEPTED; // Actually, NOT intercepted

	// Convert string to UINT
	string msgString = message;
	if (mapWM_toMsg.count(msgString))
		uMsg = mapWM_toMsg[msgString];
	else
	{
		errno = 0;
		uMsg = strtoul(message, nullptr, 16);
		if (errno != 0 || (uMsg == 0 && !(strstr(message, "0x0000")))) // 0x0000 is a valid message type, so cannot assume 0 is error.
			return ERR_PARSING;
	}

	// Is this message type actually already being intercepted?
	if (Julian::mapWindowToData[hwnd].messages.count(uMsg) == 0)
		return ERR_ALREADY_INTERCEPTED;

	// Change passthrough
	Julian::mapWindowToData[hwnd].messages[uMsg].passthrough = passThrough;

	return 1;
}

int JS_WindowMessage_InterceptList(void* windowHWND, const char* messages)
{
	using namespace Julian;
	HWND hwnd = (HWND)windowHWND;

	// According to swell-functions.h, IsWindow is slow in swell. However, JS_Window_Intercept will probably not be called many times per script. 
	if (!IsWindow(hwnd))
		return ERR_NOT_WINDOW;

	// strtok *replaces* characters in the string, so better copy messages to new char array.
	// It should not be possible for API functions to pass more than API_LEN characters.  But make doubly sure...
	if (strlen(messages) > API_LEN)
		return ERR_PARSING;
	char msg[API_LEN];
	strncpy(msg, messages, API_LEN);
	msg[API_LEN - 1] = 0;

	// messages string will be parsed into uMsg message types and passthrough modifiers 
	UINT uMsg;
	bool passthrough;

	// For use while tokenizing messages string
	char *token;
	std::string msgString;
	const char* delim = ":;,= \n\t";
	char* endPtr; 

	// Parsed info will be stored in these temporary maps
	map<UINT, sMsgData> newMessages;

	// Parse!
	token = strtok(msg, delim);
	while (token)
	{
		// Get message number
		msgString = token;
		if (mapWM_toMsg.count(msgString))
			uMsg = mapWM_toMsg[msgString];
		else
		{
			errno = 0;
			uMsg = strtoul(token, &endPtr, 16);
			if (endPtr == token || errno != 0) // 0x0000 is a valid message type, so cannot assume 0 is error.
				return ERR_PARSING;
		}

		// Now get passthrough
		token = strtok(NULL, delim);
		if (token == NULL)
			return ERR_PARSING; // Each message type must be followed by a modifier
		else if (token[0] == 'p' || token[0] == 'P')
			passthrough = true;
		else if (token[0] == 'b' || token[0] == 'B')
			passthrough = false;
		else // Not block or passthrough
			return ERR_PARSING;

		// Save in temporary maps
		newMessages.emplace(uMsg, sMsgData{ passthrough, 0, 0, 0 }); // time = 0 indicates that this message type is being intercepted OK, but that no message has yet been received.

		token = strtok(NULL, delim);
	}

	// Parsing went OK?  Any messages to intercept?
	if (newMessages.size() == 0)
		return ERR_PARSING;

	// Is this window already being intercepted?
	if (mapWindowToData.count(hwnd) == 0) // Not yet intercepted
	{
		// Try to get the original process.
		WNDPROC origProc = nullptr;
#ifdef _WIN32
		origProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
#else
		origProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
#endif
		if (!origProc)
			return ERR_ORIGPROC;

		// Got everything OK.  Finally, store struct.
		Julian::mapWindowToData.emplace(hwnd, sWindowData{ origProc, newMessages }); // Insert into static map of namespace
		return 1;
	}

	// Already intercepted.  So add to existing maps.
	else
	{
		// Check that no overlaps: only one script may intercept each message type
		// Want to update existing map, so use aliases/references
		auto& existingMsg = Julian::mapWindowToData[hwnd].messages; // Messages that are already being intercepted for this window
		for (const auto& it : newMessages)
		{
			if (existingMsg.count(it.first)) // Oops, already intercepting this message type
			{
				return ERR_ALREADY_INTERCEPTED;
			}
		}
		// No overlaps, so add new intercepts to existing messages to intercept
		existingMsg.insert(newMessages.begin(), newMessages.end());
		return 1;
	}
}

int JS_WindowMessage_Release(void* windowHWND, const char* messages)
{
	using namespace Julian;
	HWND hwnd = (HWND)windowHWND;

	if (mapWindowToData.count(hwnd) == 0)
		return ERR_NOT_WINDOW;

	// strtok *replaces* characters in the string, so better copy messages to new char array.
	// It should not be possible for API functions to pass more than API_LEN characters.  But make doubly sure...
	if (strlen(messages) > API_LEN)
		return ERR_PARSING;
	char msg[API_LEN];
	strncpy(msg, messages, API_LEN);
	msg[API_LEN - 1] = 0;

	// messages string will be parsed into uMsg message types and passthrough modifiers 
	UINT uMsg;
	char *token;
	std::string msgString;
	const char* delim = ":;,= \n\t";
	std::set<UINT> messagesToErase;

	// Parse!
	token = strtok(msg, delim);
	while (token)
	{
		// Get message number
		msgString = token;
		if (mapWM_toMsg.count(msgString))
			uMsg = mapWM_toMsg[msgString];
		else
			uMsg = strtoul(token, NULL, 16);
		if (!uMsg || (errno == ERANGE))
			return ERR_PARSING;

		// Store this parsed uMsg number
		messagesToErase.insert(uMsg);

		token = strtok(NULL, delim);
	}

	// Erase all message types that have been parsed
	auto& existingMessages = Julian::mapWindowToData[hwnd].messages; // Messages that are already being intercepted for this window
	for (const UINT it : messagesToErase)
		existingMessages.erase(it);

	// If no messages need to be intercepted any more, release this window
	if (existingMessages.size() == 0)
		JS_WindowMessage_ReleaseWindow(hwnd);

	return TRUE;
}

void JS_WindowMessage_ReleaseWindow(void* windowHWND)
{
	using namespace Julian;

	if (mapWindowToData.count((HWND)windowHWND))
	{
		WNDPROC origProc = mapWindowToData[(HWND)windowHWND].origProc;
#ifdef _WIN32
		SetWindowLongPtr((HWND)windowHWND, GWLP_WNDPROC, (LONG_PTR)origProc);
#else
		SetWindowLong((HWND)windowHWND, GWL_WNDPROC, (LONG_PTR)origProc);
#endif
		mapWindowToData.erase((HWND)windowHWND);
	}
}

void JS_WindowMessage_ReleaseAll()
{
	using namespace Julian;
	for (auto it = mapWindowToData.begin(); it != mapWindowToData.end(); ++it)
	{
		HWND hwnd = it->first;
		WNDPROC origProc = it->second.origProc;
#ifdef _WIN32
		SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)origProc);
#else
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)origProc);
#endif
	}
	mapWindowToData.clear();
}


int JS_Mouse_GetState(int flags)
{
	int state = 0;
	if ((flags & 1)  && (GetAsyncKeyState(VK_LBUTTON) >> 1))	state |= 1;
	if ((flags & 2)  && (GetAsyncKeyState(VK_RBUTTON) >> 1))	state |= 2;
	if ((flags & 64) && (GetAsyncKeyState(VK_MBUTTON) >> 1))	state |= 64;
	if ((flags & 4)  && (GetAsyncKeyState(VK_CONTROL) >> 1))	state |= 4;
	if ((flags & 8)  && (GetAsyncKeyState(VK_SHIFT) >> 1))		state |= 8;
	if ((flags & 16) && (GetAsyncKeyState(VK_MENU) >> 1))		state |= 16;
	if ((flags & 32) && (GetAsyncKeyState(VK_LWIN) >> 1))		state |= 32;
	return state;
}

bool JS_Mouse_SetPosition(int x, int y)
{
	return !!SetCursorPos(x, y);
}

void* JS_Mouse_LoadCursor(int cursorNumber)
{
	// GetModuleHandle isn't implemented in SWELL, but fortunately also not necessary.
	// In SWELL, LoadCursor ignores hInst and will automatically loads either standard cursor or "registered cursor".
#ifdef _WIN32
	HINSTANCE hInst; 
	if (cursorNumber > 32000) // In Win32, hInst = NULL loads standard Window cursors, with IDs > 32000.
		hInst = NULL;
	else
		hInst = GetModuleHandle(NULL); // REAPER exe file.
	return LoadCursor(hInst, MAKEINTRESOURCE(cursorNumber));
#else
	return SWELL_LoadCursor(MAKEINTRESOURCE(cursorNumber));
#endif
}

void* JS_Mouse_LoadCursorFromFile(const char* pathAndFileName)
{
#ifdef _WIN32
	return LoadCursorFromFile(pathAndFileName);
#else
	return SWELL_LoadCursorFromFile(pathAndFileName);
#endif
}

void JS_Mouse_SetCursor(void* cursorHandle)
{
	SetCursor((HCURSOR)cursorHandle);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* JS_GDI_GetClientDC(void* windowHWND)
{
	return GetDC((HWND)windowHWND);
}

void* JS_GDI_GetWindowDC(void* windowHWND)
{
	return GetWindowDC((HWND)windowHWND);
}

void* JS_GDI_GetScreenDC()
{
	return GetDC(NULL);
}

void JS_GDI_ReleaseDC(void* windowHWND, void* deviceHDC)
{
	ReleaseDC((HWND)windowHWND, (HDC)deviceHDC);
}


void* JS_GDI_CreateFillBrush(int color)
{
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
#ifdef _WIN32
	return CreateSolidBrush(color);
#else
	return CreateSolidBrushAlpha(color, 1);
#endif
}

void* JS_GDI_CreatePen(int width, int color)
{
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
	return CreatePen(PS_SOLID, width, color);
}

#ifndef FF_DONTCARE
#define FF_DONTCARE 0
#endif
void* JS_GDI_CreateFont(int height, int weight, int angle, bool italic, bool underline, bool strikeOut, const char* fontName)
{
	return CreateFont(height, 0, angle, 0, weight, (BOOL)italic, (BOOL)underline, (BOOL)strikeOut, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName);
}

void* JS_GDI_SelectObject(void* deviceHDC, void* GDIObject)
{
	return SelectObject((HDC)deviceHDC, (HGDIOBJ)GDIObject);
}

void JS_GDI_DeleteObject(void* GDIObject)
{
	DeleteObject((HGDIOBJ)GDIObject);
}



void JS_GDI_Line(void* deviceHDC, int x1, int y1, int x2, int y2)
{
	MoveToEx((HDC)deviceHDC, x1, y1, NULL);
	LineTo((HDC)deviceHDC, x2, y2);
}

void JS_GDI_Polyline(void* deviceHDC, const char* packedX, const char* packedY, int numPoints)
// If we assume that POINT structs are always being packed into 8-byte chunks,
//		we could have used a single packed string of x1, y1, x2, y2, etc.
{
    std::vector<POINT> p(numPoints);
	for (int i = 0; i < numPoints; i++)
		p[i] = { ((int32_t*)packedX)[i], ((int32_t*)packedY)[i] };
	DWORD n[1] = { (DWORD)numPoints };
	PolyPolyline((HDC)deviceHDC, p.data(), n, 1);
}

void JS_GDI_FillEllipse(void* deviceHDC, int x1, int y1, int x2, int y2)
{
	Ellipse((HDC)deviceHDC, x1, y1, x2, y2);
}

void JS_GDI_FillRoundRect(void* deviceHDC, int x1, int y1, int x2, int y2, int xrnd, int yrnd)
{
	RoundRect((HDC)deviceHDC, x1, y1, x2, y2, xrnd, yrnd);
}

void JS_GDI_FillRect(void* deviceHDC, int x1, int y1, int x2, int y2)
{
	Rectangle((HDC)deviceHDC, x1, y1, x2, y2);
}

void JS_GDI_FillPolygon(void* deviceHDC, const char* packedX, const char* packedY, int numPoints)
// If we assume that POINT structs are always being packed into 8-byte chunks,
//		we could have used a single packed string of x1, y1, x2, y2, etc.
{
    std::vector<POINT> p(numPoints);
	for (int i = 0; i < numPoints; i++)
		p[i] = { ((int32_t*)packedX)[i], ((int32_t*)packedY)[i] };
	Polygon((HDC)deviceHDC, p.data(), (uint32_t)numPoints);
}


int JS_GDI_GetSysColor(const char* GUIElement)
{
	int intMode = COLOR_WINDOW;
	if		(strstr(GUIElement, "3DSHADOW"))	intMode = COLOR_3DSHADOW;
	else if (strstr(GUIElement, "3DHILIGHT"))	intMode = COLOR_3DHILIGHT;
	else if (strstr(GUIElement, "3DFACE"))		intMode = COLOR_3DFACE;
	else if (strstr(GUIElement, "BTNTEXT"))		intMode = COLOR_BTNTEXT;
	//else if (strstr(GUIElement, "WINDOW"))		intMode = COLOR_WINDOW;
	else if (strstr(GUIElement, "SCROLLBAR"))	intMode = COLOR_SCROLLBAR;
	else if (strstr(GUIElement, "3DDKSHADOW"))	intMode = COLOR_3DDKSHADOW;
	else if (strstr(GUIElement, "INFOBK"))		intMode = COLOR_INFOBK;
	else if (strstr(GUIElement, "INFOTEXT"))	intMode = COLOR_INFOTEXT;
	int color = GetSysColor(intMode);
	return ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);;
}

void JS_GDI_SetTextBkMode(void* deviceHDC, int mode)
{
	SetBkMode((HDC)deviceHDC, mode);
}

void JS_GDI_SetTextBkColor(void* deviceHDC, int color)
{
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
	SetBkColor((HDC)deviceHDC, color);
}

void JS_GDI_SetTextColor(void* deviceHDC, int color)
{
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
	SetTextColor((HDC)deviceHDC, color);
}

int JS_GDI_GetTextColor(void* deviceHDC)
{
	int color = GetTextColor((HDC)deviceHDC);
	return ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
}

int JS_GDI_DrawText(void* deviceHDC, const char *text, int len, int left, int top, int right, int bottom, const char* align)
{
	int intMode = DT_TOP | DT_LEFT; // = 0;
	//if(strstr(align, "TOP"))		intMode = DT_TOP ;
	//if (strstr(align, "LEFT"))	intMode = intMode | DT_LEFT ;
	if (strstr(align, "HCEN"))		intMode = intMode | DT_CENTER; // This function uses "HCENTER" instead of just "CENTER".
	if (strstr(align, "RIGH"))		intMode = intMode | DT_RIGHT ;
	if (strstr(align, "VCEN"))		intMode = intMode | DT_VCENTER;
	if (strstr(align, "BOTT"))		intMode = intMode | DT_BOTTOM ;
	if (strstr(align, "BREA"))		intMode = intMode | DT_WORDBREAK ;
	if (strstr(align, "SING"))		intMode = intMode | DT_SINGLELINE ;
	if (strstr(align, "NOCLIP"))	intMode = intMode | DT_NOCLIP ;
	if (strstr(align, "RECT"))		intMode = intMode | DT_CALCRECT ;
	if (strstr(align, "NOPRE"))		intMode = intMode | DT_NOPREFIX ;
	if (strstr(align, "ELLI"))		intMode = intMode | DT_END_ELLIPSIS ;

	RECT r{ left, top, right, bottom };
	return DrawText((HDC)deviceHDC, text, len, &r, intMode);
}



void JS_GDI_SetPixel(void* deviceHDC, int x, int y, int color)
{
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
	SetPixel((HDC)deviceHDC, x, y, color);
}



void JS_GDI_Blit(void* destHDC, int dstx, int dsty, void* sourceHDC, int srcx, int srcy, int width, int height)
{
	BitBlt((HDC)destHDC, dstx, dsty, width, height, (HDC)sourceHDC, srcx, srcy, SRCCOPY);  //swell only provdes the SRCCOPY mode.  NB: swell defines SRCCOPY as 0 instead of HCC0020
}

void JS_GDI_StretchBlit(void* destHDC, int dstx, int dsty, int dstw, int dsth, void* sourceHDC, int srcx, int srcy, int srcw, int srch)
{
	StretchBlt((HDC)destHDC, dstx, dsty, dstw, dsth, (HDC)sourceHDC, srcx, srcy, srcw, srch, SRCCOPY);
}


///////////////////////////////////////////////////////////////////////////////////////////////////

bool JS_Window_GetScrollInfo(void* windowHWND, const char* scrollbar, int* positionOut, int* pageSizeOut, int* minOut, int* maxOut, int* trackPosOut)
{
	SCROLLINFO si = { sizeof(SCROLLINFO), SIF_ALL, };
	int nBar = ((strchr(scrollbar, 'v') || strchr(scrollbar, 'V')) ? SB_VERT : SB_HORZ); // Match strings such as "SB_VERT", "VERT" or "v".
	bool isOK = !!CoolSB_GetScrollInfo((HWND)windowHWND, nBar, &si);
	*pageSizeOut = si.nPage;
	*positionOut = si.nPos;
	*minOut = si.nMin;
	*maxOut = si.nMax;
	*trackPosOut = si.nTrackPos;
	return isOK;
}

bool JS_Window_SetScrollPos(void* windowHWND, const char* scrollbar, int position)
{
	bool isOK;
	if (strchr(scrollbar, 'v') || strchr(scrollbar, 'V'))
	{
		isOK = !!CoolSB_SetScrollPos((HWND)windowHWND, SB_VERT, position, TRUE);
		if (!isOK)
			isOK = !!CoolSB_SetScrollPos((HWND)windowHWND, SB_VERT, position, TRUE);
		if (isOK)
			SendMessage((HWND)windowHWND, WM_VSCROLL, MAKEWPARAM(SB_THUMBPOSITION, position), 0);
	}
	else
	{
		isOK = !!CoolSB_SetScrollPos((HWND)windowHWND, SB_HORZ, position, TRUE);
		if (!isOK)
			isOK = !!CoolSB_SetScrollPos((HWND)windowHWND, SB_HORZ, position, TRUE);
		if (isOK)
			SendMessage((HWND)windowHWND, WM_HSCROLL, MAKEWPARAM(SB_THUMBPOSITION, position), 0);
	}
	return isOK;
}


/////////////////////////////////////////////

void* JS_LICE_CreateBitmap(bool isSysBitmap, int width, int height)
{
	LICE_IBitmap* bm = LICE_CreateBitmap((BOOL)isSysBitmap, width, height); // If SysBitmap, can BitBlt to/from screen like HDC.
	if (bm) 
		Julian::LICEBitmaps.emplace(bm);
	return bm;
}

int JS_LICE_GetHeight(void* bitmap)
{
	return LICE__GetHeight((LICE_IBitmap*)bitmap);
}

int JS_LICE_GetWidth(void* bitmap)
{
	return LICE__GetWidth((LICE_IBitmap*)bitmap);
}

void* JS_LICE_GetDC(void* bitmap)
{
	return LICE__GetDC((LICE_IBitmap*)bitmap);
}

void JS_LICE_DestroyBitmap(void* bitmap)
{
	Julian::LICEBitmaps.erase((LICE_IBitmap*)bitmap);
	LICE__Destroy((LICE_IBitmap*)bitmap);
}

#define LICE_BLIT_MODE_MASK 0xff
#define LICE_BLIT_MODE_COPY 0
#define LICE_BLIT_MODE_ADD 1
#define LICE_BLIT_MODE_DODGE 2
#define LICE_BLIT_MODE_MUL 3
#define LICE_BLIT_MODE_OVERLAY 4
#define LICE_BLIT_MODE_HSVADJ 5
#define LICE_BLIT_MODE_CHANCOPY 0xf0 // in this mode, only available for LICE_Blit(), the low nibble is 2 bits of source channel (low 2), 2 bits of dest channel (high 2)
#define LICE_BLIT_USE_ALPHA 0x10000 // use source's alpha channel


#define GETINTMODE 	int intMode = LICE_BLIT_MODE_COPY; \
					if ((strlen(mode) == 0) || strstr(mode, "COPY")); \
					else if (strstr(mode, "MASK"))		intMode = LICE_BLIT_MODE_MASK; \
					else if (strstr(mode, "ADD"))		intMode = LICE_BLIT_MODE_ADD; \
					else if (strstr(mode, "DODGE"))		intMode = LICE_BLIT_MODE_DODGE; \
					else if (strstr(mode, "MUL"))		intMode = LICE_BLIT_MODE_MUL; \
					else if (strstr(mode, "OVERLAY"))	intMode = LICE_BLIT_MODE_OVERLAY; \
					else if (strstr(mode, "HSVADJ"))	intMode = LICE_BLIT_MODE_HSVADJ; \
					if (strstr(mode, "ALPHA"))	intMode |= LICE_BLIT_USE_ALPHA;

void JS_LICE_Blit(void* destBitmap, int dstx, int dsty, void* sourceBitmap, int srcx, int srcy, int width, int height, double alpha, const char* mode)
{	
	GETINTMODE
	LICE_Blit((LICE_IBitmap*)destBitmap, (LICE_IBitmap*)sourceBitmap, dstx, dsty, srcx, srcy, width, height, (float)alpha, intMode);
}

void JS_LICE_RotatedBlit(void* destBitmap, int dstx, int dsty, int dstw, int dsth, void* sourceBitmap, double srcx, double srcy, double srcw, double srch, double angle, double rotxcent, double rotycent, bool cliptosourcerect, double alpha, const char* mode)
{
	GETINTMODE	
	LICE_RotatedBlit((LICE_IBitmap*)destBitmap, (LICE_IBitmap*)sourceBitmap, dstx, dsty, dstw, dsth, (float)srcx, (float)srcy, (float)srcw, (float)srch, (float)angle, cliptosourcerect, (float)alpha, intMode, (float)rotxcent, (float)rotycent);
}

void JS_LICE_ScaledBlit(void* destBitmap, int dstx, int dsty, int dstw, int dsth, void* sourceBitmap, double srcx, double srcy, double srcw, double srch, double alpha, const char* mode)
{
	GETINTMODE
	LICE_ScaledBlit((LICE_IBitmap*)destBitmap, (LICE_IBitmap*)sourceBitmap, dstx, dsty, dstw, dsth, (float)srcx, (float)srcy, (float)srcw, (float)srch, (float)alpha, intMode);
}

void* JS_LICE_LoadPNG(const char* filename)
{
	return LICE_LoadPNG(filename, NULL); // In order to force the use of SysBitmaps, use must supply bitmap
}

void JS_LICE_Circle(void* bitmap, double cx, double cy, double r, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	LICE_Circle((LICE_IBitmap*)bitmap, (float)cx, (float)cy, (float)r, (LICE_pixel)color, (float)alpha, intMode, antialias);
}

bool JS_LICE_IsFlipped(void* bitmap)
{
	return LICE__IsFlipped((LICE_IBitmap*)bitmap);
}

bool JS_LICE_Resize(void* bitmap, int width, int height)
{
	return LICE__resize((LICE_IBitmap*)bitmap, width, height);
}

void JS_LICE_Arc(void* bitmap, double cx, double cy, double r, double minAngle, double maxAngle, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	LICE_Arc((LICE_IBitmap*)bitmap, (float)cx, (float)cy, (float)r, (float)minAngle, (float)maxAngle, (LICE_pixel)color, (float)alpha, intMode, antialias);
}

void JS_LICE_Clear(void* bitmap, int color)
{
	LICE_Clear((LICE_IBitmap*)bitmap, (LICE_pixel)color);
}



void* JS_LICE_CreateFont()
{
	return LICE_CreateFont();
}

void JS_LICE_SetFontFromGDI(void* LICEFont, void* GDIFont, const char* moreFormats)
{
	#define LICE_FONT_FLAG_VERTICAL 1 // rotate text to vertical (do not set the windows font to vertical though)
	#define LICE_FONT_FLAG_VERTICAL_BOTTOMUP 2
	#define LICE_FONT_FLAG_PRECALCALL 4
	//#define LICE_FONT_FLAG_ALLOW_NATIVE 8
	#define LICE_FONT_FLAG_FORCE_NATIVE 1024
	#define LICE_FONT_FLAG_FX_BLUR 16
	#define LICE_FONT_FLAG_FX_INVERT 32
	#define LICE_FONT_FLAG_FX_MONO 64 // faster but no AA/etc
	#define LICE_FONT_FLAG_FX_SHADOW 128 // these imply MONO
	#define LICE_FONT_FLAG_FX_OUTLINE 256
	#define LICE_FONT_FLAG_OWNS_HFONT 512
	
	int intMode = LICE_FONT_FLAG_PRECALCALL;
	if (strstr(moreFormats, "VERTI"))	intMode |= LICE_FONT_FLAG_VERTICAL;
	if (strstr(moreFormats, "BOTT"))	intMode |= LICE_FONT_FLAG_VERTICAL_BOTTOMUP;
	if (strstr(moreFormats, "PRE"))		intMode |= LICE_FONT_FLAG_PRECALCALL;
	if (strstr(moreFormats, "NATI"))	intMode |= LICE_FONT_FLAG_FORCE_NATIVE;
	if (strstr(moreFormats, "BLUR"))	intMode |= LICE_FONT_FLAG_FX_BLUR;
	if (strstr(moreFormats, "INVE"))	intMode |= LICE_FONT_FLAG_FX_INVERT;
	if (strstr(moreFormats, "MONO"))	intMode |= LICE_FONT_FLAG_FX_MONO;
	if (strstr(moreFormats, "SHAD"))	intMode |= LICE_FONT_FLAG_FX_SHADOW;
	if (strstr(moreFormats, "OUT"))		intMode |= LICE_FONT_FLAG_FX_OUTLINE;

	LICE__SetFromHFont((LICE_IFont*)LICEFont, (HFONT)GDIFont, intMode);
}

void JS_LICE_DestroyFont(void* LICEFont)
{
	LICE__DestroyFont((LICE_IFont*)LICEFont);
}

void JS_LICE_SetFontBkColor(void* LICEFont, int color)
{
	LICE__SetBkColor((LICE_IFont*)LICEFont, (LICE_pixel)color);
}

void JS_LICE_SetFontColor(void* LICEFont, int color)
{
	LICE__SetTextColor((LICE_IFont*)LICEFont, (LICE_pixel)color);
}

int JS_LICE_DrawText(void* bitmap, void* LICEFont, const char* text, int textLen, int x1, int y1, int x2, int y2)
{
	RECT r{ x1, y1, x2, y2 };
	return LICE__DrawText((LICE_IFont*)LICEFont, (LICE_IBitmap*)bitmap, text, textLen, &r, 0); // I don't know what UINT dtFlags does, so make 0.
}

void JS_LICE_DrawChar(void* bitmap, int x, int y, char c, int color, double alpha, const char* mode)
{
	GETINTMODE
	LICE_DrawChar((LICE_IBitmap*)bitmap, x, y, c, (LICE_pixel)color, (float)alpha, intMode);
}

void JS_LICE_MeasureText(const char* string, int* widthOut, int* heightOut)
{
	LICE_MeasureText(string, widthOut, heightOut);
}


void JS_LICE_FillRect(void* bitmap, int x, int y, int w, int h, int color, double alpha, const char* mode)
{
	GETINTMODE
	LICE_FillRect((LICE_IBitmap*)bitmap, x, y, w, h, (LICE_pixel)color, (float)alpha, intMode);
}

void JS_LICE_RoundRect(void* bitmap, double x, double y, double w, double h, int cornerradius, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	LICE_RoundRect((LICE_IBitmap*)bitmap, (float)x, (float)y, (float)w, (float)h, cornerradius, color, (float)alpha, intMode, antialias);
}


void JS_LICE_GradRect(void* bitmap, int dstx, int dsty, int dstw, int dsth, double ir, double ig, double ib, double ia, double drdx, double dgdx, double dbdx, double dadx, double drdy, double dgdy, double dbdy, double dady, const char* mode)
{
	GETINTMODE
	LICE_GradRect((LICE_IBitmap*)bitmap, dstx, dsty, dstw, dsth, (float)ir, (float)ig, (float)ib, (float)ia, (float)drdx, (float)dgdx, (float)dbdx, (float)dadx, (float)drdy, (float)dgdy, (float)dbdy, (float)dady, intMode);
}

void JS_LICE_FillTriangle(void* bitmap, int x1, int y1, int x2, int y2, int x3, int y3, int color, double alpha, const char* mode)
{
	GETINTMODE
	LICE_FillTriangle((LICE_IBitmap*)bitmap, x1, y1, x2, y2, x3, y3, color, (float)alpha, intMode);
}

void JS_LICE_FillPolygon(void* bitmap, const char* packedX, const char* packedY, int numPoints, int color, double alpha, const char* mode)
{
	GETINTMODE
	LICE_FillConvexPolygon((LICE_IBitmap*)bitmap, (int32_t*)packedX, (int32_t*)packedY, numPoints, color, (float)alpha, intMode);
}

void JS_LICE_FillCircle(void* bitmap, double cx, double cy, double r, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	LICE_FillCircle((LICE_IBitmap*)bitmap, (float)cx, (float)cy, (float)r, color, (float)alpha, intMode, antialias);
}


void JS_LICE_Line(void* bitmap, double x1, double y1, double x2, double y2, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	LICE_Line((LICE_IBitmap*)bitmap, (float)x1, (float)y1, (float)x2, (float)y2, (LICE_pixel)color, (float)alpha, intMode, antialias);
}

void JS_LICE_Bezier(void* bitmap, double xstart, double ystart, double xctl1, double yctl1, double xctl2, double yctl2, double xend, double yend, double tol, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	LICE_DrawCBezier((LICE_IBitmap*)bitmap, xstart, ystart, xctl1, yctl1, xctl2, yctl2, xend, yend, (LICE_pixel)color, (float)alpha, intMode, antialias, tol);
}


int JS_LICE_GetPixel(void* bitmap, int x, int y)
{
	return (int)LICE_GetPixel((LICE_IBitmap*)bitmap, x, y);
}

void JS_LICE_PutPixel(void* bitmap, int x, int y, int color, double alpha, const char* mode)
{
	GETINTMODE
	LICE_PutPixel((LICE_IBitmap*)bitmap, x, y, (LICE_pixel)color, (float)alpha, intMode);
}


///////////////////////////////////////////////////////////
// Undocumented functions

void JS_Window_AttachTopmostPin(void* windowHWND)
{
	AttachWindowTopmostButton((HWND)windowHWND);
	SetWindowPos((HWND)windowHWND, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER); // Force re-draw frame, otherwise pin only becomes visible when window is moved.
}

void JS_Window_AttachResizeGrip(void* windowHWND)
{
	AttachWindowResizeGrip((HWND)windowHWND);
}


////////////////////////////////////////////////////////////

void* JS_PtrFromStr(const char* s)
{
	return (void*)s;
}

void JS_Int(void* address, int offset, int* intOut)
{
	*intOut = ((int32_t*)address)[offset];
}

void JS_Byte(void* address, int offset, int* byteOut)
{
	*byteOut = (int)(((int8_t*)address)[offset]);
}

void JS_Double(void* address, int offset, double* doubleOut)
{
	*doubleOut = ((double*)address)[offset];
}


//////////////////////////////////////////////////////////////
// Listview functions

int JS_ListView_CountItems(HWND listviewHWND)
{
	return ListView_GetItemCount(listviewHWND);
}

int JS_ListView_GetFocusedItem(HWND listviewHWND, char* textOut, int textOut_sz)
{
	textOut[0] = '\0';
	int index = ListView_GetNextItem(listviewHWND, -1, LVNI_FOCUSED);
	if (index != -1) {
		LVITEM info
		{
			LVIF_TEXT | LVIF_STATE,	//UINT mask;
			index,		//int iItem;
			0,		//int iSubItem;
			0,		//UINT state;
			LVIS_SELECTED|LVIS_FOCUSED,	//UINT stateMask;
			textOut,	//LPSTR pszText;
			textOut_sz,	//int cchTextMax;
			0,		//int iImage;
			0		//LPARAM lParam;
		};
		ListView_GetItem(listviewHWND, &info);
	}
	return index;
}

int JS_ListView_EnumSelItems(HWND listviewHWND, int index)
{
	// WDL/swell doesn't offer all these flag options, so this function only offers SELECTED:
	/*
	int intFlags = 0; // = LVNI_ALL;

	if		(strstr(flags, "ABOVE")) intFlags |= LVNI_ABOVE;
	else if (strstr(flags, "BELOW")) intFlags |= LVNI_BELOW;
	else if (strstr(flags, "LEFT"))  intFlags |= LVNI_TOLEFT;
	else if (strstr(flags, "RIGHT")) intFlags |= LVNI_TORIGHT;

	if (strstr(flags, "CUT")) intFlags |= LVNI_CUT;
	if (strstr(flags, "SELECTED")) intFlags |= LVNI_SELECTED;
	if (strstr(flags, "DROPHILITED")) intFlags |= LVNI_DROPHILITED;
	if (strstr(flags, "FOCUSED")) intFlags |= LVNI_FOCUSED;

	if (strstr(flags, "VISIBLEORDER")) intFlags |= LVNI_VISIBLEORDER;
	else if (strstr(flags, "VISIBLEONLY")) intFlags |= LVNI_VISIBLEONLY;
	else if (strstr(flags, "SAMEGROUPONLY")) intFlags |= LVNI_SAMEGROUPONLY;
	*/
	return ListView_GetNextItem(listviewHWND, index, LVNI_SELECTED);
}

void JS_ListView_GetItem(HWND listviewHWND, int index, char* textOut, int textOut_sz, int* stateOut)
{
	textOut[0] = '\0';
	LVITEM info
	{
		LVIF_TEXT | LVIF_STATE,	//UINT mask;
		index,		//int iItem;
		0,		//int iSubItem;
		0,		//UINT state;
		LVIS_SELECTED|LVIS_FOCUSED,	//UINT stateMask;
		textOut,	//LPSTR pszText;
		textOut_sz,	//int cchTextMax;
		0,		//int iImage;
		0		//LPARAM lParam;

					// swell doesn't use the following entries, and also not relevant to this function:
					/*
					0,			//int iIndent;
					#if (NTDDI_VERSION >= NTDDI_WINXP)
					0,			//int iGroupId;
					0,			//UINT cColumns; // tile view columns
					0,			//PUINT puColumns;
					#endif
					#if (NTDDI_VERSION >= NTDDI_VISTA) // Will be unused downlevel, but sizeof(LVITEMA) must be equal to sizeof(LVITEMW)
					0,			//int* piColFmt;
					0			//int iGroup; // readonly. only valid for owner data.
					#endif
					*/
	};

	ListView_GetItem(listviewHWND, &info);
	*stateOut = info.state;

	// WIN32 and swell define LVIS_SELECTED and LVIS_FOCUSED differently, so if swell, swap values:
#ifndef _WIN32
	if (((*stateOut) & LVIS_SELECTED) && !((*stateOut) & LVIS_FOCUSED))
	{
		*stateOut |= LVIS_FOCUSED;
		*stateOut &= !LVIS_SELECTED;
	}
	else if (((*stateOut) & LVIS_FOCUSED) && !((*stateOut) & LVIS_SELECTED))
	{
		*stateOut &= !LVIS_FOCUSED;
		*stateOut |= LVIS_SELECTED;
	}
#endif
}

int JS_ListView_ListAllSelItems(HWND listviewHWND, char* itemsOutNeedBig, int itemsOutNeedBig_sz)
{
	using namespace Julian;

	std::vector<int> items;
	items.reserve(1024);

	int prevIndex = -1; // When trying to run this function on a window that is not a listview, may return endless list of 0's.  So check if index increases.

	int index = ListView_GetNextItem(listviewHWND, -1, LVNI_SELECTED);
	while ((index != -1) && (items.size() < 100000))
	{
		if (index == prevIndex) // New index should always be higher than previous.  If not, it may indicate that listviewHWND is not actually a listview window.
		{
			items.clear();
			break;
		}
		else
		{
			prevIndex = index;
			items.push_back(index);
			index = ListView_GetNextItem(listviewHWND, index, LVNI_SELECTED);
		}
	}

	// By default, the function will return an error and 0-length string. Only if everything goes well, will retval be flipped positive.
	int retval = -(int)items.size();
	itemsOutNeedBig[0] = 0;

	// If windows have been found, convert foundHWNDs into a comma-separated string.
	if (retval) {
		// REAPER's realloc_cmd_ptr deletes the contents of the previous buffer,
		//		so must first print into a large, temporary buffer to determine the exact length of the list string,
		//		then call realloc_cmd_ptr once and copy the string to itemsOutNeedBig. 
		char* itemBuf = (char*)malloc(items.size() * Julian::MAX_CHARS_PER_INT); // This buffer is large enough to hold entire list string.
		if (itemBuf) {
			int totalLen = 0;  // Total length of list of items (minus terminating \0)
			int tempLen = 0;  // Length of last item printed
			for (const int& i : items) {
				tempLen = snprintf(itemBuf + totalLen, MAX_CHARS_PER_INT, "%u,", i); // As long as < MAX_CHARS, there will not be buffer overflow.
				if (tempLen < 2 || tempLen > MAX_CHARS_PER_INT) { // Whoops, something went wrong. 
					totalLen = 0;
					break;
				}
				else
					totalLen += tempLen;
			}
			if (totalLen) {
				totalLen--; // If any numbers were printed, remove final comma
							// Copy exact number of printed characters to itemsOut buffer 
				if (realloc_cmd_ptr(&itemsOutNeedBig, &itemsOutNeedBig_sz, totalLen)) {	// Was realloc successful?
					if (itemsOutNeedBig && itemsOutNeedBig_sz == totalLen) {	// Was realloc really successful?
						memcpy(itemsOutNeedBig, itemBuf, totalLen);
						retval = -retval; // Finally, retval gets an OK value
					}
				}
			}
			free(itemBuf);
		}
	}
	return retval;
}


///////////////////////////////////////////////////////////////////////

inline int getArraySize(double* arr)
{
	if (arr == nullptr)
		return 0;
	int* bufptr = (int*)arr;
	int r = bufptr[1];
	if (r < 1 || r>(4 * 1024 * 1024)) // size is apparently nonsense
		return 0;
	return r;
}


class AudioWriter
{
public:
	AudioWriter(const char* outfn, int numchans, int sr)
	{
		char cfg[] = { 'e','v','a','w', 32, 0 };
		m_sink = PCM_Sink_Create(outfn, cfg, sizeof(cfg), numchans, sr, true);
		m_convbuf.resize(65536); // reserve some initial space so might not need to resize later...
		memset(m_writearraypointers, 0, sizeof(double*) * 64);
	}
	~AudioWriter()
	{
		delete m_sink;
	}
	int Write(double* data, int numframes, int offset)
	{
		if (m_sink == nullptr)
			return 0;
		int nch = m_sink->GetNumChannels();
		if (m_convbuf.size() < numframes*nch)
			m_convbuf.resize(numframes*nch);
		for (int i = 0; i < nch; ++i)
		{
			m_writearraypointers[i] = &m_convbuf[numframes*i];
			for (int j = 0; j < numframes; ++j)
			{
				m_writearraypointers[i][j] = data[(j + offset)*nch + i];
			}
		}
		m_sink->WriteDoubles(m_writearraypointers, numframes, nch, 0, 1);
		return numframes;
	}
	int GetNumChans()
	{
		if (m_sink == nullptr)
			return 0;
		return m_sink->GetNumChannels();
	}
	bool IsReady() { return m_sink != nullptr; }
private:
	PCM_sink* m_sink = nullptr;
	std::vector<double> m_convbuf;
	double* m_writearraypointers[64];
};

AudioWriter* Xen_AudioWriter_Create(const char* filename, int numchans, int samplerate)
{
	AudioWriter* aw = new AudioWriter(filename, numchans, samplerate);
	if (aw->IsReady())
		return aw;
	delete aw;
	return nullptr;
}

void Xen_AudioWriter_Destroy(AudioWriter* aw)
{
	delete aw;
}

int Xen_AudioWriter_Write(AudioWriter* aw, double* data, int numframes, int offset)
{
	if (aw == nullptr || data == nullptr)
		return 0;
	if ((numframes < 1) || (offset < 0))
		return 0;
	return aw->Write(data, numframes, offset);
}


int Xen_GetMediaSourceSamples(PCM_source* src, double* destbuf, int destbufoffset, int numframes, int numchans, double samplerate, double positioninfile)
{
	if (src == nullptr || destbuf==nullptr || numframes<1 || numchans < 1 || samplerate < 1.0)
		return 0;
	int bufsize = getArraySize(destbuf);
	if (bufsize == 0)
		return 0;
	PCM_source_transfer_t block;
	memset(&block, 0, sizeof(PCM_source_transfer_t));
	block.time_s = positioninfile; // seeking in the source is based on seconds
	block.length = numframes;
	block.nch = numchans; // the source will attempt to render as many channels as requested
	block.samplerate = samplerate; // properly implemented sources will resample to requested samplerate
	block.samples = &destbuf[destbufoffset];
	src->GetSamples(&block);
	return block.samples_out;
}

////////////////////////////////////////////////////////////////

