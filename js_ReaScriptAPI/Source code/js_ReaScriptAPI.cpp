#include "stdafx.h"

using namespace std;

// This function is called when REAPER loads or unloads the extension.
// If rec != nil, the extension is being loaded.  If rec == nil, the extension is being UNloaded.
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
			MessageBox(NULL, "Unable to import default API functions.\n\nNOTE:\nThis extension requires REAPER v5.974 or higher.", "ERROR: js_ReaScriptAPI extension", 0);  //fprintf(stderr, "Unable to import API functions.\n");
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

		Julian::REAPER_VERSION = atof(GetAppVersion());
		if (Julian::REAPER_VERSION < 5.974)
		{
			MessageBox(NULL, "This extension requires REAPER v5.974 or higher.", "ERROR: js_ReaScriptAPI extension", 0);
			return 0;
		}

		// Don't know what this does, but apparently it's necessary for the localization functions.
		IMPORT_LOCALIZE_RPLUG(rec);

		// Functions imported, continue initing plugin by exporting this extension's functions to ReaScript API.

		Julian::ReaScriptAPI_Instance = hInstance;

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
			// APIvararg_... for exporting to ReaScript API
			plugin_register(f.regkey_vararg, f.func_vararg);
		}

		// Construct mapMsgToWM_ as inverse of mapWM_ToMsg
		for (auto& i : Julian::mapWM_toMsg)
			Julian::mapMsgToWM_.emplace(i.second, i.first);

		// UNDOCUMENTED FEATURE! "<accelerator" instead of "accelerator" places function in front of keyboard processing queue
		plugin_register("<accelerator", &(Julian::sAccelerator));

		// On WindowsOS, register new class for Window_Create
		/*
#ifdef _WIN32
		Julian::structWindowClass.hInstance = hInstance;
		if (!RegisterClassEx(&(Julian::structWindowClass)))
			MessageBox(NULL, "Window Class registration failed.\n\nThe extension will continue loading, but some script may not work properly.", "ERROR: js_ReaScriptAPI extension",	MB_ICONEXCLAMATION | MB_OK);
#endif
		*/

		return 1; // success
	}
	// Does an extension need to do anything when unloading?  
	// To prevent memory leaks, perhaps try to delete any stuff that may remain in memory?
	// On Windows, LICE bitmaps are automatically destroyed when REAPER quits, but to make extra sure, this function will destroy them explicitly.
	// Why store stuff in extra sets?  For some unexplained reason REAPER crashes if I try to destroy LICE bitmaps explicitly. And for another unexplained reason, this roundabout way works...
	else 
	{
		std::set<HWND> windowsToRestore;
		for (auto& i : Julian::mapWindowData)
			windowsToRestore.insert(i.first);
		for (HWND hwnd : windowsToRestore)
			JS_WindowMessage_RestoreOrigProc(hwnd);

		//for (auto& bm : Julian::LICEBitmaps)
		//	LICE__Destroy(bm.first);
		
		std::set<LICE_IBitmap*> bitmapsToDelete;
		for (auto& i : Julian::LICEBitmaps)
			bitmapsToDelete.insert(i.first);
		for (LICE_IBitmap* bm : bitmapsToDelete)
			JS_LICE_DestroyBitmap(bm);
		
		for (auto& i : Julian::mapMallocToSize)
			free(i.first);
		for (APIdef& i : ::aAPIdefs)
			free(i.defstring);
		for (HGDIOBJ i : Julian::setGDIObjects)
			DeleteObject(i);


		plugin_register("-accelerator", &(Julian::sAccelerator));
		return 0;
	}
}

/*
v0.963:
 * Fix bug in Find functions in macOS.
 * New optional bool parameter for Mouse_LoadCursorFromFile. If not true, function will try to re-use previously loaded cursor.
 * Window_FindEx function.
v0.964:
 * Implement IsWindow for Linux and macOS.
v0.970
 * Linux and macOS: Improved IsWindow.
 * Windows OS: File dialogs accept forward slash.
 * Windows OS: BrowseForOpenFiles returns folder with terminal slash.
 * New functions: Memory allocation and access.
 * WindowMessage functions: Recognize ComboBox message names such as "CB_GETCURSEL".
v0.971
 * Fix possible memory leak in File dialogs.
v0.972
 * macOS: Fixed GetClientRect.
 * WindowsOS: Confirmation dialogs for BrowseForSaveFile and BrowseForOpenFiles.
 * New function: MonitorFromRect.
 * GDI: Linux and macOS use same color format as Windows.
v0.980
 * Functions for getting and intercepting virtual key / keyboard states.
 * Functions for compositing LICE bitmaps into REAPER windows.
 * Enabled alpha blending for GDI blitting.
v0.981
 * Don't cache GDI HDCs.
 * JS_WindowMessage_Send and _Post can skip MAKEWPARAM and MAKELPARAM to send larger values.
v0.983
 * New audio preview functions by Xenakios.
 * Improvements in Compositing functions, including:
    ~ Bug fix: Return original window proc when all bitmaps are unlinked.
	~ Source and dest RECTs of existing linked bitmap can be changed without first having to unlink.
v0.984
 * Hotfix for keyboard intercepts on macOS.
v0.985
 * VKeys: Keyboard intercepts are first in queue, so work in MIDI editor too.
 * VKeys: Cutoff time, new functions for KeyUp and KeyDown
 * LICE: MeasureText
 * BrowseForOpenFiles: On WindowsOS, prevent creating new file.
v0.986
 * New: JS_LICE_WritePNG.
v0.987
 * New: Various functions for manipulating LICE colors.
v0.988
 * New: JS_Mouse_GetCursor
v0.989
 * Window_Find does not search child windows.
 * New: Window_FindAny for searching top-level as well as child windows.
v0.990
 * Fix: Window_SetOpacity works on macOS.
 * Fix: Defstring memory leak, GDI Objects are automatically destroyed on exit.
 * New: JS_Window_Create.
 * New: JS_ListView_EnsureVisible.
 * New: More options for Window_Show.
 * New: JS_Window_SetZOrder options work on macOS.
*/


void JS_ReaScriptAPI_Version(double* versionOut)
{
	*versionOut = 0.990;
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Virtual keys / Keyboard functions
static double VK_KeyDown[256]{ 0 }; // time stamp of latest WM_KEYDOWN message
static double VK_KeyUp[256]{ 0 }; // time stamp of latest WM_KEYUP message
static unsigned char VK_Intercepts[256]{ 0 }; // Should the VK be intercepted?
static constexpr size_t VK_Size = 255; // sizeof(VK_Intercepts);

int JS_VKeys_Callback(MSG* event, accelerator_register_t*)
{
	const WPARAM& keycode = event->wParam;
	const UINT& uMsg = event->message;

	switch (uMsg) 
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (keycode < 256) 
				VK_KeyDown[keycode] = time_precise();
			break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (keycode < 256) 
				VK_KeyUp[keycode] = time_precise();
			break;
	}

	if ((keycode < 256) && VK_Intercepts[keycode] && (uMsg != WM_KEYUP) && (uMsg != WM_SYSKEYUP)) // Block keystroke, but not when releasing key
		return 1; // Eat keystroke
	else
		return 0; // "Not my window", whatever this means?
}

void JS_VKeys_GetState(double cutoffTime, char* stateOutNeedBig, int stateOutNeedBig_sz)
{
	if (realloc_cmd_ptr(&stateOutNeedBig, &stateOutNeedBig_sz, VK_Size)) {
		if (stateOutNeedBig_sz == VK_Size) {
			if (cutoffTime < 0)
				cutoffTime = time_precise() + cutoffTime;
			for (unsigned int i = 1; i <= VK_Size; i++)
			{
				if (VK_KeyDown[i] > VK_KeyUp[i] && VK_KeyDown[i] > cutoffTime)
					stateOutNeedBig[i-1] = 1;
				else
					stateOutNeedBig[i-1] = 0;
			}
		}
	}
}

void JS_VKeys_GetDown(double cutoffTime, char* stateOutNeedBig, int stateOutNeedBig_sz)
{
	if (realloc_cmd_ptr(&stateOutNeedBig, &stateOutNeedBig_sz, VK_Size)) {
		if (stateOutNeedBig_sz == VK_Size) {
			if (cutoffTime < 0)
				cutoffTime = time_precise() + cutoffTime;
			for (unsigned int i = 1; i <= VK_Size; i++)
			{
				if (VK_KeyDown[i] > cutoffTime)
					stateOutNeedBig[i-1] = 1;
				else
					stateOutNeedBig[i-1] = 0;
			}
		}
	}
}

void JS_VKeys_GetUp(double cutoffTime, char* stateOutNeedBig, int stateOutNeedBig_sz)
{
	if (realloc_cmd_ptr(&stateOutNeedBig, &stateOutNeedBig_sz, VK_Size)) {
		if (stateOutNeedBig_sz == VK_Size) {
			if (cutoffTime < 0)
				cutoffTime = time_precise() + cutoffTime;
			for (unsigned int i = 1; i <= VK_Size; i++)
			{
				if (VK_KeyUp[i] > cutoffTime)
					stateOutNeedBig[i-1] = 1;
				else
					stateOutNeedBig[i-1] = 0;
			}
		}
	}
}

int JS_VKeys_Intercept(int keyCode, int intercept)
{
	using namespace Julian;
	if (0 <= keyCode && keyCode < 256) {
		if (intercept > 0 && VK_Intercepts[keyCode] < 255) VK_Intercepts[keyCode]++;
		else if (intercept < 0 && VK_Intercepts[keyCode] > 0) VK_Intercepts[keyCode]--;

		return VK_Intercepts[keyCode];
	}

	else if (keyCode == -1) {
		int maxIntercept = 0;
		if (intercept > 0) {
			for (int i = 0; i < 256; i++) {
				if (VK_Intercepts[i] < 255) VK_Intercepts[i]++;
				if (VK_Intercepts[i] > maxIntercept) maxIntercept = VK_Intercepts[i];
			}
		}
		else if (intercept < 0) {
			for (int i = 0; i < 256; i++) {
				if (VK_Intercepts[i] > 0) VK_Intercepts[i]--;
				if (VK_Intercepts[i] > maxIntercept) maxIntercept = VK_Intercepts[i];
			}
		}
		else { // intercept == 0
			for (int i = 0; i < 256; i++) {
				if (VK_Intercepts[i] > maxIntercept) maxIntercept = VK_Intercepts[i];
			}
		}
		return maxIntercept;
	}

	return -1;
}

//////////////////////////////////////////////////////////////////////

void* JS_Mem_Alloc(int sizeBytes)
{
	using namespace Julian;
	void* ptr = nullptr;
	if (sizeBytes > 0) {
		ptr = malloc(sizeBytes);
		if (ptr) {
			mapMallocToSize.emplace(ptr, sizeBytes);
		}
	}
	return ptr;
}

bool JS_Mem_Free(void* mallocPointer)
{
	using namespace Julian;
	if (mapMallocToSize.count(mallocPointer))
	{
		free(mallocPointer);
		mapMallocToSize.erase(mallocPointer);
		return true;
	}
	else
		return false;
}

bool JS_Mem_FromString(void* mallocPointer, int offset, const char* packedString, int stringLength)
{
	using namespace Julian;
	if (mapMallocToSize.count(mallocPointer) && offset >= 0) {
		if (mapMallocToSize[mallocPointer] >= offset + stringLength)	{
			memcpy((char*)mallocPointer+offset, packedString, stringLength);
			return true;
		}
	}
	return false;
}

bool JS_String(void* pointer, int offset, int lengthChars, char* bufOutNeedBig, int bufOutNeedBig_sz)
{
	int len = lengthChars < 0 ? 0 : lengthChars;
	if (realloc_cmd_ptr(&bufOutNeedBig, &bufOutNeedBig_sz, len)) {
		if (bufOutNeedBig_sz == len) {
			memcpy(bufOutNeedBig, (char*)pointer+offset, len);
			return true;
		}
	}
	return false;
}

void JS_Int(void* pointer, int offset, int* intOut)
{
	*intOut = ((int32_t*)pointer)[offset];
}

void JS_Byte(void* pointer, int offset, int* byteOut)
{
	*byteOut = (int)(((int8_t*)pointer)[offset]);
}

void JS_Double(void* pointer, int offset, double* doubleOut)
{
	*doubleOut = ((double*)pointer)[offset];
}


///////////////////////////////////////////////////////////////////////////

int JS_Dialog_BrowseForSaveFile(const char* windowTitle, const char* initialFolder, const char* initialFile, const char* extensionList, char* fileNameOutNeedBig, int fileNameOutNeedBig_sz)
{			
	// NeedBig buffers should be 2^15 chars by default
	if (fileNameOutNeedBig_sz < 16000) return -1;

	// Set default extension and filter.
	const char* newExtList = ((strlen(extensionList) > 0) ? extensionList : "All files (*.*)\0*.*\0\0");

	BOOL gotFile = FALSE;
	
#ifdef _WIN32
	// These Windows file dialogs do not understand /, so v0.970 added this quick hack to replace with \.
	size_t folderLen = strlen(initialFolder) + 1; // Include terminating \0.
	char* newInitFolder = (char*)malloc(folderLen);
	if (!newInitFolder) return -2;

	for (size_t i = 0; i < folderLen; i++)
		newInitFolder[i] = (initialFolder[i] == '/') ? '\\' : initialFolder[i];

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
		newInitFolder,				//LPCSTR        lpstrInitialDir;
		windowTitle,			//LPCSTR        lpstrTitle;
		OFN_EXPLORER | OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT, //DWORD         Flags; -- 
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
	gotFile = GetSaveFileName(&info);
	free(newInitFolder);
#else
	// On macOS, this function easily crashes if the extList is empty or not properly formatted.
	try {
		// returns TRUE if file was chosen.
		gotFile = (BOOL)BrowseForSaveFile(windowTitle, initialFolder, initialFile, newExtList, fileNameOutNeedBig, fileNameOutNeedBig_sz);
	}
	catch(...) {
		return -3;
	}
#endif
	if(!gotFile)
		fileNameOutNeedBig[0] = '\0';
	return gotFile;
}

int JS_Dialog_BrowseForOpenFiles(const char* windowTitle, const char* initialFolder, const char* initialFile, const char* extensionList, bool allowMultiple, char* fileNamesOutNeedBig, int fileNamesOutNeedBig_sz)
{
	// Set default extension and filter.
	// OSX dialogs do not understand *.*, and in any case do not show filter text, so don't change if on OSX.
#ifdef __APPLE__
	const char* newExtList = extensionList;
#else
	const char* newExtList = ((strlen(extensionList) > 0) ? extensionList : "All files (*.*)\0*.*\0\0");
#endif

	// GetOpenFileName returns the required buffer length in a mere 2 bytes, so a beffer size of 1024*1024 should be more than enough.
	constexpr uint32_t LONGLEN = 1024 * 1024;

	// If buffer is succesfully created, this will not be NULL anymore, and must be freed.
	char* fileNames = nullptr;
	char* newInitFolder = nullptr;

	// Default retval is -1.  Will only become true once all the steps are completed succesfully.
	int retval = -1;

#ifdef _WIN32
	char slash = '\\';

	// These Windows file dialogs do not understand /, so v0.970 added this quick hack to replace with \.
	size_t folderLen = strlen(initialFolder) + 1; // Include terminating \0.
	newInitFolder = (char*)malloc(folderLen);
	if (!newInitFolder) return -1;

	for (size_t i = 0; i < folderLen; i++)
		newInitFolder[i] = (initialFolder[i] == '/') ? '\\' : initialFolder[i];

	// The potentially very long return string will be stored in here
	fileNames = (char*)malloc(LONGLEN);
	if (fileNames) {
		strncpy(fileNames, initialFile, LONGLEN);
		fileNames[LONGLEN - 1] = 0;

		DWORD flags = allowMultiple ? (OFN_EXPLORER | OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT)
									: (OFN_EXPLORER | OFN_LONGNAMES | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST);

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
			newInitFolder,			//LPCSTR        lpstrInitialDir;
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
	char slash = '/';
	try {
		fileNames = BrowseForFiles(windowTitle, initialFolder, initialFile, allowMultiple, newExtList);
	}
	catch(...) {
		return -3;
	}
			
	retval = (fileNames ? -1 : 0);
	{
#endif
		if (retval) {
			// If allowMulti, filenames will be 0-separated, and entire string will end in a double \0\0.
			size_t totalLen = strlen(fileNames);
			size_t firstLen = totalLen;
			bool addSlash = false;
			if (allowMultiple) {
				for (; totalLen < LONGLEN; totalLen++) {
					if (fileNames[totalLen] == 0 && fileNames[totalLen + 1] == 0)
						break;
				}
			}
			if (totalLen > 0) {
				// WDL/swell returns folder substring with terminal slash, but Windows doesn't.
				// For consistency, add terminal slash.  (Terminal slash also makes it easier to concatenate folder string with file strings.)
				// Check if folder substring ended with slash:
				// (Remember to skip empty folder strings, which may be returned on macOS.)
				if (firstLen > 0 && firstLen < totalLen && !(fileNames[firstLen - 1] == slash)) {
					addSlash = true;
					totalLen = totalLen + 1;
				}

				if (totalLen < LONGLEN) {
					if (realloc_cmd_ptr(&fileNamesOutNeedBig, &fileNamesOutNeedBig_sz, (int)totalLen)) {
						if (fileNamesOutNeedBig && fileNamesOutNeedBig_sz == totalLen) {
							if (addSlash) {
								memcpy(fileNamesOutNeedBig, fileNames, firstLen);
								fileNamesOutNeedBig[firstLen] = slash;
								memcpy(fileNamesOutNeedBig+firstLen+1, fileNames+firstLen, totalLen-1-firstLen);
							}
							else {
								memcpy(fileNamesOutNeedBig, fileNames, totalLen);
							}
							retval = TRUE;
						}
					}
				}
			}
		}
	}
	if (newInitFolder) free(newInitFolder);
	if (fileNames) free(fileNames);
	return retval;
}


// Browsing for folders is quite a mess in WIN32 (compared to swell).
// Must use this weird callback function merely to set initial folder.
#ifdef _WIN32
INT CALLBACK JS_Dialog_BrowseForFolder_CallBack(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM pData)
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
	// These Windows file dialogs do not understand /, so v0.970 added this quick hack to replace with \.
	size_t folderLen = strlen(initialFolder) + 1; // Include terminating \0.
	char* newInitFolder = (char*)malloc(folderLen);
	if (!newInitFolder) return -1;

	for (size_t i = 0; i < folderLen; i++)
		newInitFolder[i] = (initialFolder[i] == '/') ? '\\' : initialFolder[i];

	_browseinfoA info{
		NULL,					//HWND        hwndOwner;
		NULL,					//PCIDLIST_ABSOLUTE pidlRoot;
		folderOutNeedBig,		//pszDisplayName;       // Return display name of item selected.
		caption,				//LPCSTR       lpszTitle; // text to go in the banner over the tree.
		BIF_NEWDIALOGSTYLE | BIF_BROWSEINCLUDEURLS | BIF_RETURNONLYFSDIRS , //UINT         ulFlags;                       // Flags that control the return stuff
		JS_Dialog_BrowseForFolder_CallBack,		//BFFCALLBACK  lpfn;
		(LPARAM)newInitFolder,	//LPARAM       lParam;	// extra info that's passed back in callbacks
		0						//int          iImage;	// output var: where to return the Image index.
	};

	PIDLIST_ABSOLUTE folderID = SHBrowseForFolder(&info);
	if (folderID)
		SHGetPathFromIDList(folderID, folderOutNeedBig);
	ILFree(folderID);
	free(newInitFolder);

	return (BOOL)!!folderID;

#else
	// returns TRUE if path was chosen.
	return BrowseForDirectory(caption, initialFolder, folderOutNeedBig, folderOutNeedBig_sz);
#endif
}


//////////////////////////////////////////////////////////////////////////////////////////////////

bool JS_Window_GetRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut)
{
	RECT r{ 0, 0, 0, 0 };
	bool isOK = !!GetWindowRect((HWND)windowHWND, &r);
#ifdef __APPLE__
	if (r.top < r.bottom) {
#else
	if (r.top > r.bottom) {
#endif
		*topOut	   = (int)r.bottom;
		*bottomOut = (int)r.top;			
	}
	else {
		*topOut	   = (int)r.top;
		*bottomOut = (int)r.bottom;
	}
	*leftOut   = (int)r.left;
	*rightOut  = (int)r.right;

	return (isOK);
}

void JS_Window_ScreenToClient(void* windowHWND, int x, int y, int* xOut, int* yOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	POINT p{ x, y };
	ScreenToClient((HWND)windowHWND, &p);
	*xOut = (int)p.x;
	*yOut = (int)p.y;
}

void JS_Window_ClientToScreen(void* windowHWND, int x, int y, int* xOut, int* yOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	POINT p{ x, y };
	ClientToScreen((HWND)windowHWND, &p);
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
	if (isOK) {
		POINT p{ 0, 0 };
		ClientToScreen(hwnd, &p);
		*leftOut = (int)p.x;
		*topOut = (int)p.y;
		*rightOut = (int)p.x + (int)r.right;
#ifdef __APPLE__
		*bottomOut = (int)p.y - (int)r.bottom; // macOS uses coordinates from bottom, so r.bottom must be smaller than r.top
#else
		*bottomOut = (int)p.y + (int)r.bottom;
#endif
	}
	return (isOK);
}

bool JS_Window_GetClientSize(void* windowHWND, int* widthOut, int* heightOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	// However, if hwnd is not a true hwnd, SWELL will return a {0,0,0,0} rect.
	RECT r{ 0, 0, 0, 0 };
#ifdef _WIN32
	bool isOK = !!GetClientRect((HWND)windowHWND, &r);
#else
	GetClientRect((HWND)windowHWND, &r);
	bool isOK = (r.bottom != 0 || r.right != 0);
#endif
	r.right = (r.right >= 0) ? (r.right) : (-(r.right));
	r.bottom = (r.bottom >= 0) ? (r.bottom) : (-(r.bottom));
	*widthOut = (int)r.right;
	*heightOut = (int)r.bottom;
	return (isOK);
}

void JS_Window_MonitorFromRect(int x1, int y1, int x2, int y2, bool wantWork, int* leftOut, int* topOut, int* rightOut, int* bottomOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	RECT s{ x1, y1, x2, y2 };
#ifdef _WIN32
	HMONITOR m = MonitorFromRect(&s, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO i{ sizeof(MONITORINFO), };
	GetMonitorInfo(m, &i);
	RECT& r = wantWork ? i.rcWork : i.rcMonitor;
#else
	RECT r{ 0,0,0,0 };
	SWELL_GetViewPort(&r, &s, wantWork);
#endif
	
#ifdef __APPLE__
	if (r.top < r.bottom) {
#else
	if (r.top > r.bottom) {
#endif
		*topOut	   = (int)r.bottom;
		*bottomOut = (int)r.top;			
	}
	else {
		*topOut	   = (int)r.top;
		*bottomOut = (int)r.bottom;
	}
	*leftOut = (int)r.left;
	*rightOut = (int)r.right;
}

void JS_Window_GetViewportFromRect(int x1, int y1, int x2, int y2, bool wantWork, int* leftOut, int* topOut, int* rightOut, int* bottomOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	RECT s{ x1, y1, x2, y2 };
#ifdef _WIN32
	HMONITOR m = MonitorFromRect(&s, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO i{ sizeof(MONITORINFO), };
	GetMonitorInfo(m, &i);
	RECT& r = wantWork ? i.rcWork : i.rcMonitor;
#else
	RECT r{ 0,0,0,0 };
	SWELL_GetViewPort(&r, &s, wantWork);
#endif

#ifdef __APPLE__
	if (r.top < r.bottom) {
#else
	if (r.top > r.bottom) {
#endif
		*topOut = (int)r.bottom;
		*bottomOut = (int)r.top;
	}
	else {
		*topOut = (int)r.top;
		*bottomOut = (int)r.bottom;
	}
	*leftOut = (int)r.left;
	*rightOut = (int)r.right;
	}

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
	if (ValidatePtr(windowHWND, "HWND"))
		EnableWindow((HWND)windowHWND, (BOOL)enable); // (enable ? (int)1 : (int)0));
}

void  JS_Window_Destroy(void* windowHWND)
{
	if (ValidatePtr(windowHWND, "HWND"))
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
	if (ValidatePtr(windowHWND, "HWND"))
	{
		int intState;
		if		(strstr(state, "SHOWNA"))	intState = SW_SHOWNA;
		else if (strstr(state, "NOACT"))	intState = SW_SHOWNOACTIVATE;
		else if (strstr(state, "MINI"))		intState = SW_SHOWMINIMIZED;
		else if (strstr(state, "HIDE"))		intState = SW_HIDE;
		else if (strstr(state, "MAX"))		intState = SW_SHOWMAXIMIZED;
		else if (strstr(state, "REST"))		intState = SW_RESTORE;
		else if (strstr(state, "DEF"))		intState = SW_SHOWDEFAULT;
		else if (strstr(state, "NORM"))		intState = SW_NORMAL;
		else if (strstr(state, "HIDE"))		intState = SW_HIDE;
		else intState = SW_SHOW;
		ShowWindow((HWND)windowHWND, intState);
	}
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
	else if (strstr(info, "INSTANCE"))	intMode = GWLP_HINSTANCE;
	else if (strstr(info, "PARENT"))	intMode = GWLP_HWNDPARENT;
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

void JS_Window_GetLong(void* windowHWND, const char* info, double* retvalOut)
{
	int intMode;

#ifdef _WIN32
	if (strstr(info, "USER"))			intMode = GWLP_USERDATA;
	else if (strstr(info, "WND"))		intMode = GWLP_WNDPROC;
	else if (strstr(info, "ID"))		intMode = GWLP_ID;
	else if (strstr(info, "EXSTYLE"))	intMode = GWL_EXSTYLE;
	else if (strstr(info, "STYLE"))		intMode = GWL_STYLE;
	else if (strstr(info, "INSTANCE"))	intMode = GWLP_HINSTANCE;
	else if (strstr(info, "PARENT"))	intMode = GWLP_HWNDPARENT;
	else {*retvalOut = 0; return;}

	*retvalOut = (double)GetWindowLongPtr((HWND)windowHWND, intMode);

#else 
	if (strstr(info, "USER"))			intMode = GWL_USERDATA;
	else if (strstr(info, "WND"))		intMode = GWL_WNDPROC;
	else if (strstr(info, "DLG"))		intMode = DWL_DLGPROC;
	else if (strstr(info, "ID"))		intMode = GWL_ID;
	else if (strstr(info, "EXSTYLE"))	intMode = GWL_EXSTYLE;
	else if (strstr(info, "STYLE"))		intMode = GWL_STYLE;
	else {*retvalOut = 0; return;}

	*retvalOut = (double)GetWindowLong((HWND)windowHWND, intMode);
#endif
}

void JS_Window_SetLong(void* windowHWND, const char* info, double value, double* retvalOut)
{
	int intMode;

#ifdef _WIN32
	if (strstr(info, "USER"))			intMode = GWLP_USERDATA;
	else if (strstr(info, "WND"))		intMode = GWLP_WNDPROC;
	else if (strstr(info, "ID"))		intMode = GWL_ID;
	else if (strstr(info, "EXSTYLE"))	intMode = GWL_EXSTYLE;
	else if (strstr(info, "STYLE"))		intMode = GWL_STYLE;
	else if (strstr(info, "INSTANCE"))	intMode = GWLP_HINSTANCE;
	else if (strstr(info, "PARENT"))	intMode = GWLP_HWNDPARENT;
	else { *retvalOut = 0; return; }

	*retvalOut = (double)SetWindowLongPtr((HWND)windowHWND, intMode, (LONG_PTR)value);

#else 
	if (strstr(info, "USER"))			intMode = GWL_USERDATA;
	else if (strstr(info, "WND"))		intMode = GWL_WNDPROC;
	else if (strstr(info, "DLG"))		intMode = DWL_DLGPROC;
	else if (strstr(info, "ID"))		intMode = GWL_ID;
	else if (strstr(info, "EXSTYLE"))	intMode = GWL_EXSTYLE;
	else if (strstr(info, "STYLE"))		intMode = GWL_STYLE;
	else { *retvalOut = 0; return; }

	*retvalOut = (double)SetWindowLong((HWND)windowHWND, intMode, (LONG_PTR)value);
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

BOOL CALLBACK JS_Window_FindTop_Callback_Top(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows& s = *(reinterpret_cast<sEnumWindows*>(structPtr));
	s.temp[0] = '\0';
	GetWindowText(hwnd, s.temp, s.tempLen); // WARNING: swell only returns a BOOL, not the title length
	s.temp[s.tempLen - 1] = '\0'; // Make sure that loooong titles are properly terminated.
	for (unsigned int i = 0; (s.temp[i] != '\0') && (i < s.tempLen); i++) s.temp[i] = (char)tolower(s.temp[i]); // FindWindow is case-insensitive, so this implementation is too
	if (	 (s.exact  && (strcmp(s.temp, s.target) == 0))
		|| (!(s.exact) && (strstr(s.temp, s.target) != NULL)))
	{
		s.foundHWNDs->insert(hwnd);
		return FALSE;
	}
	else
		return TRUE;
}

// Cockos SWELL doesn't provide FindWindow, and FindWindowEx doesn't provide the NULL, NULL top-level mode,
//		so must code own implementation...
// This implemetation optionally matches substrings.
void* JS_Window_FindTop(const char* title, bool exact)
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
	EnumWindows(JS_Window_FindTop_Callback_Top, reinterpret_cast<LPARAM>(&e));
	if (foundHWNDs.size())
		return *(foundHWNDs.begin());
	else
		return NULL;
}


/////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK JS_Window_Find_Callback_Child(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows& s = *(reinterpret_cast<sEnumWindows*>(structPtr));
	s.temp[0] = '\0';
	GetWindowText(hwnd, s.temp, s.tempLen); // WARNING: swell only returns a BOOL, not the title length.
	s.temp[s.tempLen - 1] = '\0'; // Make sure that loooong titles are properly terminated.
	for (unsigned int i = 0; (s.temp[i] != '\0') && (i < s.tempLen); i++) s.temp[i] = (char)tolower(s.temp[i]); // FindWindow is case-insensitive, so this implementation is too
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
	s.temp[0] = '\0';
	GetWindowText(hwnd, s.temp, s.tempLen); // WARNING: swell only returns a BOOL, not the title length
	s.temp[s.tempLen-1] = '\0'; // Make sure that loooong titles are properly terminated.
	for (unsigned int i = 0; (s.temp[i] != '\0') && (i < s.tempLen); i++) s.temp[i] = (char)tolower(s.temp[i]); // FindWindow is case-insensitive, so this implementation is too
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
	s.temp[0] = '\0';
	GetWindowText(hwnd, s.temp, s.tempLen); // WARNING: swell only returns a BOOL, not the title length.
	for (unsigned int i = 0; (s.temp[i] != '\0') && (i < s.tempLen); i++) s.temp[i] = (char)tolower(s.temp[i]); // Convert to lowercase
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

	s.temp[0] = '\0';
	GetWindowText(hwnd, s.temp, s.tempLen); // WARNING: swell only returns a BOOL, not the title length.
	// Make sure that loooong titles are properly terminated.
	s.temp[s.tempLen - 1] = '\0';
	// FindWindow is case-insensitive, so this implementation is too. Convert to lowercase.
	for (unsigned int i = 0; (s.temp[i] != '\0') && (i < s.tempLen); i++) s.temp[i] = (char)tolower(s.temp[i]);
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

	s.temp[0] = '\0';
	GetWindowText(hwnd, s.temp, s.tempLen); // WARNING: swell only returns a BOOL, not the title length.
	// Make sure that loooong titles are properly terminated.
	s.temp[s.tempLen - 1] = '\0'; 
	// FindWindow is case-insensitive, so this implementation is too. Convert to lowercase.
	for (unsigned int i = 0; (s.temp[i] != '\0') && (i < s.tempLen); i++) s.temp[i] = (char)tolower(s.temp[i]);
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
	EnumWindows(JS_Window_ListAllTop_Callback, reinterpret_cast<LPARAM>(&foundHWNDs));

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
// Window_Create and other functions using SetWindowLong

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

// swell uses CreateDialog instead of CreateWindow to create new windows, and the callback returns INT_PTR instead of LRESULT
#ifdef _WIN32
typedef LRESULT callbacktype;
#else
typedef INT_PTR callbacktype;
#endif

callbacktype CALLBACK JS_Window_Create_WinProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
		//case WM_PAINT:
		//    return FALSE;
	case WM_CLOSE:
	case WM_DESTROY:
		DestroyWindow(hwnd);
		return FALSE;
		//case WM_DESTROY:
		//	PostQuitMessage(0);
		//	return FALSE;
	default:
		return (callbacktype)DefWindowProc(hwnd, msg, wParam, lParam);
	}
}

#ifndef _WIN32
#define WS_SIZEBOX WS_THICKFRAME
#define WS_OVERLAPPEDWINDOW (WS_SIZEBOX|WS_SYSMENU|WS_CAPTION)
#define WS_OVERLAPPED WS_CAPTION
#define WS_MAXIMIZEBOX (WS_CAPTION|WS_SIZEBOX)
#define WS_MINIMIZEBOX WS_CAPTION
#define WS_BORDER WS_CAPTION
#define WS_DLGFRAME WS_CAPTION
#define WS_CLIPCHILDREN 0
#define WS_POPUP 0
#endif

void JS_ConvertStringToStyle(char* styleString, DWORD* style, int* show)
{
	*show = SW_SHOW; // Default values if styleOptional not specified.
	*style = WS_OVERLAPPEDWINDOW;

	if (styleString && *styleString) {
		*style = 0;
		// To distinguish MAXIMIZEBOX from MAXIMIZE, alter the M of all MAXIMIZEBOX's.
		// swell doesn't implement WS_SHOWMAXIMIZED and WS_SHOWMINIMIZED, so will use ShowWindow's options instead.
		char* box;
		while (box = strstr(styleString, "MAXIMIZEBOX")) { *style |= (WS_MAXIMIZEBOX | WS_SYSMENU); *box = 'N'; }
		if (strstr(styleString, "MAXIMIZE"))		*show = SW_SHOWMAXIMIZED;
		while (box = strstr(styleString, "MINIMIZEBOX")) { *style |= (WS_MINIMIZEBOX | WS_SYSMENU); *box = 'N'; }
		if (strstr(styleString, "MINIMIZE") || strstr(styleString, "ICONIC")) *show = SW_SHOWMINIMIZED;

		if (strstr(styleString, "CHILD"))			*style |= WS_CHILD;
		//if (strstr(styleString, "CHILDWINDOW"))	*style |= WS_CHILDWINDOW;
		if (strstr(styleString, "CLIPSIBLINGS"))	*style |= WS_CLIPSIBLINGS;
		if (strstr(styleString, "DISABLED"))		*style |= WS_DISABLED;
		if (strstr(styleString, "VISIBLE"))			*style |= WS_VISIBLE;
		if (strstr(styleString, "CAPTION"))			*style |= WS_CAPTION;
		if (strstr(styleString, "VSCROLL"))			*style |= WS_VSCROLL;
		if (strstr(styleString, "HSCROLL"))			*style |= WS_HSCROLL;
		if (strstr(styleString, "SYSMENU"))			*style |= WS_SYSMENU;
		if (strstr(styleString, "THICKFRAME")  || strstr(styleString, "SIZEBOX"))			*style |= WS_SIZEBOX;
		if (strstr(styleString, "GROUP"))			*style |= WS_GROUP;
		if (strstr(styleString, "TABSTOP"))			*style |= WS_TABSTOP;
		if (strstr(styleString, "OVERLAPPED")  || strstr(styleString, "TILED"))				*style |= WS_OVERLAPPED;
		if (strstr(styleString, "TILEDWINDOW") || strstr(styleString, "OVERLAPPEDWINDOW"))	*style |= WS_OVERLAPPEDWINDOW;
		if (strstr(styleString, "DLGFRAME"))		*style |= WS_DLGFRAME;
		if (strstr(styleString, "BORDER"))			*style |= WS_BORDER;
		if (strstr(styleString, "CLIPCHILDREN"))	*style |= WS_CLIPCHILDREN;
		if (strstr(styleString, "POPUP"))			{ *style &= (~(WS_CAPTION | WS_CHILD)); *style |= WS_POPUP; } // swell doesn't actually implement WS_POPUP as separate *style
	}
}

void* JS_Window_Create(const char* title, const char* className, int x, int y, int w, int h, char* styleOptional, void* ownerHWNDOptional)
{
	using namespace Julian;
	HWND hwnd = nullptr; // Default return value if everything doesn't go OK.
	
	if ((ownerHWNDOptional==nullptr) || ValidatePtr((HWND)ownerHWNDOptional, "HWND")) // NULL owner is allowed, but not an invalid one
	{
		int show;
		DWORD style;
		JS_ConvertStringToStyle(styleOptional, &style, &show);

		// On Windows, each new class name requires a new class.
	#ifdef _WIN32
		// Does the class already exist?
		std::string classString = className;
		if (!mapClassNames.count(classString))
		{
			WNDCLASSEX structWindowClass
			{
				sizeof(WNDCLASSEX),		//UINT cbSize;
				CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW | CS_OWNDC, //UINT style;
				JS_Window_Create_WinProc, //WNDPROC     lpfnWndProc;
				0,			//int         cbClsExtra;
				0,			//int         cbWndExtra;
				ReaScriptAPI_Instance,		//HINSTANCE   hInstance;
				NULL,		//HICON       hIcon;
				NULL,		//HCURSOR     hCursor;
				NULL,		//HBRUSH      hbrBackground;
				NULL,		//LPCSTR      lpszMenuName;
				className,	//LPCSTR      lpszClassName;
				NULL,		//HICON       hIconSm;
			};

			if (RegisterClassEx(&structWindowClass))
				mapClassNames[classString] = strdup(className);
		}

		if (mapClassNames.count(classString))
		{
			hwnd = CreateWindowEx(
				WS_EX_LEFT | WS_EX_ACCEPTFILES | WS_EX_APPWINDOW | WS_EX_CONTEXTHELP,	//DWORD     dwExStyle,
				mapClassNames[classString], 	//LPCSTR    lpClassName,
				title, 	//LPCSTR    lpWindowName,
				style, //WS_POPUP, //WS_OVERLAPPEDWINDOW, //WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_VISIBLE | WS_MINIMIZEBOX,	//DWORD     dwStyle,
				x, 		//int       X,
				y, 		//int       Y,
				w, 		//int       nWidth,
				h, 		//int       nHeight,
				(HWND)ownerHWNDOptional,	//HWND      hWndParent,
				NULL,	//HMENU     hMenu,
				ReaScriptAPI_Instance,//HINSTANCE hInstance,
				NULL	//LPVOID    lpParam
			);
			if (hwnd)
			{
				ShowWindow(hwnd, show);
				UpdateWindow(hwnd);
			}
		}

	#else
		hwnd = CreateDialog(nullptr, MAKEINTRESOURCE(0), nullptr, JS_Window_Create_WinProc);
		if (hwnd) {
			// Does the class already exist?
			std::string classString = className;
			if (!mapClassNames.count(classString))
				mapClassNames[classString] = strdup(className);
			SWELL_SetClassName(hwnd, mapClassNames[classString]);
			SetWindowLong(hwnd, GWL_STYLE, style);
			SetWindowText(hwnd, title);
			SetWindowPos(hwnd, HWND_TOP, x, y, w, h, SWP_SHOWWINDOW | SWP_NOCOPYBITS | SWP_FRAMECHANGED);
			#ifdef __APPLE__
			JS_Window_SetZOrder_ObjC(hwnd, HWND_TOP); // swell's SetWindowPos doesn't work well for Z-ordering
			#endif
			ShowWindow(hwnd, show);
			//UpdateWindow(hwnd);
		}
	#endif
	}
	return hwnd;
}

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

// Gets the NSWindowLevel on macOS
int JS_GetLevel(void* hwnd)
{
	#ifdef __APPLE__
	return JS_GetLevel_ObjC(hwnd);
	#else
	return 0;
	#endif
}
	
// swell's Z ordering doesn't work well, and doesn't even interpret TOPMOST and NOTOPMOST.
// So I tried to code my own Z ordering
bool JS_Window_SetZOrder(void* windowHWND, const char* ZOrder, void* insertAfterHWNDOptional)
{
	constexpr intptr_t CHECK_NO_FLAG = -3; // Some value that should not be one of the existing flags.
	if (ValidatePtr(windowHWND, "HWND")) 
	{
		HWND insertAfter = (HWND)CHECK_NO_FLAG;
		if (strstr(ZOrder, "BO"))		insertAfter = HWND_BOTTOM;
		else if (strstr(ZOrder, "NOT"))		insertAfter = HWND_NOTOPMOST;
		else if (strstr(ZOrder, "TOPM"))	insertAfter = HWND_TOPMOST;
		else if (strstr(ZOrder, "TOP"))		insertAfter = HWND_TOP; // Top
		#ifndef __linux__ // swell doesn't provide all options
		else if (strstr(ZOrder, "IN")) 
		{
			if (insertAfterHWNDOptional && ValidatePtr(insertAfterHWNDOptional, "HWND"))
				insertAfter = (HWND)insertAfterHWNDOptional;
		}
		#endif

		if (insertAfter != (HWND)CHECK_NO_FLAG) { // Was given a proper new value?
		#ifdef _WIN32
			return !!SetWindowPos((HWND)windowHWND, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		#elif __linux__
			SetWindowPos((HWND)windowHWND, insertAfter, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
			return true;
		#else
			return JS_Window_SetZOrder_ObjC(windowHWND, insertAfter);
		#endif
		}
	}
	return false;
}

void JS_Window_Update(HWND windowHWND)
{
	UpdateWindow(windowHWND);
}

bool JS_Window_InvalidateRect(HWND windowHWND, int left, int top, int right, int bottom, bool eraseBackground)
{
	RECT rect{ left, top, right, bottom };
	return InvalidateRect(windowHWND, &rect, (BOOL)eraseBackground);
}

bool JS_Window_SetOpacity(HWND windowHWND, const char* mode, double value)
{
	// Opacity can only be applied to top-level framed windows, AFAIK, and in Linux, REAPER crashes if opacity is applied to a child window.
	bool OK = false;
	if (ValidatePtr(windowHWND, "HWND"))
	{
#ifdef _WIN32
		windowHWND = GetAncestor(windowHWND, GA_ROOT);
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
			}
		}
#elif __linux__
		GetNextAncestorWindow:
		{
			// Definitions of swell's HWND, m_oswindow etc can be found in swell-internal.h
			if (windowHWND->m_oswindow) // Does this HWND correspond to a GDKWindow?
			{
				GdkWindow* w = gdk_window_get_effective_toplevel((GdkWindow*)windowHWND->m_oswindow);
				if (w)
				{
					gdk_window_set_opacity(w, value);
					OK = true;
				}
			}
			else if (windowHWND->m_parent) // Else, try to go high in hierarchy, until oswindow is found
			{
				windowHWND = windowHWND->m_parent;
				goto GetNextAncestorWindow;
			}
			//else // Oops, nowhere to go, just return
			//	break;
		}
#elif __APPLE__
		OK = JS_Window_SetOpacity_ObjC(windowHWND, value);
#endif
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

//---------------------------------------------------------------------------------
// WDL/swell has not implemented IsWindow in Linux., and IsWindow is slow in MacOS.
// If scripts or extensions try to access a HWND that doesn't exist any more,
//		REAPER may crash completely.
// AFAIK, this implementation only searches WDL/swell windows, but this shouldn't be a problem,
//		since scripts are in any case only supposed to access REAPER's windows.
BOOL CALLBACK JS_Window_IsWindow_Callback_Child(HWND hwnd, LPARAM lParam)
{
	HWND& target = *(reinterpret_cast<HWND*>(lParam));
	if (hwnd == target)
	{
		target = nullptr;
		return FALSE;
	}
	else
		return TRUE;
}

BOOL CALLBACK JS_Window_IsWindow_Callback_Top(HWND hwnd, LPARAM lParam)
{
	HWND& target = *(reinterpret_cast<HWND*>(lParam));
	if (hwnd == target)
		target = nullptr;
	else
		EnumChildWindows(hwnd, JS_Window_IsWindow_Callback_Child, lParam);

	if (!target)
		return FALSE;
	else
		return TRUE;
}

bool  JS_Window_IsWindow(void* windowHWND)
{
#ifdef _WIN32
	return !!IsWindow((HWND)windowHWND);
#else
	if (Julian::REAPER_VERSION >= 5.974)
		return ValidatePtr(windowHWND, "HWND");
	else {
		// This implementation will enumerate all WDL/swell windows, looking for a match.
		// The "target" HWND will be passed to each callback function.
		// If a match is found, target will be replaced with NULL;
		if (!windowHWND) return false;
		HWND target = (HWND)windowHWND;

		HWND editor = MIDIEditor_GetActive();
		if (editor)
		{
			if (target == editor)
				return true;
			HWND midiview = GetDlgItem(editor, 1000);
			if (target == midiview)
				return true;
		}

		/*HWND main = GetMainHwnd();
		if (main) {
			if (target == main)
				return true;
			else {
				EnumChildWindows(main, JS_Window_IsWindow_Callback_Child, reinterpret_cast<LPARAM>(&target));
				if (!target) return true;
		}
	}*/

		EnumWindows(JS_Window_IsWindow_Callback_Top, reinterpret_cast<LPARAM>(&target));

		return !target;
	}
#endif
}



bool JS_WindowMessage_ListIntercepts(void* windowHWND, char* listOutNeedBig, int listOutNeedBig_sz)
{
	using namespace Julian;
	listOutNeedBig[0] = '\0';
	
	if (mapWindowData.count((HWND)windowHWND))
	{
		auto& messages = mapWindowData[(HWND)windowHWND].mapMessages;
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

bool JS_WindowMessage_Post(void* windowHWND, const char* message, double wParam, int wParamHighWord, double lParam, int lParamHighWord)
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
	
	WPARAM wP;
	if (wParamHighWord || ((wParam < 0) && (-(2^15) > wParam))) // WARNING: Negative values (such as mousewheel turns) are not bitwise encoded the same in low WORD vs entire WPARAM. So if small negative, assume that low WORD is intended.
		wP = MAKEWPARAM(wParam, wParamHighWord);
	else
		wP = (WPARAM)(int64_t)wParam;
		
	LPARAM lP;
	if (lParamHighWord || ((lParam < 0) && (-(2 ^ 15) > lParam)))
		lP = MAKELPARAM(lParam, lParamHighWord);
	else
		lP = (LPARAM)(int64_t)lParam;
		
	HWND hwnd = (HWND)windowHWND;

	// Is this window currently being intercepted?
	if (mapWindowData.count(hwnd)) {
		sWindowData& w = mapWindowData[hwnd];
		if (w.mapMessages.count(uMsg)) {
			w.origProc(hwnd, uMsg, wP, lP); // WindowProcs usually return 0 if message was handled.  But not always, 
			return true;
		}
	}
	return !!PostMessage(hwnd, uMsg, wP, lP);
}


int JS_WindowMessage_Send(void* windowHWND, const char* message, double wParam, int wParamHighWord, double lParam, int lParamHighWord)
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
	
	WPARAM wP;
	if (wParamHighWord || ((wParam < 0) && (-(2 ^ 15) > wParam))) // WARNING: Negative values (such as mousewheel turns) are not bitwise encoded the same in low WORD vs entire WPARAM. So if small negative, assume that low WORD is intended.
		wP = MAKEWPARAM(wParam, wParamHighWord);
	else
		wP = (WPARAM)(int64_t)wParam;

	LPARAM lP;
	if (lParamHighWord || ((lParam < 0) && (-(2 ^ 15) > lParam)))
		lP = MAKELPARAM(lParam, lParamHighWord);
	else
		lP = (LPARAM)(int64_t)lParam;

	return (int)SendMessage((HWND)windowHWND, uMsg, wP, lP);
}

// swell does not define these macros:
#ifndef GET_KEYSTATE_WPARAM
#define GET_KEYSTATE_WPARAM(wParam) LOWORD(wParam)
#endif
#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wParam) ((short)HIWORD(wParam))
#endif


bool JS_Window_OnCommand(void* windowHWND, int commandID)
{
	return JS_WindowMessage_Post(windowHWND, "WM_COMMAND", commandID, 0, 0, 0);
}


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

	if (mapWindowData.count((HWND)windowHWND))
	{
		sWindowData& w = mapWindowData[(HWND)windowHWND];

		if (w.mapMessages.count(uMsg))
		{
			sMsgData& m = w.mapMessages[uMsg];

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
	if (mapWindowData.count(hwnd) == 0)
		return 1;

	// INTERCEPT / BLOCK WINDOW MESSAGES
	sWindowData& windowData = mapWindowData[hwnd]; // Get reference/alias because want to write into existing struct.

	// Event that should be intercepted? 
	if (windowData.mapMessages.count(uMsg)) // ".contains" has only been implemented in more recent C++ versions
	{
		windowData.mapMessages[uMsg].time = time_precise();
		windowData.mapMessages[uMsg].wParam = wParam;
		windowData.mapMessages[uMsg].lParam = lParam;

		// If event will not be passed through, can quit here.
		if (windowData.mapMessages[uMsg].passthrough == false)
		{
			// Most WM_ messages return 0 if processed, with only a few exceptions:
			switch (uMsg)
			{
			case WM_SETCURSOR:
			case WM_DRAWITEM:
			case WM_COPYDATA:
			case WM_ERASEBKGND:
				return 1;
			case WM_MOUSEACTIVATE:
				return 3;
			default:
				return 0;
			}
		}
	}

	// All messages that aren't blocked, end up here

	// COMPOSITE LICE BITMAPS - if any
	if (uMsg == WM_PAINT && !windowData.mapBitmaps.empty())
	{
		RECT r{ 0,0,0,0 };
		GetClientRect(hwnd, &r);
		InvalidateRect(hwnd, &r, false);  // If entire window isn't redrawn, and if compositing destination falls outside invalidated area, bitmap may be composited multiple times over itself.

		LRESULT result = windowData.origProc(hwnd, uMsg, wParam, lParam);

		HDC windowDC = GetDC(hwnd);
		if (windowDC) {
			for (auto& b : windowData.mapBitmaps) {
				if (LICEBitmaps.count(b.first)) {
					HDC& bitmapDC = LICEBitmaps[b.first];
					if (bitmapDC) {
						sBitmapData& i = b.second;
						if (i.dstw != -1) {
							r.left = i.dstx; r.right = i.dstw;
						}
						if (i.dsth != -1) {
							r.top = i.dsty; r.bottom = i.dsth;
						}
#ifdef _WIN32
						AlphaBlend(windowDC, r.left, r.top, r.right, r.bottom, bitmapDC, i.srcx, i.srcy, i.srcw, i.srch, BLENDFUNCTION{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
#else
						StretchBlt(windowDC, r.left, r.top, r.right, r.bottom, bitmapDC, i.srcx, i.srcy, i.srcw, i.srch, SRCCOPY_USEALPHACHAN);
#endif
					}
				}
			}
			ReleaseDC(hwnd, windowDC);
		}
		return result;
	}

	// NO COMPOSITING - just return original results
	else
		return windowData.origProc(hwnd, uMsg, wParam, lParam);
}

		//LRESULT result = windowData.origProc(hwnd, uMsg, wParam, lParam); // PRF_CLIENT | PRF_, );
		/*
		cW, cH = GetClientRect
		RECT rr{ 10000, 10000, 0, 0 };
		if (uMsg == WM_PAINT) {
			if (!windowData.mapBitmaps.empty()) {
				for (auto& b : windowData.mapBitmaps) {
					sBitmapData& i = b.second;
					if (i.dstx < rr.left) rr.left = i.dstx;
					if (i.dsty < rr.top)  rr.top  = i.dsty;
					if (i.dstx+i.dstw > rr.right) rr.right = i.dstx+i.dstw;
					if (i.dsty+i.dsth > rr.bottom) rr.bottom = i.dsty+i.dsth;
					InvalidateRect(hwnd, &rr, true);
				}
			}
		}

		// All events that are not blocked
		LRESULT r = windowData.origProc(hwnd, uMsg, wParam, lParam);
		*/

		/*
		if (uMsg == WM_PAINT) {
			if (!windowData.mapBitmaps.empty()) {
				HDC windowDC = GetDC(hwnd);
				if (windowDC) {
					for (auto& b : windowData.mapBitmaps) {
						if (LICEBitmaps.count(b.first)) {
							HDC& bitmapDC = LICEBitmaps[b.first];
							if (bitmapDC) {
								sBitmapData& i = b.second;
								RECT r{ i.dstx, i.dsty, i.dstw, i.dsth };
								if (i.dstw == -1 || i.dsth == -1) {
									GetClientRect(hwnd, &r);
									if (i.dstw != -1) {
										r.left = i.dstx; r.right = i.dsty;
									}
									else if (i.dsth != -1) {
										r.top = i.dsty; r.bottom = i.dsth;
									}
								}
#ifdef _WIN32
								AlphaBlend(memDC, r.left, r.top, r.right, r.bottom, bitmapDC, i.srcx, i.srcy, i.srcw, i.srch, BLENDFUNCTION{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
#else
								StretchBlt(windowDC, r.left, r.top, r.right, r.bottom, bitmapDC, i.srcx, i.srcy, i.srcw, i.srch, SRCCOPY_USEALPHACHAN);
#endif
							}
						}
					}
					//ReleaseDC(hwnd, windowDC);
				}
			}
		}
		
		//BitBlt(windowDC, 0, 0, r.right, r.bottom, memDC, 0, 0, SRCCOPY);

		//SelectObject(memDC, oldbitmap);
		DeleteObject(hbitmap);
		DeleteDC(memDC);
		
		return result;
	}
}*/


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
	if (Julian::mapWindowData.count(hwnd) == 0) 
	{
		if (!JS_Window_IsWindow(hwnd)) 
			return ERR_NOT_WINDOW;

		HDC windowDC = (HDC)JS_GDI_GetClientDC(hwnd);
		if (!windowDC) 
			return ERR_WINDOW_HDC;

		// Try to get the original process.
		WNDPROC origProc = nullptr;
		#ifdef _WIN32
		origProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
		#else
		origProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
		#endif
		if (!origProc) 
			return ERR_ORIGPROC;

		Julian::mapWindowData.emplace(hwnd, sWindowData{ origProc }); // , map<UINT, sMsgData>{}, map<LICE_IBitmap*, sBitmapData>{} }); // Insert empty map
	}

	// Window already intercepted.  So try to add to existing maps.
	else
	{
		// Check that no overlaps: only one script may intercept each message type
		if (Julian::mapWindowData[hwnd].mapMessages.count(uMsg))
			return ERR_ALREADY_INTERCEPTED;
	}

	Julian::mapWindowData[hwnd].mapMessages.emplace(uMsg, sMsgData{ passthrough, 0, 0, 0 });
	return 1;
}

int JS_WindowMessage_PassThrough(void* windowHWND, const char* message, bool passThrough)
{
	using namespace Julian;
	HWND hwnd = (HWND)windowHWND;
	UINT uMsg;

	// Is this window already being intercepted?
	if (Julian::mapWindowData.count(hwnd) == 0)
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
	if (Julian::mapWindowData[hwnd].mapMessages.count(uMsg) == 0)
		return ERR_ALREADY_INTERCEPTED;

	// Change passthrough
	Julian::mapWindowData[hwnd].mapMessages[uMsg].passthrough = passThrough;

	return 1;
}

int JS_WindowMessage_InterceptList(void* windowHWND, const char* messages)
{
	using namespace Julian;
	HWND hwnd = (HWND)windowHWND;

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
	if (newMessages.empty())
		return ERR_PARSING;

	// Is this window already being intercepted?
	if (mapWindowData.count(hwnd) == 0) // Not yet intercepted
	{
		// IsWindow is slow in Linux and MacOS. 
		// However, checking may be prudent this may be necessary since Linux will crash if windowHWND is not an actual window.
		if (!JS_Window_IsWindow(hwnd)) 
			return ERR_NOT_WINDOW;

		HDC windowDC = (HDC)JS_GDI_GetClientDC(hwnd);
		if (!windowDC) 
			return ERR_WINDOW_HDC;

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
		Julian::mapWindowData.emplace(hwnd, sWindowData{ origProc, newMessages }); // Insert into static map of namespace
		return 1;
	}

	// Already intercepted.  So add to existing maps.
	else
	{
		// Check that no overlaps: only one script may intercept each message type
		// Want to update existing map, so use aliases/references
		auto& existingMsg = Julian::mapWindowData[hwnd].mapMessages; // Messages that are already being intercepted for this window
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

	if (mapWindowData.count(hwnd) == 0)
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
	auto& existingMessages = Julian::mapWindowData[hwnd].mapMessages; // Messages that are already being intercepted for this window
	for (const UINT& it : messagesToErase)
		existingMessages.erase(it);

	// If no messages need to be intercepted any more, release this window
	if (existingMessages.empty() && mapWindowData[hwnd].mapBitmaps.empty())
		JS_WindowMessage_RestoreOrigProc(hwnd);

	return TRUE;
}

void JS_WindowMessage_ReleaseAll()
{
	using namespace Julian;
	for (auto it = mapWindowData.begin(); it != mapWindowData.end(); ++it) {
		JS_WindowMessage_ReleaseWindow(it->first);
	}
}

void JS_WindowMessage_ReleaseWindow(void* windowHWND)
{
	using namespace Julian;
	HWND hwnd = (HWND)windowHWND;
	if (mapWindowData.count(hwnd)) {
		if (mapWindowData[hwnd].mapBitmaps.empty()) JS_WindowMessage_RestoreOrigProc(hwnd); // no linked bitmaps either, so can restore original WNDPROC
		else mapWindowData[hwnd].mapMessages.clear(); // delete intercepts, but leave linked bitmaps alone
	}
}

static void JS_WindowMessage_RestoreOrigProc(HWND hwnd)
{
	using namespace Julian;

	if (mapWindowData.count(hwnd)) {
		if (JS_Window_IsWindow(hwnd)) {
			WNDPROC origProc = mapWindowData[hwnd].origProc;
#ifdef _WIN32
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)origProc);
#else
			SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)origProc);
#endif
		}
		mapWindowData.erase(hwnd);
	}
}


/////////////////////////////////////////////////////////////////////////////////////////
// Mouse functions

int JS_Mouse_GetState(int flags)
{
	int state = 0;
	if (flags & 1)	 if (GetAsyncKeyState(VK_LBUTTON) >> 1)	state |= 1;
	if (flags & 2)	 if (GetAsyncKeyState(VK_RBUTTON) >> 1)	state |= 2;
	if (flags & 64)  if (GetAsyncKeyState(VK_MBUTTON) >> 1)	state |= 64;
	if (flags & 4)	 if (GetAsyncKeyState(VK_CONTROL) >> 1)	state |= 4;
	if (flags & 8)	 if (GetAsyncKeyState(VK_SHIFT) >> 1)	state |= 8;
	if (flags & 16)  if (GetAsyncKeyState(VK_MENU) >> 1)	state |= 16;
	if (flags & 32)  if (GetAsyncKeyState(VK_LWIN) >> 1)	state |= 32;
	return state;
}
/*
int JS_Mouse_GetHistory(int flags)
{
	int state = 0;
	if (VK_History[VK_LBUTTON]) state |= 1;
	if (VK_History[VK_RBUTTON]) state |= 2;
	if (VK_History[VK_MBUTTON]) state |= 64;
	if (VK_History[VK_CONTROL]) state |= 4;
	if (VK_History[VK_SHIFT]) state |= 8;
	if (VK_History[VK_MENU]) state |= 16;
	if (VK_History[VK_LWIN]) state |= 32;
	state = state & flags;
	return state;
}

void JS_Mouse_ClearHistory()
{
	VK_History[VK_LBUTTON] = 0;
	VK_History[VK_RBUTTON] = 0;
	VK_History[VK_MBUTTON] = 0;
	VK_History[VK_CONTROL] = 0;
	VK_History[VK_SHIFT] = 0;
	VK_History[VK_MENU] = 0;
	VK_History[VK_LWIN] = 0;
}
*/

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

void* JS_Mouse_LoadCursorFromFile(const char* pathAndFileName, bool* forceNewLoadOptional)
{
	using namespace Julian;
	string  file	= pathAndFileName;
	HCURSOR cursor	= NULL;
	if ((forceNewLoadOptional && *forceNewLoadOptional)
		|| !(mapFileToMouseCursor.count(file)))
	{
#ifdef _WIN32
		cursor = LoadCursorFromFile(pathAndFileName);
#else
		cursor = SWELL_LoadCursorFromFile(pathAndFileName);
#endif
		if (cursor)
			mapFileToMouseCursor[file] = cursor;
	}
	else
		cursor = mapFileToMouseCursor[file];

	return cursor;
}

void JS_Mouse_SetCursor(void* cursorHandle)
{
	SetCursor((HCURSOR)cursorHandle);
}

void* JS_Mouse_GetCursor()
{
#ifdef _WIN32
	return (void*)GetCursor();
#else
	return (void*)SWELL_GetLastSetCursor();
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void* JS_GDI_GetClientDC(void* windowHWND)
{
	HDC dc = GetDC((HWND)windowHWND);
	if (dc) Julian::GDIHDCs.emplace(dc);
	return dc;
}

void* JS_GDI_GetWindowDC(void* windowHWND)
{
	HDC dc = GetWindowDC((HWND)windowHWND);
	if (dc) Julian::GDIHDCs.emplace(dc);
	return dc;
}

void* JS_GDI_GetScreenDC()
{
	HDC dc = GetDC(NULL);
	if (dc) Julian::GDIHDCs.emplace(dc);
	return dc;
}

void JS_GDI_ReleaseDC(void* windowHWND, void* deviceHDC)
{
	if (Julian::GDIHDCs.count((HDC)deviceHDC)) {
		Julian::GDIHDCs.erase((HDC)deviceHDC);
		ReleaseDC((HWND)windowHWND, (HDC)deviceHDC);
	}
}


void* JS_GDI_CreateFillBrush(int color)
{
#ifdef _WIN32
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
	HGDIOBJ object = CreateSolidBrush(color);
#else
	HGDIOBJ object = CreateSolidBrushAlpha(color, 1);
#endif
	if (object)
		Julian::setGDIObjects.insert(object);
	return object;
}

void* JS_GDI_CreatePen(int width, int color)
{
#ifdef _WIN32
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
#endif
	HGDIOBJ object = CreatePen(PS_SOLID, width, color);
	if (object)
		Julian::setGDIObjects.insert(object);
	return object;
}

#ifndef FF_DONTCARE
#define FF_DONTCARE 0
#endif
void* JS_GDI_CreateFont(int height, int weight, int angle, bool italic, bool underline, bool strikeOut, const char* fontName)
{
	HGDIOBJ object = CreateFont(height, 0, angle, 0, weight, (BOOL)italic, (BOOL)underline, (BOOL)strikeOut, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, fontName);
	if (object)
		Julian::setGDIObjects.insert(object);
	return object;
}

void* JS_GDI_SelectObject(void* deviceHDC, void* GDIObject)
{
	if (Julian::setGDIObjects.count((HGDIOBJ)GDIObject))
		return SelectObject((HDC)deviceHDC, (HGDIOBJ)GDIObject);
	else
		return nullptr;
}

void JS_GDI_DeleteObject(void* GDIObject)
{
	if (Julian::setGDIObjects.count((HGDIOBJ)GDIObject))
	{
		DeleteObject((HGDIOBJ)GDIObject);
		Julian::setGDIObjects.erase((HGDIOBJ)GDIObject);
	}
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
#ifdef _WIN32
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
#endif
	return color;
}

void JS_GDI_SetTextBkMode(void* deviceHDC, int mode)
{
	SetBkMode((HDC)deviceHDC, mode);
}

void JS_GDI_SetTextBkColor(void* deviceHDC, int color)
{
#ifdef _WIN32
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
#endif
	SetBkColor((HDC)deviceHDC, color);
}

void JS_GDI_SetTextColor(void* deviceHDC, int color)
{
#ifdef _WIN32
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
#endif
	SetTextColor((HDC)deviceHDC, color);
}

int JS_GDI_GetTextColor(void* deviceHDC)
{
	int color = GetTextColor((HDC)deviceHDC);
#ifdef _WIN32
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
#endif
	return color;
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
#ifdef _WIN32
	color = ((color & 0x0000FF) << 16) | (color & 0x00FF00) | ((color & 0xFF0000) >> 16);
#endif
	SetPixel((HDC)deviceHDC, x, y, color);
}



void JS_GDI_Blit(void* destHDC, int dstx, int dsty, void* sourceHDC, int srcx, int srcy, int width, int height, const char* modeOptional)
{
	if (modeOptional && (strchr(modeOptional, 'A') || strchr(modeOptional, 'a')))
#ifdef _WIN32
		AlphaBlend((HDC)destHDC, dstx, dsty, width, height, (HDC)sourceHDC, srcx, srcy, width, height, BLENDFUNCTION { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
#else
		StretchBlt((HDC)destHDC, dstx, dsty, width, height, (HDC)sourceHDC, srcx, srcy, width, height, SRCCOPY_USEALPHACHAN);
#endif
	else
		StretchBlt((HDC)destHDC, dstx, dsty, width, height, (HDC)sourceHDC, srcx, srcy, width, height, SRCCOPY);
}

void JS_GDI_StretchBlit(void* destHDC, int dstx, int dsty, int dstw, int dsth, void* sourceHDC, int srcx, int srcy, int srcw, int srch, const char* modeOptional)
{
	if (modeOptional && (strchr(modeOptional, 'A') || strchr(modeOptional, 'a')))
#ifdef _WIN32
		AlphaBlend((HDC)destHDC, dstx, dsty, dstw, dsth, (HDC)sourceHDC, srcx, srcy, srcw, srch, BLENDFUNCTION{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA });
#else
		StretchBlt((HDC)destHDC, dstx, dsty, dstw, dsth, (HDC)sourceHDC, srcx, srcy, srcw, srch, SRCCOPY_USEALPHACHAN);
#endif
	else
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

////////////////////////////////////////////////////////////////////////////////////////////////
// Functions for compositing into REAPER's UI

int JS_Composite(HWND hwnd, int dstx, int dsty, int dstw, int dsth, LICE_IBitmap* sysBitmap, int srcx, int srcy, int srcw, int srch)
{
	using namespace Julian;
	if (!LICEBitmaps.count(sysBitmap)) return ERR_NOT_BITMAP;
	HDC bitmapDC = LICEBitmaps[sysBitmap]; if (!bitmapDC) return ERR_NOT_SYSBITMAP; // Is this a sysbitmap?

	// If window not already intercepted, get original window proc and emplace new struct
	if (mapWindowData.count(hwnd) == 0) {
		if (!JS_Window_IsWindow(hwnd)) return ERR_NOT_WINDOW;
		
		WNDPROC origProc = nullptr;
#ifdef _WIN32
		origProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
#else
		origProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
#endif
		if (!origProc) return ERR_ORIGPROC;
		
		mapWindowData[hwnd] = sWindowData{ origProc };
	}

	// OK, hwnd should now be in map. Don't use emplace, since may need to replace previous dst or src RECT of already-linked bitmap
	mapWindowData[hwnd].mapBitmaps[sysBitmap] = sBitmapData{ dstx, dsty, dstw, dsth, srcx, srcy, srcw, srch };
	return 1;
}

/*
if (LICEBitmaps.count(sysBitmap)) {
HDC bitmapDC = LICEBitmaps[sysBitmap];
if (bitmapDC) { // Is this a sysbitmap?
if (mapWindowData.count(hwnd) == 0) { // If window not already intercepted, get original window proc and emplace new struct
if (JS_Window_IsWindow(hwnd)) {
WNDPROC origProc = nullptr;
#ifdef _WIN32
origProc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
#else
origProc = (WNDPROC)SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
#endif
if (origProc)
mapWindowData.emplace(hwnd, sWindowData{ origProc });
}
}
if (mapWindowData.count(hwnd)) { // OK, hwnd should now be in map
HDC windowDC = nullptr;
if (mapWindowData[hwnd].mapBitmaps.count(sysBitmap))
windowDC = mapWindowData[hwnd].mapBitmaps[sysBitmap].windowDC;
else
windowDC = (HDC)JS_GDI_GetClientDC(hwnd);

if (windowDC) {
mapWindowData[hwnd].mapBitmaps.emplace(sysBitmap, sBitmapData{ windowDC, dstx, dsty, dstw, dsth, bitmapDC, srcx, srcy, srcw, srch });
return 1;
}
}
}
}
return false;
}*/

void JS_Composite_Unlink(HWND hwnd, LICE_IBitmap* bitmap)
{
	using namespace Julian;
	if (mapWindowData.count(hwnd)) {
		mapWindowData[hwnd].mapBitmaps.erase(bitmap);
		if (mapWindowData[hwnd].mapBitmaps.empty() && mapWindowData[hwnd].mapMessages.empty()) {
			JS_WindowMessage_RestoreOrigProc(hwnd);
		}
	}

}

int JS_Composite_ListBitmaps(HWND hwnd, char* listOutNeedBig, int listOutNeedBig_sz)
{
	using namespace Julian;
	if (mapWindowData.count(hwnd) == 0) return 0;
	std::set<HWND> bitmaps; // Use the helper function that was originally meant for listing window HWNDs.
	for (auto& m : mapWindowData[hwnd].mapBitmaps) bitmaps.emplace((HWND)m.first);
	return ConvertSetHWNDToString(bitmaps, listOutNeedBig, listOutNeedBig_sz);
}

/////////////////////////////////////////////

void* JS_LICE_CreateBitmap(bool isSysBitmap, int width, int height)
{
	LICE_IBitmap* bm = LICE_CreateBitmap((BOOL)isSysBitmap, width, height); // If SysBitmap, can BitBlt to/from screen like HDC.
	// Immediately get HDC and store, so that all scripts can use the same HDC.
	if (bm) {
		HDC dc = LICE__GetDC(bm);
		Julian::LICEBitmaps[bm] = dc;
	}
	return bm;
}

int JS_LICE_GetHeight(void* bitmap)
{
	using namespace Julian;
	if (LICEBitmaps.count((LICE_IBitmap*)bitmap))
		return LICE__GetHeight((LICE_IBitmap*)bitmap);
	else 
		return 0;
}

int JS_LICE_GetWidth(void* bitmap)
{
	using namespace Julian;
	if (LICEBitmaps.count((LICE_IBitmap*)bitmap))
		return LICE__GetWidth((LICE_IBitmap*)bitmap);
	else
		return 0;
}

void* JS_LICE_GetDC(void* bitmap)
{
	using namespace Julian;
	if (LICEBitmaps.count((LICE_IBitmap*)bitmap))
		return LICEBitmaps[(LICE_IBitmap*)bitmap];
	else
		return nullptr;
}


void JS_LICE_DestroyBitmap(LICE_IBitmap* bitmap)
{
	using namespace Julian;
	// Also delete any occurence of this bitmap from UI Compositing
	if (LICEBitmaps.count(bitmap)) {
		for (auto& m : mapWindowData) {
			m.second.mapBitmaps.erase(bitmap);
		}
		LICEBitmaps.erase(bitmap);
		LICE__Destroy(bitmap);
	}
}


#define LICE_BLIT_MODE_MASK 0xff // Similar to CHANCOPY_ATOA
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
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)destBitmap) && Julian::LICEBitmaps.count((LICE_IBitmap*)sourceBitmap))
	{
		if (strstr(mode, "BLUR"))
			LICE_Blur((LICE_IBitmap*)destBitmap, (LICE_IBitmap*)sourceBitmap, dstx, dsty, srcx, srcy, width, height);
		else if (strstr(mode, "ALPHAMUL"))
			JS_LICE_Blit_AlphaMultiply((LICE_IBitmap*)destBitmap, dstx, dsty, (LICE_IBitmap*)sourceBitmap, srcx, srcy, width, height, alpha);
		else {
			GETINTMODE
			if (strstr(mode, "CHANCOPY")) { // This mode is only available for LICE_Blit()
				intMode = LICE_BLIT_MODE_CHANCOPY;
				if (strstr(mode, "_G")) intMode |= 1;
				else if (strstr(mode, "_R")) intMode |= 2;
				else if (strstr(mode, "_A")) intMode |= 3;
				if (strstr(mode, "TOG")) intMode |= 1 << 2;
				else if (strstr(mode, "TOR")) intMode |= 2 << 2;
				else if (strstr(mode, "TOA")) intMode |= 3 << 2;
			}
			LICE_Blit((LICE_IBitmap*)destBitmap, (LICE_IBitmap*)sourceBitmap, dstx, dsty, srcx, srcy, width, height, (float)alpha, intMode);
		}
	}
}

void JS_LICE_RotatedBlit(void* destBitmap, int dstx, int dsty, int dstw, int dsth, void* sourceBitmap, double srcx, double srcy, double srcw, double srch, double angle, double rotxcent, double rotycent, bool cliptosourcerect, double alpha, const char* mode)
{
	GETINTMODE	
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)destBitmap) && Julian::LICEBitmaps.count((LICE_IBitmap*)sourceBitmap))
		LICE_RotatedBlit((LICE_IBitmap*)destBitmap, (LICE_IBitmap*)sourceBitmap, dstx, dsty, dstw, dsth, (float)srcx, (float)srcy, (float)srcw, (float)srch, (float)angle, cliptosourcerect, (float)alpha, intMode, (float)rotxcent, (float)rotycent);
}

void JS_LICE_ScaledBlit(void* destBitmap, int dstx, int dsty, int dstw, int dsth, void* sourceBitmap, double srcx, double srcy, double srcw, double srch, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)destBitmap) && Julian::LICEBitmaps.count((LICE_IBitmap*)sourceBitmap))
		LICE_ScaledBlit((LICE_IBitmap*)destBitmap, (LICE_IBitmap*)sourceBitmap, dstx, dsty, dstw, dsth, (float)srcx, (float)srcy, (float)srcw, (float)srch, (float)alpha, intMode);
}

void JS_LICE_Blur(void* destBitmap, int dstx, int dsty, void* sourceBitmap, int srcx, int srcy, int width, int height)
{
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)destBitmap) && Julian::LICEBitmaps.count((LICE_IBitmap*)sourceBitmap))
		LICE_Blur((LICE_IBitmap*)destBitmap, (LICE_IBitmap*)sourceBitmap, dstx, dsty, srcx, srcy, width, height);
}

void* JS_LICE_LoadPNG(const char* filename)
{
	LICE_IBitmap* sysbitmap = nullptr;
	LICE_IBitmap* png = nullptr;
	sysbitmap = LICE_CreateBitmap(TRUE, 1, 1); // By default, does not return a SysBitmap. In order to force the use of SysBitmaps, use must supply own bitmap.
	if (sysbitmap) {
		png = LICE_LoadPNG(filename, sysbitmap);
		if (png != sysbitmap) LICE__Destroy(sysbitmap);
		if (png) {
			HDC dc = LICE__GetDC(png);
			Julian::LICEBitmaps.emplace(png, dc);
		}
	}
	return png;
}

bool JS_LICE_WritePNG(const char* filename, LICE_IBitmap* bitmap, bool wantAlpha)
{
	return LICE_WritePNG(filename, bitmap, wantAlpha);
}

void JS_LICE_Circle(void* bitmap, double cx, double cy, double r, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_Circle((LICE_IBitmap*)bitmap, (float)cx, (float)cy, (float)r, (LICE_pixel)color, (float)alpha, intMode, antialias);
}

bool JS_LICE_IsFlipped(void* bitmap)
{
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		return LICE__IsFlipped((LICE_IBitmap*)bitmap);
	else
		return false;
}

bool JS_LICE_Resize(void* bitmap, int width, int height)
{
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		return LICE__resize((LICE_IBitmap*)bitmap, width, height);
	else
		return false;
}

void JS_LICE_Arc(void* bitmap, double cx, double cy, double r, double minAngle, double maxAngle, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_Arc((LICE_IBitmap*)bitmap, (float)cx, (float)cy, (float)r, (float)minAngle, (float)maxAngle, (LICE_pixel)color, (float)alpha, intMode, antialias);
}

void JS_LICE_Clear(void* bitmap, int color)
{
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_Clear((LICE_IBitmap*)bitmap, (LICE_pixel)color);
}

bool JS_LICE_Blit_AlphaMultiply(LICE_IBitmap* destBitmap, int dstx, int dsty, LICE_IBitmap* sourceBitmap, int srcx, int srcy, int width, int height, double alpha)
{
	//if (Julian::LICEBitmaps.count((LICE_IBitmap*)destBitmap) && Julian::LICEBitmaps.count((LICE_IBitmap*)sourceBitmap))
	int srcr = srcx + width;
	int srcb = srcy + height;

	// clip to input
	if (srcx < 0) { dstx -= srcx; srcx = 0; }
	if (srcy < 0) { dsty -= srcy; srcy = 0; }
	if (srcr > sourceBitmap->getWidth()) srcr = sourceBitmap->getWidth();
	if (srcb > sourceBitmap->getHeight()) srcb = sourceBitmap->getHeight();

	// clip to output
	if (dstx < 0) { srcx -= dstx; dstx = 0; }
	if (dsty < 0) { srcy -= dsty; dsty = 0; }

	const int destbm_w = destBitmap->getWidth(), destbm_h = destBitmap->getHeight();
	if (srcr <= srcx || srcb <= srcy || dstx >= destbm_w || dsty >= destbm_h) return true;

	if (srcr > srcx + (destbm_w - dstx)) srcr = srcx + (destbm_w - dstx);
	if (srcb > srcy + (destbm_h - dsty)) srcb = srcy + (destbm_h - dsty);

	// ignore blits that are 0
	if (srcr <= srcx || srcb <= srcy) return true;

	int dest_span = destBitmap->getRowSpan(); // *sizeof(LICE_pixel);
	int src_span = sourceBitmap->getRowSpan(); // *sizeof(LICE_pixel);
	const LICE_pixel *psrc = (LICE_pixel *)sourceBitmap->getBits();
	LICE_pixel *pdest = (LICE_pixel *)destBitmap->getBits();
	if (!psrc || !pdest) return false;

	if (sourceBitmap->isFlipped())
	{
		psrc += (sourceBitmap->getHeight() - srcy - 1)*src_span;
		src_span = -src_span;
	}
	else psrc += srcy*src_span;
	psrc += srcx * sizeof(LICE_pixel);

	if (destBitmap->isFlipped())
	{
		pdest += (destbm_h - dsty - 1)*dest_span;
		dest_span = -dest_span;
	}
	else pdest += dsty*dest_span;
	pdest += dstx * sizeof(LICE_pixel);

	height = srcb - srcy;
	width = srcr - srcx;

	LICE_pixel *o, a, a2;
	const LICE_pixel* in;
	while (height--)
	{
		o = pdest;
		in = psrc;
		int cnt = width;
		while (cnt--)
		{
			a = (LICE_pixel)((*in) * alpha) & 0xFF000000;
			if (!a)
				*o = 0;
			else if (a == 0xFF000000)
				*o = *in;
			else {
				a2 = a >> 24;
				*o = a | (((((*in) & 0x00FF0000) * a2) >> 8) & 0x00FF0000) | (((((*in) & 0x0000FF00) * a2) >> 8) & 0x0000FF00) | ((((*in) & 0x000000FF) * a2) >> 8);
			}
			o++;
			in++;
		}
		pdest += dest_span;
		psrc += src_span;
	}
	return true;
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
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		return LICE__DrawText((LICE_IFont*)LICEFont, (LICE_IBitmap*)bitmap, text, textLen, &r, 0); // I don't know what UINT dtFlags does, so make 0.
	else
		return 0;
}

void JS_LICE_DrawChar(void* bitmap, int x, int y, char c, int color, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_DrawChar((LICE_IBitmap*)bitmap, x, y, c, (LICE_pixel)color, (float)alpha, intMode);
}

void JS_LICE_MeasureText(const char* string, int* widthOut, int* heightOut)
{
	LICE_MeasureText(string, widthOut, heightOut);
}

void JS_LICE_FillRect(void* bitmap, int x, int y, int w, int h, int color, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_FillRect((LICE_IBitmap*)bitmap, x, y, w, h, (LICE_pixel)color, (float)alpha, intMode);
}

void JS_LICE_RoundRect(void* bitmap, double x, double y, double w, double h, int cornerradius, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_RoundRect((LICE_IBitmap*)bitmap, (float)x, (float)y, (float)w, (float)h, cornerradius, color, (float)alpha, intMode, antialias);
}

void JS_LICE_GradRect(void* bitmap, int dstx, int dsty, int dstw, int dsth, double ir, double ig, double ib, double ia, double drdx, double dgdx, double dbdx, double dadx, double drdy, double dgdy, double dbdy, double dady, const char* mode)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_GradRect((LICE_IBitmap*)bitmap, dstx, dsty, dstw, dsth, (float)ir, (float)ig, (float)ib, (float)ia, (float)drdx, (float)dgdx, (float)dbdx, (float)dadx, (float)drdy, (float)dgdy, (float)dbdy, (float)dady, intMode);
}

void JS_LICE_FillTriangle(void* bitmap, int x1, int y1, int x2, int y2, int x3, int y3, int color, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_FillTriangle((LICE_IBitmap*)bitmap, x1, y1, x2, y2, x3, y3, color, (float)alpha, intMode);
}

void JS_LICE_FillPolygon(void* bitmap, const char* packedX, const char* packedY, int numPoints, int color, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_FillConvexPolygon((LICE_IBitmap*)bitmap, (int32_t*)packedX, (int32_t*)packedY, numPoints, color, (float)alpha, intMode);
}

void JS_LICE_FillCircle(void* bitmap, double cx, double cy, double r, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_FillCircle((LICE_IBitmap*)bitmap, (float)cx, (float)cy, (float)r, color, (float)alpha, intMode, antialias);
}

void JS_LICE_Line(void* bitmap, double x1, double y1, double x2, double y2, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_Line((LICE_IBitmap*)bitmap, (float)x1, (float)y1, (float)x2, (float)y2, (LICE_pixel)color, (float)alpha, intMode, antialias);
}

void JS_LICE_Bezier(void* bitmap, double xstart, double ystart, double xctl1, double yctl1, double xctl2, double yctl2, double xend, double yend, double tol, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_DrawCBezier((LICE_IBitmap*)bitmap, xstart, ystart, xctl1, yctl1, xctl2, yctl2, xend, yend, (LICE_pixel)color, (float)alpha, intMode, antialias, tol);
}

void JS_LICE_GetPixel(void* bitmap, int x, int y, double* colorOut)
{
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		*colorOut = (double)(LICE_GetPixel((LICE_IBitmap*)bitmap, x, y));
	else
		*colorOut = -1;
}

void JS_LICE_PutPixel(void* bitmap, int x, int y, double color, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_PutPixel((LICE_IBitmap*)bitmap, x, y, (LICE_pixel)color, (float)alpha, intMode);
}

void JS_LICE_SetAlphaFromColorMask(LICE_IBitmap* bitmap, LICE_pixel color)
{
	color &= 0xFFFFFF;
	LICE_SetAlphaFromColorMask(bitmap, color);
	/*if (!bitmap) return;
	LICE_pixel *p = bitmap->getBits();
	int h = bitmap->getHeight();
	int w = bitmap->getWidth();
	int sp = bitmap->getRowSpan();
	if (!p || w<1 || h<1 || sp<1) return;

	while (h-->0)
	{
		int n = w;
		while (n--)
		{
			if ((*p&LICE_RGBA(255, 255, 255, 0)) == color) *p &= LICE_RGBA(255, 255, 255, 0);
			else *p |= LICE_RGBA(0, 0, 0, 255);
			p++;
		}
		p += sp - w;
	}*/
}

void  JS_LICE_AlterBitmapHSV(LICE_IBitmap* bitmap, float hue, float saturation, float value)  // hue is rolled over, saturation and value are clamped, all 0..1
{
	LICE_AlterBitmapHSV(bitmap, hue, saturation, value);
}

void  JS_LICE_AlterRectHSV(LICE_IBitmap* bitmap, int x, int y, int w, int h, float hue, float saturation, float value)  // hue is rolled over, saturation and value are clamped, all 0..1
{
	LICE_AlterRectHSV(bitmap, x, y, w, h, hue, saturation, value);
}

bool JS_LICE_ProcessRect(LICE_IBitmap* bitmap, int x, int y, int w, int h, const char* mode, double operand)
{
	// In order to avoid the overhead of a separate function call for each pixel, 
	//		this code is copied from Cockos's lice.h.
	if (!Julian::LICEBitmaps.count((LICE_IBitmap*)bitmap)) return false;

	if (x<0) { w += x; x = 0; }
	if (y<0) { h += y; y = 0; }

	LICE_pixel *p = bitmap->getBits();
	const int sp = bitmap->getRowSpan();
	const int destbm_w = bitmap->getWidth(), destbm_h = bitmap->getHeight();
	if (!p || !sp || w<1 || h < 1 || x >= destbm_w || y >= destbm_h) return false;

	if (w > destbm_w - x) w = destbm_w - x;
	if (h > destbm_h - y) h = destbm_h - y;

	if (bitmap->isFlipped()) p += (destbm_h - y - h)*sp;
	else p += sp*y;

	p += x;

	if (strstr(mode, "XOR")) {
		LICE_pixel q = (uint32_t)operand;
		LICE_pixel* pout;
		while (h--)	{
			pout = p;
			int n = w;
			while (n--) {
				*pout ^= q;
				pout++;
			}
			p += sp;
		}
	}
	else if (strstr(mode, "AND")) {
		LICE_pixel q = (uint32_t)operand;
		LICE_pixel* pout;
		while (h--) {
			pout = p;
			int n = w;
			while (n--) {
				*pout &= q;
				pout++;
			}
			p += sp;
		}
	}
	else if (strstr(mode, "OR")) {
		LICE_pixel q = (uint32_t)operand;
		LICE_pixel* pout;
		while (h--) {
			pout = p;
			int n = w;
			while (n--) {
				*pout |= q;
				pout++;
			}
			p += sp;
		}
	}
	else if (strstr(mode, "SET_")) {
		LICE_pixel mask = 0xFFFFFFFF; // which channels must be removed from bitmap pixels?
		LICE_pixel* pout;
		if (strchr(mode, 'A')) mask &= 0x00FFFFFF;
		if (strchr(mode, 'R')) mask &= 0xFF00FFFF;
		if (strchr(mode, 'G')) mask &= 0xFFFF00FF;
		if (strchr(mode, 'B')) mask &= 0xFFFFFF00;
		if (mask == 0xFFFFFFFF) return false;
		LICE_pixel q = ((uint32_t)operand) & (uint32_t)(~mask);
		while (h--)	{
			pout = p;
			int n = w;
			while (n--) {
				*pout = ((*pout) & mask) | q;
				pout++;
			}
			p += sp;
		}
	}
	else if (strstr(mode, "ALPHAMUL")) {
		LICE_pixel *pout;
		uint32_t a, a2;
		while (h--) {
			pout = p;
			int n = w;
			while (n--) {
				if (*pout) { // Quickly skip blank pixels
					a = (*pout & 0xFF000000);
					if (a != 0xFF000000) { // And fully opaque pixels also don't need to change
						if (a == 0)
							*pout = 0;
						else {
							a2 = a >> 24; // Normalize 1..256 instead of 0..255
							*pout = a | ((((*pout & 0x00FF0000) * a2) >> 8) & 0x00FF0000) | ((((*pout & 0x0000FF00) * a2) >> 8) & 0x0000FF00) | (((*pout & 0x000000FF) * a2) >> 8);
						}	
					}
				}
				pout++;
			}
			p += sp;
		}
	}
	else
		return false;

	return true;
}

///////////////////////////////////////////////////////////
// Undocumented functions

void JS_Window_AttachTopmostPin(void* windowHWND)
{
	if (ValidatePtr((HWND)windowHWND, "HWND")) {
#ifdef _WIN32
		if (GetWindowLongPtr((HWND)windowHWND, GWL_STYLE) & WS_THICKFRAME)
#elif __APPLE__
		if (GetWindowLong((HWND)windowHWND, GWL_STYLE) & WS_THICKFRAME)
#elif __linux__ // Not yet available on Linux
		return;
#endif
		{
			AttachWindowTopmostButton((HWND)windowHWND);
			SetWindowPos((HWND)windowHWND, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER); // Force re-draw frame, otherwise pin only becomes visible when window is moved.
		}
	}
}

void JS_Window_AttachResizeGrip(void* windowHWND)
{
	if (ValidatePtr((HWND)windowHWND, "HWND")) {
#ifdef _WIN32
		if (GetWindowLongPtr((HWND)windowHWND, GWL_STYLE) & WS_THICKFRAME)
#elif __APPLE__
		if (GetWindowLong((HWND)windowHWND, GWL_STYLE) & WS_THICKFRAME)
#elif __linux__ // Not yet available on Linux
		return;
#endif
		{
			AttachWindowResizeGrip((HWND)windowHWND);
			SetWindowPos((HWND)windowHWND, NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER); // Force re-draw frame, otherwise pin only becomes visible when window is moved.
		}
	}
}


//////////////////////////////////////////////////////////////
// Listview functions

int JS_ListView_GetItemCount(HWND listviewHWND)
{
	return ListView_GetItemCount(listviewHWND);
}

int JS_ListView_GetSelectedCount(HWND listviewHWND)
{
	return ListView_GetSelectedCount(listviewHWND);
}

int JS_ListView_GetFocusedItem(HWND listviewHWND, char* textOut, int textOut_sz)
{
	int index = ListView_GetNextItem(listviewHWND, -1, LVNI_FOCUSED);
	if (index != -1) {
		ListView_GetItemText(listviewHWND, index, 0, textOut, textOut_sz); }
	else
		textOut[0] = '\0';
	return index;
}

void JS_ListView_EnsureVisible(HWND listviewHWND, int index, bool partialOK)
{
	ListView_EnsureVisible(listviewHWND, index, partialOK);
}

int JS_ListView_EnumSelItems(HWND listviewHWND, int index)
{
	// WDL/swell doesn't offer all these flag options, so this function only offers SELECTED:
	return ListView_GetNextItem(listviewHWND, index, LVNI_SELECTED);
}

void JS_ListView_GetItem(HWND listviewHWND, int index, int subItem, char* textOut, int textOut_sz, int* stateOut)
{
	ListView_GetItemText(listviewHWND, index, subItem, textOut, textOut_sz);
	*stateOut = ListView_GetItemState(listviewHWND, index, LVIS_SELECTED | LVIS_FOCUSED);
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

int JS_ListView_GetItemState(HWND listviewHWND, int index)
{
	int state = ListView_GetItemState(listviewHWND, index, LVIS_SELECTED | LVIS_FOCUSED);
	// WIN32 and swell define LVIS_SELECTED and LVIS_FOCUSED differently, so if swell, swap values:
	#ifndef _WIN32
	if ((state & LVIS_SELECTED) && !(state & LVIS_FOCUSED))
	{
		state |= LVIS_FOCUSED;
		state &= !LVIS_SELECTED;
	}
	else if ((state & LVIS_FOCUSED) && !(state & LVIS_SELECTED))
	{
		state &= !LVIS_FOCUSED;
		state |= LVIS_SELECTED;
	}
	#endif
	return state;
}

void JS_ListView_GetItemText(HWND listviewHWND, int index, int subItem, char* textOut, int textOut_sz)
{
	ListView_GetItemText(listviewHWND, index, subItem, textOut, textOut_sz);
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

// Class that manages both a PCM_sink instance and some helper buffers

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
		if ((numframes * nch) + offset > getArraySize(data))
			return 0;
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
		/*
		For mysterious reasons, WriteDoubles wants a split audio buffer (array of pointers into mono audio buffers),
		which is the reason the helper buffer and copying data into it is needed. Pretty much everything
		else in the Reaper API seems to be using interleaved buffers for audio...
		*/
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
	delete aw; // sink creation failed, delete created instance and return null
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
	if (src == nullptr || destbuf == nullptr || numframes<1 || numchans < 1 || samplerate < 1.0)
		return 0;
	int bufsize = getArraySize(destbuf);
	if (bufsize == 0)
		return 0;
	if ((numframes*numchans) + destbufoffset > bufsize)
		return 0;
	PCM_source_transfer_t block;
	memset(&block, 0, sizeof(PCM_source_transfer_t));
	block.time_s = positioninfile; // seeking in the source is based on seconds
	block.length = numframes;
	block.nch = numchans; // the source should attempt to render as many channels as requested
	block.samplerate = samplerate; // properly implemented sources should resample to requested samplerate
	block.samples = &destbuf[destbufoffset];
	src->GetSamples(&block);
	return block.samples_out;
}

class PreviewEntry
{
public:
	PreviewEntry(int id, PCM_source* src, double gain, bool loop)
	{
		m_id = id;
		memset(&m_preg, 0, sizeof(preview_register_t));
#ifdef WIN32
		InitializeCriticalSection(&m_preg.cs);
#else
		pthread_mutexattr_t mta;
		pthread_mutexattr_init(&mta);
		pthread_mutexattr_settype(&mta, PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(&m_preg.mutex, &mta);
#endif
		MediaItem_Take* parent_take = nullptr;
		for (int i = 0; i < CountMediaItems(nullptr); ++i)
		{
			MediaItem* item = GetMediaItem(nullptr, i);
			for (int j = 0; j < CountTakes(item); ++j)
			{
				MediaItem_Take* temptake = GetMediaItemTake(item, j);
				PCM_source* tempsrc = GetMediaItemTake_Source(temptake);
				if (tempsrc == src)
				{
					parent_take = temptake;
					break;
				}
			}
			if (parent_take)
				break;
		}
		if (parent_take)
		{
			//ShowConsoleMsg("PCM_source has parent take, duplicating...\n");
			m_preg.src = src->Duplicate();
		}
		else
		{
			//ShowConsoleMsg("PCM_source has no parent take\n");
			m_preg.src = src;
		}
		m_preg.loop = loop;
		if (gain < 0.0)
			gain = 0.0;
		if (gain > 8.0)
			gain = 8.0;
		m_preg.volume = gain;
	}
	~PreviewEntry()
	{
#ifdef WIN32
		DeleteCriticalSection(&m_preg.cs);
#else
		pthread_mutex_destroy(&m_preg.mutex);
#endif
		delete m_preg.src;
	}
	void lock_mutex()
	{
#ifdef WIN32
		EnterCriticalSection(&m_preg.cs);
#else
		pthread_mutex_lock(&m_preg.mutex);
#endif
	}
	void unlock_mutex()
	{
#ifdef WIN32
		LeaveCriticalSection(&m_preg.cs);
#else
		pthread_mutex_unlock(&m_preg.mutex);
#endif
	}
	preview_register_t m_preg;
	int m_id = -1;
};

class PCMSourcePlayerManager;
PCMSourcePlayerManager* g_sourcepreviewman = nullptr;

class PCMSourcePlayerManager
{
public:
	PCMSourcePlayerManager()
	{
		// the 1000 millisecond timer is used to check for non-looping previews that have ended
		m_timer_id = SetTimer(NULL, 3000000, 1000, MyTimerproc);
	}
	~PCMSourcePlayerManager()
	{
		KillTimer(NULL, m_timer_id);
	}
	int startPreview(PCM_source* src, double gain, bool loop)
	{
		auto entry = std::make_unique<PreviewEntry>(m_preview_id_count, src, gain, loop);
		if (entry->m_preg.src)
		{
			PlayPreview(&entry->m_preg);
			m_previews.push_back(std::move(entry));
			int old_id = m_preview_id_count;
			++m_preview_id_count;
			return old_id;
		}
		return -1;
	}
	void stopPreview(int preview_id)
	{
		if (preview_id >= 0)
		{
			for (int i = 0; i < m_previews.size(); ++i)
			{
				if (m_previews[i]->m_id == preview_id)
				{
					StopPreview(&m_previews[i]->m_preg);
					m_previews.erase(m_previews.begin() + i);
					break;
				}
			}
		}
		if (preview_id == -1)
		{
			for (int i = 0; i < m_previews.size(); ++i)
			{
				StopPreview(&m_previews[i]->m_preg);
			}
			m_previews.clear();
		}
	}
	void stopPreviewsIfAtEnd()
	{
		for (int i = m_previews.size() - 1; i >= 0; --i)
		{
			m_previews[i]->lock_mutex();
			double curpos = m_previews[i]->m_preg.curpos;
			bool looping = m_previews[i]->m_preg.loop;
			m_previews[i]->unlock_mutex();
			if (looping) // the user is responsible for stopping looping previews!
				continue;
			if (curpos >= m_previews[i]->m_preg.src->GetLength() - 0.01)
			{
				//char buf[100];
				//sprintf(buf, "Stopping preview %d\n", m_previews[i]->m_id);
				//ShowConsoleMsg(buf);
				StopPreview(&m_previews[i]->m_preg);
				m_previews.erase(m_previews.begin() + i);
			}

		}
	}
private:
	// for Windows 32 bit, this may need a calling convention qualifier...?
	static void CALLBACK MyTimerproc(
		HWND Arg1,
		UINT Arg2,
		UINT_PTR Arg3,
		DWORD Arg4
	)
	{
		if (g_sourcepreviewman)
			g_sourcepreviewman->stopPreviewsIfAtEnd();
	}
	std::vector<std::unique_ptr<PreviewEntry>> m_previews;
	int m_preview_id_count = 0;
	UINT_PTR m_timer_id = 0;
};



int Xen_StartSourcePreview(PCM_source* src, double gain, bool loop)
{
	if (g_sourcepreviewman == nullptr)
		g_sourcepreviewman = new PCMSourcePlayerManager;
	return (int)g_sourcepreviewman->startPreview(src, gain, loop);
}

int Xen_StopSourcePreview(int preview_id)
{
	if (g_sourcepreviewman != nullptr)
		g_sourcepreviewman->stopPreview(preview_id);
	return 0;
}

void Xen_DestroyPreviewSystem()
{
	delete g_sourcepreviewman;
	g_sourcepreviewman = nullptr;
}

////////////////////////////////////////////////////////////////
