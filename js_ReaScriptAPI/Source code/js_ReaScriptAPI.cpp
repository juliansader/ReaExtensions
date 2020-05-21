#include "stdafx.h" // This will in turn #include all the other header files

using namespace std;

#define JS_REASCRIPTAPI_VERSION 1.215

#ifndef _WIN32
#define _WDL_SWELL 1 // So that I don't have to type #ifdef __linux__ and __APPLE__ everywhere
#define IsWindow(X) ValidatePtr(X, "HWND")
#define jsAlphaBlend(A, B, C, D, E, F, G, H, I, J) StretchBlt(A, B, C, D, E, F, G, H, I, J, SRCCOPY_USEALPHACHAN)
#else
#define jsAlphaBlend(A, B, C, D, E, F, G, H, I, J) AlphaBlend(A, B, C, D, E, F, G, H, I, J, BLENDFUNCTION{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA })
#endif

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
		
		Julian::compositeCanvas = LICE_CreateBitmap(true, 2000, 2000);
		if (!Julian::compositeCanvas)
		{
			MessageBox(NULL, "Could not create a LICE bitmap for compositing.", "ERROR: js_ReaScriptAPI extension", 0);
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
		
		// List js_ReaScriptAPI in REAPER main toolbar menu
		//plugin_register("hookcustommenu", menuHook);

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

	// rec == nil
	// Does an extension need to do anything when unloading?  
	// To prevent memory leaks, perhaps try to delete any stuff that may remain in memory?
	// On Windows, LICE bitmaps are automatically destroyed when REAPER quits, but to make extra sure, this function will destroy them explicitly.
	else 
	{
		// WARNING!!!  Iterator may crash if map entries is deleted while iterating,
		// which JS_WindowMessage_RestoreOrigProcAndErase indeed does.
		// So leave JS_WindowMessage_RestoreOrigProcAndErase out of loop.
		for (auto& i : Julian::mapWindowData) {
			i.second.mapBitmaps.clear();
			i.second.mapMessages.clear();
		}
		JS_WindowMessage_RestoreOrigProcAndErase();
			
		// Strangely, REAPER seems to execute script atexits AFTER running this REAPER_PLUGIN_ENTRYPOINT with rec = 0.
		// So if the extension destroys all bitmaps but doesn't clear mLICEBitmaps, and a script also tries to destroy
		//		a bitmap, the JS_LICE_DestroyBitmap may think that the bitmap still exists, and then crash when
		//		attempting to destroy a non-existing bitmap.  
		// So mLICEBitmaps and all other resource containers must be cleared if scripts depend on them to be accurate.
		//std::set<LICE_I
		for (auto& i : Julian::mLICEBitmaps)
			LICE__Destroy(i.first);
		Julian::mLICEBitmaps.clear();
		for (auto& i : Julian::mapClassNames)
			free(i.second);
		for (auto& i : Julian::mapMallocToSize)
			free(i.first);
		for (APIdef& i : ::aAPIdefs)
			free(i.defstring);
		for (HGDIOBJ i : Julian::setGDIObjects)
			DeleteObject(i);
		Julian::setGDIObjects.clear();

		LICE__Destroy(Julian::compositeCanvas);

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
v0.991
 * Improved: Xen_StartSourcePreview: Add support for setting hardware output channels for PCM_source previews
 * Improved: JS_Window_SetZOrder and JS_Window_SetPos.
 * New: JS_Window_SetStyle: Can add or remove frames from gfx and other windows.
v0.992
 * Support Unicode characters for Dialog functions, Get/SetTitle/ClassName, and GDI_DrawText.
 * VKeys functions ignore auto-repeated KEYDOWN messages.
v0.993
 * VKeys functions: improved handling of auto-repeated KEYDOWN messages.
v0.995
 * Fixed: Script-created windows crashing when subclassing.
v0.996
 * JS_Window_SetParent
v0.997
 * WindowMessage_Post and _Send work similarly when message type is being intercepted.
v0.998
 * Proper linking against GDK on Linux.
v0.999
 * JS_Window functions: On Linux and macOS, don't crash if handle is invalid.
 * LoadPNG, SavePNG, LoadCursorFromFile: On Windows, accept Unicode paths.
 * ReleaseDC: Can release screen HDCs.
v1.000f
 * Composite, Composite_Unlink and DestroyBitmap automatically updates window.
 * Composite_Unlink can optionally unlink all bitmaps from window.
 * JS_LICE_List/ArrayAllBitmaps
 * JS_Window_EnableMetal.
v1.001
 * Safely delete resources if REAPER quits while script is running.
v1.002
 * Linux and macOS: JS_Window_GetForeground returns top-level window.
 * WindowsOS: New JS_Composite_Delay function to reduce flickering.
 * JS_Composite: Auto-update now optional.
 * JS_Composite: Fixed incorrect InvalidateRect calculation. 
v1.010
 * Streamline Compositing functions, particularly when window is only partially invalidated.
 * Fix bug in JS_WindowMessage_RestoreOrigProcAndErase.
v1.210
 * macOS: JS_Window_EnableMetal returns correct modes.
 * macOS: Compositing still doesn't work if Metal graphics is enabled.
v1.215
 * Fixed: LICE_WritePNG when image has transparency.
 * New: LICE_LoadJPG, LICE_WriteJPG.
 * Updated: If Metal graphics, JS_Composite clips to client area.
*/



int JS_Zip_Add(char* zipFile, char* inputFiles, int inputFiles_sz)
{/*
	using namespace zipper;
	
	Zipper zipper(zipFile);
	if (!zipper) return 0;
	
	while (inputFiles && *inputFiles)
	{
		#ifdef _WIN32 // convert to WideChar for Unicode support
		int wideCharLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, inputFiles, -1, NULL, 0);
		if (!wideCharLength) return -2
		WCHAR* widePath = (WCHAR*) alloca(wideCharLength * sizeof(WCHAR) * 2);
		if (!widePath) return -2
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, inputFiles, -1, widePath, wideCharLength * 2);
		std::ifstream file{widePath};
		#else
		std::ifstream file{inputFiles};
		#endif
		if (!file) return -1;
		zipper.add(input1, inputFiles);
		inputFiles = strchr(inputFiles, 0)
		if (inputFiles) inputFiles += 1;
	}

	zipper.close();
	*/
	return 1;
}

void JS_ReaScriptAPI_Version(double* versionOut)
{
	*versionOut = JS_REASCRIPTAPI_VERSION;
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
static double VK_KeyDown[256]{ 0 }; // time stamp of latest WM_KEYDOWN message -- INcluding all auto-repeated KEYDOWNs
static double VK_KeyDownEx[256]{ 0 }; // time stamp of latest WM_KEYDOWN message -- EXcluding auto-repeated KEYDOWNs
static double VK_KeyUp[256]{ 0 }; // time stamp of latest WM_KEYUP message
static unsigned char VK_Intercepts[256]{ 0 }; // Should the VK be intercepted?
static constexpr size_t VK_Size = 255; // sizeof(VK_Intercepts);

int JS_VKeys_Callback(MSG* event, accelerator_register_t*)
{
	const WPARAM& keycode = event->wParam;
	const UINT& uMsg = event->message;
	double time;

	switch (uMsg)
	{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (keycode < 256)
			{
				time = time_precise();
				if ((time > VK_KeyDown[keycode] + 4) || (VK_KeyUp[keycode] >= VK_KeyDown[keycode])) // Ignore repeated keys. Assume keyboard repeat delay is always less than 4s.
					VK_KeyDownEx[keycode] = time;
				VK_KeyDown[keycode] = time;
			}
			break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (keycode < 256)
				VK_KeyUp[keycode] = time_precise();
			break;
	}

	if ((keycode < 256) && VK_Intercepts[keycode] && (uMsg != WM_KEYUP) && (uMsg != WM_SYSKEYUP)) // Block keystroke (including WM_CHAR), but not when releasing key
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
				if (VK_KeyDownEx[i] > VK_KeyUp[i] && VK_KeyDownEx[i] > cutoffTime)
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
				if (VK_KeyDownEx[i] > cutoffTime)
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
	gotFile = GetSaveFileName(&info); // On Windows, this is DEFINE'd as GetSaveFileNameUTF8 in win32_tuf8.h , which enables Windows' WCHAR and UNICODE.
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
		// On Windows, this is DEFINE'd as GetSaveFileNameUTF8 in win32_tuf8.h , which enables Windows' WCHAR and UNICODE.
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
#ifdef _WIN32
	int retval = FALSE;
	// These Windows file dialogs do not understand /, so v0.970 added this quick hack to replace / with \.
	size_t folderLen = strlen(initialFolder) + 1; // Include terminating \0.
	char* newInitFolder = (char*)malloc(folderLen);
	if (newInitFolder)
	{
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

		PIDLIST_ABSOLUTE folderID = SHBrowseForFolderUTF8(&info); // Unlike GetWindowText etc, SHBrowseForFolder isn't redefined to call SHBrowseForFolderUTF8 in win32_utf8.h
		if (folderID)
		{
			wchar_t* folderOutW = (wchar_t*)malloc(folderOutNeedBig_sz * sizeof(wchar_t));  // 64000 may look bizarre, but UNICODE paths may be more than 32767 characters in length!
			if (folderOutW)
			{
				if (SHGetPathFromIDListW(folderID, folderOutW))
				{
					int s = WideCharToMultiByte(CP_UTF8, 0, folderOutW, -1, NULL, 0, NULL, NULL);
					if (s && (s < folderOutNeedBig_sz))
					{
						WideCharToMultiByte(CP_UTF8, 0, folderOutW, -1, folderOutNeedBig, folderOutNeedBig_sz, NULL, NULL);
						retval = TRUE;
					}
				}
				free(folderOutW);
			}
			ILFree(folderID);
		}
		free(newInitFolder);
	}
	return retval;

#else
	// returns TRUE if path was chosen.
	return BrowseForDirectory(caption, initialFolder, folderOutNeedBig, folderOutNeedBig_sz);
#endif
}


//////////////////////////////////////////////////////////////////////////////////////////////////

bool JS_Window_GetRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut)
{
	#ifdef _WDL_SWELL
	if(!ValidatePtr(windowHWND, "HWND")) return false;
	#endif
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
	POINT p{ x, y };
	#ifdef _WDL_SWELL
	if(ValidatePtr(windowHWND, "HWND"))
	#endif
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	ScreenToClient((HWND)windowHWND, &p);
	*xOut = (int)p.x;
	*yOut = (int)p.y;
}

void JS_Window_ClientToScreen(void* windowHWND, int x, int y, int* xOut, int* yOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	POINT p{ x, y };
	#ifdef _WDL_SWELL
	if(ValidatePtr(windowHWND, "HWND"))
	#endif
	ClientToScreen((HWND)windowHWND, &p);
	*xOut = (int)p.x;
	*yOut = (int)p.y;
}


bool JS_Window_GetClientRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut)
{
	// Unlike Win32, Cockos WDL doesn't return a bool to confirm success.
	// However, if hwnd is not a true hwnd, SWELL will return a {0,0,0,0} rect.
	RECT r{ 0, 0, 0, 0 };
#ifdef _WIN32
	bool isOK = !!GetClientRect((HWND)windowHWND, &r);
#else
	if(ValidatePtr(windowHWND, "HWND")) 
		GetClientRect((HWND)windowHWND, &r);
	bool isOK = (r.bottom != 0 || r.right != 0);
#endif
	if (isOK) {
		POINT p{ 0, 0 };
		ClientToScreen((HWND)windowHWND, &p);
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
	if(ValidatePtr(windowHWND, "HWND")) 
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
	#ifdef _WDL_SWELL
	if(!ValidatePtr(windowHWND, "HWND")) return nullptr;
	#endif
	return GetParent((HWND)windowHWND);
}
	
void* JS_Window_SetParent(void* childHWND, void* parentHWND)
{
	#ifdef _WDL_SWELL
	if(!(ValidatePtr(childHWND, "HWND") && ValidatePtr(parentHWND, "HWND"))) return nullptr;
	#endif
	return SetParent((HWND)childHWND, (HWND)parentHWND);
}

bool  JS_Window_IsChild(void* parentHWND, void* childHWND)
{
	#ifdef _WDL_SWELL
	if(!(ValidatePtr(childHWND, "HWND") && ValidatePtr(parentHWND, "HWND"))) return false;
	#endif
	return !!IsChild((HWND)parentHWND, (HWND)childHWND);
}

void* JS_Window_GetRelated(void* windowHWND, const char* relation)
{
	#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) return nullptr;
	#endif
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
	#ifdef _WDL_SWELL
	if (ValidatePtr(windowHWND, "HWND"))
	#endif
	// SWELL returns different types than Win32, so this function won't return anything.
	SetFocus((HWND)windowHWND);
}

void  JS_Window_SetForeground(void* windowHWND)
{
	#ifdef _WDL_SWELL
	if(ValidatePtr(windowHWND, "HWND"))
	#endif
	// SWELL returns different types than Win32, so this function won't return anything.
	SetForegroundWindow((HWND)windowHWND);
}

void* JS_Window_GetFocus()
{
	return GetFocus();
}

// WDL/swell often simply returns the focused window, instead of the toplevel foreground window.
// Some this extension adapted the WDL/swell function.
void* JS_Window_GetForeground()
{
#ifdef _WIN32
	return GetForegroundWindow();
#elif __linux__
	// Definitions of swell's HWND, HWND__ struct, m_oswindow etc can be found in swell-internal.h
	HWND w = GetForegroundWindow();
	while (w && (w->m_parent))
		w = w->m_parent;
	return w;
#else
	return JS_GetContentViewFromSwellHWND(GetForegroundWindow());
#endif
}

	
int JS_Window_EnableMetal(void* windowHWND)
{
#ifdef __APPLE__
	if (ValidatePtr(windowHWND, "HWND"))
		return SWELL_EnableMetal((HWND)windowHWND, 0);
	else
		return 0;
#else
	return 0;
#endif
}
	
void  JS_Window_Enable(void* windowHWND, bool enable)
{
	#ifdef _WDL_SWELL
	if (ValidatePtr(windowHWND, "HWND"))
	#endif
	EnableWindow((HWND)windowHWND, (BOOL)enable); // (enable ? (int)1 : (int)0));
}

void  JS_Window_Destroy(void* windowHWND)
{
	#ifdef _WDL_SWELL
	if(ValidatePtr(windowHWND, "HWND"))
	#endif
	DestroyWindow((HWND)windowHWND);
}

void  JS_Window_Show(void* windowHWND, const char* state)
{
	/* from swell-types.h:
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
	#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) return false;
	#endif
	return !!IsWindowVisible((HWND)windowHWND);
}



void* JS_Window_SetCapture(void* windowHWND)
{
	#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) return nullptr;
	#endif
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
	#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) return nullptr;
	#endif
	
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
	#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) *retvalOut = 0;
	#endif
	
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
	#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) *retvalOut = 0;
	#endif
	
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
	#ifdef _WDL_SWELL
	if (!(ValidatePtr(parentHWND, "HWND") && ValidatePtr(childHWND, "HWND"))) return nullptr;
	#endif
	// REAPER API cannot pass null pointers, so must do another way:
	HWND		c = ((parentHWND == childHWND) ? nullptr : childHWND);
	const char* t = ((strlen(title) == 0) ? nullptr : title);
	return FindWindowEx(parentHWND, c, className, t);
}


HWND JS_Window_FindChildByID(HWND parentHWND, int ID)
{
	#ifdef _WDL_SWELL
	if (!ValidatePtr(parentHWND, "HWND")) return nullptr;
	#endif
	return GetDlgItem(parentHWND, ID);
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
	#ifdef _WDL_SWELL
	if(!(ValidatePtr(parentHWND, "HWND"))) return nullptr;
	#endif
	
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
	#ifdef _WDL_SWELL
	if(!(ValidatePtr(parentHWND, "HWND"))) return 0;
	#endif
	// Enumerate through all child windows and store their HWNDs in a set. (Sets are nice, since will automatically check for uniqueness.)
	std::set<HWND> foundHWNDs;
	EnumChildWindows((HWND)parentHWND, JS_Window_ListAllChild_Callback, reinterpret_cast<LPARAM>(&foundHWNDs));

	return ConvertSetHWNDToString(foundHWNDs, listOutNeedBig, listOutNeedBig_sz);
}

int JS_Window_ArrayAllChild(void* parentHWND, double* reaperarray)
{
	#ifdef _WDL_SWELL
	if (!(ValidatePtr(parentHWND, "HWND"))) return 0;
	#endif
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
#define myDefProc DefWindowProc
#else
typedef INT_PTR callbacktype;
#define myDefProc DefWindowProc
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

#ifdef _WDL_SWELL
#define WS_SIZEBOX WS_THICKFRAME
#define WS_OVERLAPPEDWINDOW (WS_SIZEBOX|WS_SYSMENU|WS_CAPTION)
#define WS_OVERLAPPED WS_CAPTION
#define WS_MAXIMIZEBOX (WS_CAPTION|WS_SIZEBOX)
#define WS_MINIMIZEBOX WS_CAPTION
#undef WS_BORDER
#define WS_BORDER WS_CAPTION
#define WS_DLGFRAME WS_CAPTION
#define WS_CLIPCHILDREN 0
#define WS_POPUP 0
#define WS_MINIMIZE 0x20000000
#define WS_MAXIMIZE 0x01000000
#endif

DWORD JS_ConvertStringToStyle(char* styleString)
{
	//*show = SW_SHOW; // Default values if styleOptional not specified.
	DWORD style = WS_OVERLAPPEDWINDOW;

	if (styleString && *styleString) {
		style = 0;
		// To distinguish MAXIMIZEBOX from MAXIMIZE, remove the M's of all MAXIMIZEBOXes.
		// swell doesn't implement WS_SHOWMAXIMIZED and WS_SHOWMINIMIZED, so will use ShowWindow's options instead.
		char* box;
		while (box = strstr(styleString, "MAXIMIZEBOX"))	{ style |= (WS_MAXIMIZEBOX | WS_SYSMENU); *box = 'N'; }
		while (box = strstr(styleString, "MINIMIZEBOX"))	{ style |= (WS_MINIMIZEBOX | WS_SYSMENU); *box = 'N'; }
		if (strstr(styleString, "MAXIMIZE"))		style |= WS_MAXIMIZE;
		if (strstr(styleString, "CHILD"))			style |= WS_CHILD;
		//if (strstr(styleString, "CHILDWINDOW"))	style |= WS_CHILDWINDOW;
		if (strstr(styleString, "CLIPSIBLINGS"))	style |= WS_CLIPSIBLINGS;
		if (strstr(styleString, "DISABLED"))		style |= WS_DISABLED;
		if (strstr(styleString, "VISIBLE"))			style |= WS_VISIBLE;
		if (strstr(styleString, "CAPTION"))			style |= WS_CAPTION;
		if (strstr(styleString, "VSCROLL"))			style |= WS_VSCROLL;
		if (strstr(styleString, "HSCROLL"))			style |= WS_HSCROLL;
		if (strstr(styleString, "SYSMENU"))			style |= WS_SYSMENU;
		if (strstr(styleString, "GROUP"))			style |= WS_GROUP;
		if (strstr(styleString, "TABSTOP"))			style |= WS_TABSTOP;
		if (strstr(styleString, "DLGFRAME"))		style |= WS_DLGFRAME;
		if (strstr(styleString, "BORDER"))			style |= WS_BORDER;
		if (strstr(styleString, "CLIPCHILDREN"))	style |= WS_CLIPCHILDREN;
		if (strstr(styleString, "MINIMIZE")		|| strstr(styleString, "ICONIC"))			style |= WS_MINIMIZE;
		if (strstr(styleString, "THICKFRAME")	|| strstr(styleString, "SIZEBOX"))			style |= WS_SIZEBOX;
		if (strstr(styleString, "OVERLAPPED")	|| strstr(styleString, "TILED"))			style |= WS_OVERLAPPED;
		if (strstr(styleString, "TILEDWINDOW")	|| strstr(styleString, "OVERLAPPEDWINDOW"))	style |= WS_OVERLAPPEDWINDOW;
		if (strstr(styleString, "POPUP"))			{ style &= (~(WS_CAPTION | WS_CHILD));	style |= WS_POPUP; } // swell doesn't actually implement WS_POPUP as separate style
	}
	return style;
}

void* JS_Window_Create(const char* title, const char* className, int x, int y, int w, int h, char* styleOptional, void* ownerHWNDOptional)
{
	using namespace Julian;
	HWND hwnd = nullptr; // Default return value if everything doesn't go OK.

	if ((ownerHWNDOptional == nullptr) || ValidatePtr((HWND)ownerHWNDOptional, "HWND")) // NULL owner is allowed, but not an invalid one
	{
		DWORD style = JS_ConvertStringToStyle(styleOptional);

		// On Windows, each new class name requires a new class.
#ifdef _WIN32
		// Does the class already exist?  If not, create it and save class name.
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
				mapClassNames[classString] = strdup(className); // Class names must be saved, so use strdup (and remember to free memory later)
		}

		// OK, class exists, so can go ahead to create window
		if (mapClassNames.count(classString))
		{
			hwnd = CreateWindowEx(
				WS_EX_LEFT | WS_EX_ACCEPTFILES | WS_EX_APPWINDOW | WS_EX_CONTEXTHELP,	//DWORD     dwExStyle,
				mapClassNames[classString], 	//LPCSTR    lpClassName,
				"", 	//LPCSTR    lpWindowName, // SetWindowText will be used below to set title with (potentially) Unicode text.
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
				SetWindowTextUTF8(hwnd, title); // Since non-widechar function were used to create the window, title must be set separately with potentially Unicode text.
				//SetWindowPos(hwnd, HWND_TOP, x, y, w, h, SWP_NOMOVE | SWP_NOSIZE);
				if (style&WS_MINIMIZE)		ShowWindow(hwnd, SW_SHOWMINIMIZED);
				else if (style&WS_MAXIMIZE) ShowWindow(hwnd, SW_SHOWMAXIMIZED);
				else						ShowWindow(hwnd, SW_SHOWNORMAL);
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
			if (style&WS_MINIMIZE)	ShowWindow(hwnd, SW_SHOWMINIMIZED); //swell doesn't implement WS_MAXIMIZED and WS_VISIBLE.  The latter is simply 0 in swell.
			else					ShowWindow(hwnd, SW_SHOW);
			UpdateWindow(hwnd);
		}
#endif
	}
	return hwnd;
}

/////////////////////////////////////////////////////////////////
// Function based on SetWindowPos

#define SETWINDOWPOS_INVALID_INSERTAFTER (HWND)(-3) // Some value that should not be one of the valid insertAfter values.
bool JS_Window_SetPosition(void* windowHWND, int left, int top, int width, int height, char* ZOrderOptional, char* flagsOptional)
{
	if (ValidatePtr(windowHWND, "HWND"))
	{
		// For compatibility with older versions, when ZOrder and flags are not supplied, simply reposition.
		if (!(ZOrderOptional && *ZOrderOptional) && !(flagsOptional && *flagsOptional))
		{
			SetWindowPos((HWND)windowHWND, NULL, left, top, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER);
			return true;
		}

		// If ZOrder or flags are given, do full-scale SetWindowPos
		else
		{
			// Convert flags string to integer
			UINT intFlags = 0;
			if (flagsOptional)
			{
				if (strstr(flagsOptional, "NOMOVE"))			intFlags = intFlags | SWP_NOMOVE;
				if (strstr(flagsOptional, "NOSIZE"))			intFlags = intFlags | SWP_NOSIZE;
				if (strstr(flagsOptional, "NOZORDER"))			intFlags = intFlags | SWP_NOZORDER;
				if (strstr(flagsOptional, "NOACTIVATE"))		intFlags = intFlags | SWP_NOACTIVATE;
				if (strstr(flagsOptional, "SHOWWINDOW"))		intFlags = intFlags | SWP_SHOWWINDOW;
				if (strstr(flagsOptional, "FRAMECHANGED"))		intFlags = intFlags | SWP_FRAMECHANGED;
				if (strstr(flagsOptional, "NOCOPYBITS"))		intFlags = intFlags | SWP_NOCOPYBITS;
#ifdef _WIN32
				if (strstr(flagsOptional, "ASYNCWINDOWPOS"))	intFlags = intFlags | SWP_ASYNCWINDOWPOS;
				if (strstr(flagsOptional, "DEFERERASE"))		intFlags = intFlags | SWP_DEFERERASE;
				if (strstr(flagsOptional, "DRAWFRAME"))			intFlags = intFlags | SWP_DRAWFRAME;
				if (strstr(flagsOptional, "HIDEWINDOW"))		intFlags = intFlags | SWP_HIDEWINDOW;
				if (strstr(flagsOptional, "NOOWNERZORDER"))		intFlags = intFlags | SWP_NOOWNERZORDER;
				if (strstr(flagsOptional, "NOREDRAW"))			intFlags = intFlags | SWP_NOREDRAW;
				if (strstr(flagsOptional, "NOREPOSITION"))		intFlags = intFlags | SWP_NOREPOSITION;
				if (strstr(flagsOptional, "NOSENDCHANGING"))	intFlags = intFlags | SWP_NOSENDCHANGING;
#endif
			}

			// Convert insertAfter string to HWND
			HWND insertAfterHWND = SETWINDOWPOS_INVALID_INSERTAFTER;
			if (ZOrderOptional && !(intFlags&SWP_NOZORDER)) // ZOrder only relevant if NOZORDER flag is not set
			{
				if (strstr(ZOrderOptional, "BOTTOM"))		insertAfterHWND = HWND_BOTTOM;
				else if (strstr(ZOrderOptional, "NOTOPMOST"))	insertAfterHWND = HWND_NOTOPMOST;
				else if (strstr(ZOrderOptional, "TOPMOST"))		insertAfterHWND = HWND_TOPMOST;
				else if (strstr(ZOrderOptional, "TOP"))			insertAfterHWND = HWND_TOP;
				else
				{ // Check if tostring(hwnd) was used, which should give "userdata: 0000somenumber"
					const char* p = strrchr(ZOrderOptional, ' ');
					if (!p) p = ZOrderOptional;
					HWND h = (HWND)(void*)strtoll(p, NULL, 16);
					if (ValidatePtr(h, "HWND"))
						insertAfterHWND = h;
				}
			}

			// If ZOrder, only continue if insertAfter has been set
			if ((intFlags&SWP_NOZORDER) || (insertAfterHWND != SETWINDOWPOS_INVALID_INSERTAFTER))
			{
#ifdef _WIN32
				return SetWindowPos((HWND)windowHWND, insertAfterHWND, left, top, width, height, intFlags);
#elif __APPLE__
				SetWindowPos((HWND)windowHWND, insertAfterHWND, left, top, width, height, intFlags);
				if (!(intFlags&SWP_NOZORDER))
					return JS_Window_SetZOrder_ObjC(windowHWND, insertAfterHWND);
				else
					return true;
#elif __linux__
				SetWindowPos((HWND)windowHWND, insertAfterHWND, left, top, width, height, intFlags);
				return true;
#endif
			}
		}
	}
	return false;
}

/*
bool JS_Window_SetPos(void* windowHWND, const char* ZOrder, int x, int y, int w, int h, const char* flags)
{
	if (ValidatePtr(windowHWND, "HWND"))
	{
		// Convert flags string to integer
		UINT intFlags = 0;
		if (strstr(flags, "NOMOVE"))			intFlags = intFlags | SWP_NOMOVE;
		if (strstr(flags, "NOSIZE"))			intFlags = intFlags | SWP_NOSIZE;
		if (strstr(flags, "NOZORDER"))			intFlags = intFlags | SWP_NOZORDER;
		if (strstr(flags, "NOACTIVATE"))		intFlags = intFlags | SWP_NOACTIVATE;
		if (strstr(flags, "SHOWWINDOW"))		intFlags = intFlags | SWP_SHOWWINDOW;
		if (strstr(flags, "FRAMECHANGED"))		intFlags = intFlags | SWP_FRAMECHANGED;
		if (strstr(flags, "NOCOPYBITS"))		intFlags = intFlags | SWP_NOCOPYBITS;
#ifdef _WIN32
		if (strstr(flags, "ASYNCWINDOWPOS"))	intFlags = intFlags | SWP_ASYNCWINDOWPOS;
		if (strstr(flags, "DEFERERASE"))		intFlags = intFlags | SWP_DEFERERASE;
		if (strstr(flags, "DRAWFRAME"))			intFlags = intFlags | SWP_DRAWFRAME;
		if (strstr(flags, "HIDEWINDOW"))		intFlags = intFlags | SWP_HIDEWINDOW;
		if (strstr(flags, "NOOWNERZORDER"))		intFlags = intFlags | SWP_NOOWNERZORDER;
		if (strstr(flags, "NOREDRAW"))			intFlags = intFlags | SWP_NOREDRAW;
		if (strstr(flags, "NOREPOSITION"))		intFlags = intFlags | SWP_NOREPOSITION;
		if (strstr(flags, "NOSENDCHANGING"))	intFlags = intFlags | SWP_NOSENDCHANGING;
#endif

		// Convert insertAfter string to HWND
		HWND insertAfterHWND = SETWINDOWPOS_INVALID_INSERTAFTER;
		if (!(intFlags&SWP_NOZORDER))
		{
			if (strstr(ZOrder, "BOTTOM"))		insertAfterHWND = HWND_BOTTOM;
			else if (strstr(ZOrder, "NOTOPMOST"))		insertAfterHWND = HWND_NOTOPMOST;
			else if (strstr(ZOrder, "TOPMOST"))	insertAfterHWND = HWND_TOPMOST;
			else if (strstr(ZOrder, "TOP"))		insertAfterHWND = HWND_TOP;
			else
			{ // Check if tostring(hwnd) was used, which should give "userdata: 0000somenumber"
				const char* p = strrchr(ZOrder, ' ');
				if (!p) p = ZOrder;
				HWND h = (HWND)(void*)strtoll(p, NULL, 16);
				if (ValidatePtr(h, "HWND"))
					insertAfterHWND = h;
			}
		}

		// If ZOrder, only continue if insertAfter has been set
		if ((intFlags&SWP_NOZORDER) || (insertAfterHWND != SETWINDOWPOS_INVALID_INSERTAFTER))
		{
#ifdef _WIN32
			return SetWindowPos((HWND)windowHWND, insertAfterHWND, x, y, w, h, intFlags);
#elif __APPLE__
			SetWindowPos((HWND)windowHWND, insertAfterHWND, x, y, w, h, intFlags);
			if (!(intFlags&SWP_NOZORDER))
				return JS_Window_SetZOrder_ObjC(windowHWND, insertAfterHWND);
			else
				return true;
#elif __linux__
			return true;
#endif
		}
	}
	return false;
}
*/

// This function moves windows without resizing or requiring any info about window size.
void JS_Window_Move(void* windowHWND, int left, int top)
{
#ifdef _WDL_SWELL
	if (ValidatePtr(windowHWND, "HWND"))
#endif
	SetWindowPos((HWND)windowHWND, NULL, left, top, 0, 0, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOSIZE);
}

// This function resizes windows without moving or requiring any info about window position.
void JS_Window_Resize(void* windowHWND, int width, int height)
{
#ifdef _WDL_SWELL
	if (ValidatePtr(windowHWND, "HWND"))
#endif
	SetWindowPos((HWND)windowHWND, NULL, 0, 0, width, height, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOMOVE);
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
	if (ValidatePtr(windowHWND, "HWND"))
	{
		HWND insertAfter = SETWINDOWPOS_INVALID_INSERTAFTER;
		if (strstr(ZOrder, "BOTTOM"))		insertAfter = HWND_BOTTOM;
		else if (strstr(ZOrder, "NOTOP"))	insertAfter = HWND_NOTOPMOST;
		else if (strstr(ZOrder, "TOPMOST"))	insertAfter = HWND_TOPMOST;
		else if (strstr(ZOrder, "TOP"))		insertAfter = HWND_TOP; // Top
#ifndef __linux__ // swell doesn't provide all options
		else if (strstr(ZOrder, "INSERT") || strstr(ZOrder, "AFTER"))
		{
			if (insertAfterHWNDOptional && ValidatePtr(insertAfterHWNDOptional, "HWND"))
				insertAfter = (HWND)insertAfterHWNDOptional;
		}
#endif
		else
		{ // Check if tostring(hwnd) was used, which should give "userdata: 0000somenumber"
			const char* p = strrchr(ZOrder, ' ');
			if (!p) p = ZOrder;
			HWND h = (HWND)(void*)strtoll(p, NULL, 16);
			if (ValidatePtr(h, "HWND"))
				insertAfter = h;
			/*
			for (const char* p = ZOrder; *p != 0; p++)
			{
				if (isdigit(*p))
				{
					HWND h = (HWND)(intptr_t)strtoll(p, NULL, 16);
					if (ValidatePtr(h, "HWND"))
						insertAfter = h;
					break;
				}
			}*/
		}

		if (insertAfter != SETWINDOWPOS_INVALID_INSERTAFTER) { // Was given a proper new value?
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

bool JS_Window_SetStyle(void* windowHWND, char* style)
{
	if (JS_Window_IsWindow(windowHWND))
	{
		// Try to get the toplevel window, which actually has a style to display
		// Except on macOS.  Apparently, macOS toplevel NSWindows, don't work with these styles?
	#ifdef __APPLE__
		HWND rootHWND = (HWND)windowHWND;
	#else
		HWND rootHWND = (HWND)JS_Window_GetRoot(windowHWND);
		if (!ValidatePtr(rootHWND, "HWND")) rootHWND = (HWND)windowHWND;
	#endif
		
		DWORD styleNumber = JS_ConvertStringToStyle(style);
	#ifdef _WIN32
		SetWindowLongPtr(rootHWND, GWL_STYLE, styleNumber);
	#elif __linux__
		ShowWindow(rootHWND, SW_HIDE); // On Linux (at least, on my distribution), must first hide
		SetWindowLong(rootHWND, GWL_STYLE, styleNumber);
	#else
		SetWindowLong(rootHWND, GWL_STYLE, styleNumber); 
	#endif
		//According to stuff in the Web, SetWindowPos with FRAMECHANGED is necessary and sufficient to apply new frame style. Doesn't seem to work for me. Use ShowWindow instead.
		//SetWindowPos(rootHWND, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
		if (styleNumber&WS_MINIMIZE)		ShowWindow(rootHWND, SW_SHOWMINIMIZED);
		else if (styleNumber&WS_MAXIMIZE)	ShowWindow(rootHWND, SW_SHOWMAXIMIZED);
		else					ShowWindow(rootHWND, SW_SHOW);
		return true;
	}
	return false;
}

void JS_Window_Update(HWND windowHWND)
{
#ifdef _WDL_SWELL
	if (ValidatePtr(windowHWND, "HWND"))
#endif
	UpdateWindow(windowHWND);
}

//!!!! This function does not work yet, since the NSWindow* returned on macOS isn't really the same type of window as the NSViews
//!!!! that most functions seem to work with.  NSWindow is more of a container.
void* JS_Window_GetRoot(void* windowHWND)
{
#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) return nullptr;
#endif

#ifdef _WIN32
	return GetAncestor((HWND)windowHWND, GA_ROOT);
#elif __linux__
	// Definitions of swell's HWND, m_oswindow etc can be found in swell-internal.h
	GetNextAncestorWindow:
	if (((HWND)windowHWND)->m_oswindow) // Does this HWND correspond to a GDKWindow?
	{
		return gdk_window_get_effective_toplevel((GdkWindow*)(((HWND)windowHWND)->m_oswindow));
	}
	else if (((HWND)windowHWND)->m_parent) // Else, try to go high in hierarchy, until oswindow is found
	{
		windowHWND = ((HWND)windowHWND)->m_parent;
		goto GetNextAncestorWindow;
	}
	else 
		return nullptr;
#else
	return JS_GetNSWindowFromSwellHWND(windowHWND);
#endif
}

bool JS_Window_InvalidateRect(HWND windowHWND, int left, int top, int right, int bottom, bool eraseBackground)
{
	using namespace Julian;
	
	if (left >= right || top >= bottom)
		return false;
	#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) 
		return false;
	#endif
		
	RECT rect{ left, top, right, bottom };
	
	// If window is composited, details of Invalidated Rects are stored -- and always erase background.
	if (mapWindowData.count(windowHWND))
	{
		RECT& iR = mapWindowData[windowHWND].doneInvalidRect;
		
		// If not yet invalidated in this paint cycle, InvalidateRect
		if (iR.left == iR.right || iR.top == iR.bottom)
		{
			iR = rect;
			return InvalidateRect(windowHWND, &iR, true);
		}
	#ifdef _WIN32
		// WindowsOS: If window is already invalidated, store rect in to-be-invalidated i2R, to be invalidated during WM_PAINT callback 
		//		(This avoids dozens of overlapping Invalidates in case of scripts with dozens of composited bitmaps, such as Area(51).)
		else
		{
			RECT& i2R = mapWindowData[windowHWND].mustInvalidRect;
			if (RECT_IS_EMPTY(i2R))
				i2R = rect;
			else
				UNION_RECT(i2R, rect);
			return true;
		}
	#else
		// Linux/macOS: InvalidateRect cannot be called inside WM_PAINT callback, so must invalidate now.  But only necessary if rect expands existing iR.
		else if (left < iR.left || top < iR.top || right > iR.right || bottom > iR.bottom)
		{
			UNION_RECT(iR, rect);
			return InvalidateRect(windowHWND, &iR, true);
		}
		// rect area already invalidated, so no need to do anything else
		else
			return true;		
	#endif
	}
	
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
#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) return false;
#endif
	return !!SetWindowText((HWND)windowHWND, title); // On Windows, this is DEFINE'd in win32_tuf8.h as GetWindowTextUTF8, which enables Windows' WCHAR and UNICODE.
}

void JS_Window_GetTitle(void* windowHWND, char* titleOutNeedBig, int titleOutNeedBig_sz)
{
#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) { titleOutNeedBig[0] = 0; return; }
#endif
	GetWindowText((HWND)windowHWND, titleOutNeedBig, titleOutNeedBig_sz); // On Windows, this is DEFINE'd in win32_tuf8.h as GetWindowTextUTF8, which enables Windows' WCHAR and UNICODE.
}

void JS_Window_GetClassName(HWND windowHWND, char* classOut, int classOut_sz)
{
#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) { classOut[0] = 0; return; }
#endif

#ifdef _WIN32
	classOut[0] = '\0';
	wchar_t* classW = (wchar_t*)alloca(1024 * sizeof(wchar_t));
	if (classW)
		if (GetClassNameW(windowHWND, classW, 1024))
			WideCharToMultiByte(CP_UTF8, 0, classW, -1, classOut, classOut_sz, NULL, NULL);
#else
	GetClassName(windowHWND, classOut, classOut_sz);
#endif
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

double* JS_ArrayFromAddress(double address)
{
	// Casting to intptr_t removes fractions and, in case of 32-bit systems, truncates too large addresses.
	//		This provides easy way to check whether address was valid.
	intptr_t intAddress = (intptr_t)address;
	if ((double)intAddress == address)
		return (double*)intAddress;
	else
		return nullptr;
}

void JS_AddressFromArray(double* array, double* addressOut)
{
	*addressOut = (double)(intptr_t)array;
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
	return (bool)IsWindow((HWND)windowHWND);
#else
	return ValidatePtr(windowHWND, "HWND");
	/*
	Older versïons: 

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
		}*/

		/*HWND main = GetMainHwnd();
		if (main) {
			if (target == main)
				return true;
			else {
				EnumChildWindows(main, JS_Window_IsWindow_Callback_Child, reinterpret_cast<LPARAM>(&target));
				if (!target) return true;
		}
	}*/
	/*
		EnumWindows(JS_Window_IsWindow_Callback_Top, reinterpret_cast<LPARAM>(&target));

		return !target;
	}*/
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

	if (!IsWindow((HWND)windowHWND)) return false;

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
	
	WPARAM fullWP;
	if (wParamHighWord || ((wParam < 0) && (-(2^15) > wParam))) // WARNING: Negative values (such as mousewheel turns) are not bitwise encoded the same in low WORD vs entire WPARAM. So if small negative, assume that low WORD is intended.
		fullWP = MAKEWPARAM(wParam, wParamHighWord);
	else
		fullWP = (WPARAM)(int64_t)wParam;
		
	LPARAM fullLP;
	if (lParamHighWord || ((lParam < 0) && (-(2 ^ 15) > lParam)))
		fullLP = MAKELPARAM(lParam, lParamHighWord);
	else
		fullLP = (LPARAM)(int64_t)lParam;
		
	HWND hwnd = (HWND)windowHWND;

	// Is this window currently being intercepted?
	if (mapWindowData.count(hwnd)) {
		sWindowData& w = mapWindowData[hwnd];
		if (w.mapMessages.count(uMsg)) {
#ifdef _WIN32
			CallWindowProc((WNDPROC)(intptr_t)w.origProc, hwnd, uMsg, fullWP, fullLP);
#else
			((WNDPROC)(intptr_t)w.origProc)(hwnd, uMsg, fullWP, fullLP);
#endif
			return true;
		}
	}
	// Not intercepted, so just pass on to Win32
	return !!PostMessage(hwnd, uMsg, fullWP, fullLP);
}
 

int JS_WindowMessage_Send(void* windowHWND, const char* message, double wParam, int wParamHighWord, double lParam, int lParamHighWord)
{
	using namespace Julian;

	if (!IsWindow((HWND)windowHWND)) return 0;

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

	WPARAM fullWP;
	if (wParamHighWord || ((wParam < 0) && (-(2 ^ 15) > wParam))) // WARNING: Negative values (such as mousewheel turns) are not bitwise encoded the same in low WORD vs entire WPARAM. So if small negative, assume that low WORD is intended.
		fullWP = MAKEWPARAM(wParam, wParamHighWord);
	else
		fullWP = (WPARAM)(int64_t)wParam;

	LPARAM fullLP;
	if (lParamHighWord || ((lParam < 0) && (-(2 ^ 15) > lParam)))
		fullLP = MAKELPARAM(lParam, lParamHighWord);
	else
		fullLP = (LPARAM)(int64_t)lParam;

	HWND hwnd = (HWND)windowHWND;

	// Is this window currently being intercepted?
	if (mapWindowData.count(hwnd)) {
		sWindowData& w = mapWindowData[hwnd];
		if (w.mapMessages.count(uMsg)) {
#ifdef _WIN32
			return (int)CallWindowProc((WNDPROC)(intptr_t)w.origProc, hwnd, uMsg, fullWP, fullLP);
#else
			return (int)((WNDPROC)(intptr_t)w.origProc)(hwnd, uMsg, fullWP, fullLP);
#endif
		}
	}
	// Not intercepted, so just pass on to Win32
	return (int)SendMessage(hwnd, uMsg, fullWP, fullLP);
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
#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) return false;
#endif
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

//TIMERPROC JS_InvalidateTimer(HWND hwnd, UINT msg, UINT_PTR timerID, DWORD millisecs);
// After delay, this timer is called to re-invalidate stored updateRect, which will re-send WM_PAINT
void JS_InvalidateTimer(HWND hwnd, UINT msg, UINT_PTR timerID, DWORD millisecs)
{
	using namespace Julian;
	//if (IsWindow(hwnd)) // Is it really necessary to check IsWindow?  On WindowsOS, GetClientRect and other window-related functions don't crash if hwnd doesn't exist.
	//{
		if (mapWindowData.count(hwnd))
		{
			RECT& i2R = mapWindowData[hwnd].mustInvalidRect;
			if (!RECT_IS_EMPTY(i2R)) // Anything left to paint?
			{
				RECT& iR = mapWindowData[hwnd].doneInvalidRect;
				if (RECT_IS_EMPTY(iR))
					iR = i2R;
				else
					UNION_RECT(iR, i2R);
				InvalidateRect(hwnd, &iR, true);
			}
			i2R = { 0, 0, 0, 0 };
		}
		else
		{
			RECT cR; GetClientRect(hwnd, &cR);
			InvalidateRect(hwnd, &cR, true);
		}
	//}
}


LRESULT CALLBACK JS_WindowMessage_Intercept_Callback(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	using namespace Julian;

	// If hwnd not in map, something went awry. Don't know how to call original window process.
	// This might mean that the window received a WM_DESTROY message and was removed from the map, but wasn't in fact destroyed?
	if (mapWindowData.count(hwnd) == 0)
		return 1;

	// Get reference/alias because want to write updates into existing struct.
	sWindowData& w = mapWindowData[hwnd]; 

	// INTERCEPT / BLOCK WINDOW MESSAGES
	
	// Event that should be intercepted? 
	if (w.mapMessages.count(uMsg)) // ".contains" has only been implemented in more recent C++ versions
	{
		w.mapMessages[uMsg].time = time_precise();
		w.mapMessages[uMsg].wParam = wParam;
		w.mapMessages[uMsg].lParam = lParam;

		// If event will not be passed through, must return inside from inside this switch.
		if (w.mapMessages[uMsg].passthrough == false)
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

	// PASSTHROUGH MESSAGE: All messages that aren't blocked, end up here
	
	LRESULT result; // of call to original WndProc
	bool gotResultYet = false; // Unfortunately, the return value of WM_PAINT is not restricted, so I need a separate variable to record whether origproc was already called

	// COMPOSITE LICE BITMAPS - if any
	/* The basic idea is quite simple:  
		* Assume that REAPER only paints its windows when receiving WM_PAINT messages.
		* Whenever a WM_PAINT is received, first call the window's original WndProc to update the window with REAPER's own stuff (which should paint over composited bitmap images), 
		* then, if any bitmaps are composited to this window, blit the bitmaps into the Invalidated part of the window.
		* Hope that the tiny fraction of a second between REAPER's updates and the bitmap blits don't cause flickering.
	
	There are a few crucial differences between WM_PAINT painting on WindowsOS vs Linux/macOS with WDL/swell:
				  	
	  	1) BeginPaint vs GetUpdateRect:
	 		WDL/swell doesn't provide a separate GetUpdateRect function, but BeginPaint can be used. 
	  		Fortunately, swell's BeginPaint is innocuous: It doesn't do background erasing, and doesn't even need an EndPaint. (EndPaint is simply an empty function that simply returns TRUE). 
	  		In fact, swell's GetDC uses BeginPaint to get the HDC.
	  	2) The UpdateRect:
	 		On WindowsOS, the entire RECT returned by GetUpdateRect (named uR) is not necessarily invalidated -- uR is simply the smallest RECT that contains all the invalidated sub-RECTs.
	  		So, on WindowsOS, to make sure that the extension knows exactly which areas will be re-painted, the entire uR has to be Invalidated before calling the original WndProc.
			(Why is this important?  If transparent bitmaps are re-blitted onto parts of the window that have not been re-painted, the blitted images will pile up, becoming more and more opaque, so must be very careful!)
	  		On WDL/swell (as far as I can tell), the entire RECT returned by BeginPaint will be re-drawn, so no need for additional InvalidateRect.
	  	3) InvalidateRect:
	 		The previous point is fortunate, since on WDL/swell, InvalidateRect doesn't have any effect if called from within this callback function!!  WARNING!!
	  		WDL/swell's InvalidateRect doesn't change the behaviour of an immediately following call to the original WndProc inside the callback, and doesn't change the UpdateRect of the next time that WM_PAINT is sent.
	 		On WindowsOS, InvalidateRect can succesfully be called inside the callback.
	  	4) Original WndProc:
	 		On WindowsOS, the original WndProc is responsible for erasing the background and resetting the UpdateRect.
	  		On WDL/swell, the original WndProc doesn't seem to do that, and the background gets erased even if WM_PAINT and WM_ERASEBCKGRD is blocked.
	  	5) Flickering:
	 		On WindowsOS, the amount of flickering seems to be affected by the frequency of WM_PAINT updates, each of which requires blitting of the composited bitmaps.
	  		I am not sure why this is happening, and it may be influenced randomly by GPU speed, screen refresh rate, etc.  The only way that I have found to reduce or even completely remove flicker, is to lower the frequency.
	  		On WDL/swell, preseumably since REAPER handles both window painting and bitmap blitting, they remain synchronized.  
			It seems that REAPER does the blitting after doing its own updating of the window contents, and the window is updated once per script defer cycle.
	 		Another WARNING!!  On macOS, this may not be true if GPU acceleration is enabled in REAPER's Preferences -> Advanced UI/system tweaks.  Some users report that the composited images are not visible at all.  
	  
	  So, how does this extension handle WM_PAINT and compositing differently on WindowsOS vs WDL/swell?
	  
		1) InvalidateRect:
			On WindowsOS, InvalidateRect will be called within the callback to ensure that the entire updateRECT will in fact be redrawn.
		2) Timer callback:
			On WindowsOS, JS_Composite_Delay can be used to set the frequency of WM_PAINT updates.  
			WM_PAINT messages that are received too soon will be blocked, and a timer callback function will be set to re-Invalidate the window after the delay time.
		3) RECT mustInvalidRect:
			On WindowsOS, to ensure that the window is correctly updated by the timer after the delay, the blocked UpdateRect RECT that should have been updated is stored in mapWindowData[hwnd].mustInvalidRect.
			mapWindowData[hwnd].mustInvalidRect stores the union of all RECTs that must be invalidated in the next WM_PAINT cycle.
	  	3) Skip unneccesary InvalidateRect:
			On WindowsOS, given that InvalidateRect will in any case be called in this WndProc, and that to-be-invalidated RECTs are stored in mustInvalidRect,
			there is no need to call InvalidateRect dozens of times per paint cycle (which may happen with scripts such as Area(51), when it composites hundreds of bitmaps at once).
			Instead, after the first InvalidateRect called by a script, the extension will store all subsequent Invalidated RECTs in mustInvalidRect, to be Invalidated with a single call just before calling the original WndProc.
		4) RECT doneInvalidRect:
			On WDL/swell, since InvalidateRect cannot be called inside this WndProc, and since WM_PAINT don't need to be delayed, mustInvalidRect is never used.
			However, another entry in the mapWindowData, doneInvalidRect, is used to store RECTs that have already been invalidated by this extension (and scripts) in this paint cycle.
			If the extension receives an InvalidateRect request that falls within the already-invalidated doneInvalidRect, it can be skipped.
	 */
	if (uMsg == WM_PAINT)
	{
		//char temp[1000]; // just for debugging messages
		//int c = 0;

		// These two RECTs must guaranteed always reset at each WM_PAINT cycle -- except that mustInvalidRect will be re-assigned if a Timer is set up on WindowsOS.
		RECT iR  = w.doneInvalidRect;
		RECT i2R = w.mustInvalidRect; // Stuff that extension wants to update (stored updateRect from previous, delayed cycles, as well as bitmaps that were updated)
		w.doneInvalidRect = { 0, 0, 0, 0 };
		w.mustInvalidRect = { 0, 0, 0, 0 };

		// If the window is not composited to any bitmaps, and if there is no remaining to-be-invalidated areas (perhaps from previous timer), can skip this.
		if (!w.mapBitmaps.empty() || !RECT_IS_EMPTY(w.mustInvalidRect))
		{
			#ifdef _WIN32
			// Only WindowsOS: Does this window have delay settings?  If so, must check time and perhaps set timer to return later.
			if (mapDelayData.count(hwnd))
			{
				// Calculate delay
				auto d = mapDelayData[hwnd];
				double delay = d.minTime + (d.maxTime - d.minTime) * (w.mapBitmaps.size() / d.maxBitmaps);
				if (delay > d.maxTime)
					delay = d.maxTime;

				double timeNow = time_precise();
				if (w.lastTime > timeNow) // lastTime should have starting value of 0, but double-check
					w.lastTime = 0;

				// Has delay passed?  No, too soon.
				if (timeNow < w.lastTime + delay)
				{
					// If the updateRect is not validated before returning, the system will continue to try to send WM_PAINT, and this will slow down REAPER.
					//		(For example, script defer cycle will slow down to the same rate as the composite delay.)
					// But the updateRect must be repainted as some point, so store the rect in w.invalidRect, to be manually invalidated when delay is over.
					RECT uR; GetUpdateRect(hwnd, &uR, false); // uR should cover doneInvalidRect
					if (RECT_IS_EMPTY(i2R)) // empty rect?
						i2R = uR;
					else if (!RECT_IS_EMPTY(uR))
						UNION_RECT(i2R, uR);
					w.mustInvalidRect = i2R;
					RECT cR; GetClientRect(hwnd, &cR);
					ValidateRect(hwnd, &cR); // Don't want REAPER to desperately re-send WM_PAINT
					if (!w.timerID)
						w.timerID = SetTimer(hwnd, 0, static_cast<UINT>(1001 * (w.lastTime + delay - timeNow)), (TIMERPROC)JS_InvalidateTimer); // Instead, set my own timer to invalidate
					return 0; // What is best?  1 or 0 return value?  0 = message processed.
				}

				// OK, delay has passed, so remove any remaining timer and go ahead with blitting
				else
				{
					if (w.timerID)
					{
						KillTimer(hwnd, w.timerID);
						w.timerID = 0;
					}
					w.lastTime = timeNow;
				}
			}

			RECT uR; GetUpdateRect(hwnd, &uR, false); // Should cover w.doneInvalidRect
			RECT cR; GetClientRect(hwnd, &cR);
			//c = c + sprintf(temp + c, "\nuR before: %i, %i, %i, %i", uR.left, uR.top, uR.right, uR.bottom);
			if (RECT_IS_EMPTY(uR))
				uR = i2R;
			else if (!RECT_IS_EMPTY(i2R)) // if i2R non-empty, combine with uR and make sure both are completely invalidated.
				UNION_RECT(uR, i2R);
			CONTRACT_TO_CLIENTRECT(uR, cR);
			//c = c + sprintf(temp + c, "\niR: %i, %i, %i, %i", iR.left, iR.top, iR.right, iR.bottom);
			//c = c + sprintf(temp + c, "\ni2R: %i, %i, %i, %i", i2R.left, i2R.top, i2R.right, i2R.bottom);
			//c = c + sprintf(temp + c, "\nuR after: %i, %i, %i, %i", uR.left, uR.top, uR.right, uR.bottom);
			if (uR.left < iR.left || uR.top < iR.top || uR.right > iR.right || uR.bottom > iR.bottom) // Has entire uR already been Invalidated?  If not Invalidate.
				InvalidateRect(hwnd, &uR, false); // WARNING! The entire updateRect is not necessarily invalidated.  So even if uR contains iR, must still InvalidateRect entire uR to ensure that all parts will be redrawn.

			// CALL ORIGINAL WNDPROC
			result = CallWindowProc((WNDPROC)(intptr_t)w.origProc, hwnd, uMsg, wParam, lParam); gotResultYet = true;

			#else

			PAINTSTRUCT p; BeginPaint(hwnd, &p); // swell doesn't provide a separate GetUpdateRect function. swell's BeginPaint is innocuous and doesn't start painting - even GetDC uses BeginPaint to get DC.  Doesn't need EndPaint, which just return TRUE.
			RECT& uR = p.rcPaint;
			RECT  cR; GetClientRect(hwnd, &cR);

			// CALL ORIGINAL WNDPROC
			result = ((WNDPROC)(intptr_t)w.origProc)(hwnd, uMsg, wParam, lParam); gotResultYet = true;

			#endif

			/*	Before blitting, first check: is entire client area invalidated?
			If 1) only part of the client area is invalidated, and 2) if the invalidatedRECT only partially overlaps the destinationRECT of a bitmap 
				-- and 3) if the bitmap is stretched/scaled -- it may not be possible to directly blit only the overlapped part into the window.
			For example, if a 5x5 bitmap is blitted into a 1000x1000 area, but only a small 100x50 area is invalidated (which may happen if a tooltip pops up),
				StretchBlt and AlphaBlend, which only accept integer arguments, cannot blit only the affected area.
			How can this be handled?  What this extension does, is to use a two-step solution:
				1) First, any bitmap that is partially overlapped and also stretched, is ScaledBlitted into a temporary bitmap, Julian::compositeCanvas (with stretching, but without alphablending).
				2) The overlapping section (*only* this section) is then blitted without stretching (but with alpha blending) onto the window.
			(When this two-step solution is applied, must must make sure that the temporary canvas is at least as big as the target client area.) 

			Since this two-step solution is slower than directly blitting to the window, the extension only uses it when necessary.
			In particular, if the entire client area is invalidated, the extension relies on WindowsOS or WDL/swell 
				to automatically clip blitted images to the client area, since they can do so much faster.   

			UPDATE v1.215:
			In this version, if __APPLE__ and Metal is enabled, extension will *not* rely on REAPER to clip images to client area, even if entire client area is invalidated.  
			Therefore, partially offscreen bitmaps will handles with two-step solution described above.
			 */

			bool updateEntireClientArea = (uR.left <= 0 && uR.top <= 0 && uR.right >= cR.right && uR.bottom >= cR.bottom);
#ifdef __APPLE__
			int metal = (SWELL_EnableMetal(hwnd, 0) > 0);
#else
			int metal = false;
#endif
			if (metal || !updateEntireClientArea)
			{
				int width = compositeCanvas->getWidth();
				int height = compositeCanvas->getHeight();
				if (width < cR.right || height < cR.bottom)
					BOOL resizeOK = compositeCanvas->resize(width > cR.right ? width : cR.right, height > cR.bottom ? height : cR.bottom);
			}

			// Finally, do the compositing!  Iterate through all linked bitmaps.
			HDC windowDC = GetDC(hwnd);
			HDC canvasDC = compositeCanvas->getDC();
			HDC srcDC; // For each source bitmap -- Will be assigned later
			if (windowDC && canvasDC)
			{
				for (auto& b : w.mapBitmaps) // Map that contains all the linked bitmaps: b.first is a LICE_IBitmap*
				{
					sBlitRects& coor = b.second; // Coordinates: b.second is a struct with src and dst coordinates that specify where the bitmaps should be blitted
					if (coor.dstw != 0 && coor.dstw != 0) // Making size 0 is common way of quickly hiding bitmap without needing to call Composite_Unlink
					{
						if (mLICEBitmaps.count(b.first)) // Double-check that the bitmap actually still exist?
						{
							// Calculate destination rect - updated for current client size. If dstw or dsth is -1, bitmap is stretched over entire width or height of clientRect. If not, use dst coordinates.
							RECT dstR = { coor.dstx, coor.dsty, coor.dstw, coor.dsth };  // WARNING: dstR and overlapR use relative width/height, not absolute right/left
							if (coor.dstw == -1) { dstR.left = 0; dstR.right = cR.right; }
							if (coor.dsth == -1) { dstR.top = 0; dstR.bottom = cR.bottom; }

							// Does dstR overlap with uR?  If not, skip bitmap.  (WARNING: dstR uses width/height, uR uses right/bottom.)
							if (dstR.left < uR.right && dstR.top < uR.bottom && dstR.top + dstR.bottom > uR.top && dstR.left + dstR.right > uR.left)
							{
								// OPTION 1:  Blit direcly to windowDC, let blitting function clip images that are partially offscreen.
								if (updateEntireClientArea && !metal)
								{
									if (srcDC = b.first->getDC())
										jsAlphaBlend(windowDC, dstR.left, dstR.top, dstR.right, dstR.bottom, srcDC, coor.srcx, coor.srcy, coor.srcw, coor.srch);
								}

								else
								{
									RECT overlapR; // How does the dstR overlap with the updateRect uR?  Only the overlapping part must be blitted.
									overlapR.left = (dstR.left > uR.left) ? dstR.left : uR.left;
									overlapR.top = (dstR.top > uR.top) ? dstR.top : uR.top;
									overlapR.right = (dstR.left + dstR.right < uR.right) ? dstR.left + dstR.right - overlapR.left : uR.right - overlapR.left;
									overlapR.bottom = (dstR.top + dstR.bottom < uR.bottom) ? dstR.top + dstR.bottom - overlapR.top : uR.bottom - overlapR.top;

									// OPTION 2: Blit direcly to windowDC.
									if ((overlapR.right == dstR.right && overlapR.bottom == dstR.bottom) // dstR is fully within uR, so no need to clip and can StretchBlt directly into windowDC
									|| (dstR.right == coor.srcw && dstR.bottom == coor.srch)) // or, dstR and uR partially overlap, but no stretching: Only blit overlapping part
									{
										if (srcDC = b.first->getDC())
											jsAlphaBlend(windowDC, overlapR.left, overlapR.top, overlapR.right, overlapR.bottom, srcDC, coor.srcx + (overlapR.left - dstR.left), coor.srcy + (overlapR.top - dstR.top), coor.srcw + (overlapR.right - dstR.right), coor.srch + (overlapR.bottom - dstR.bottom));
									}

									// OPTION 3: Stretching and partially overlapping: Two steps: first LICE_ScaledBlt to temporary canvas, then GDI BitBlt overlapping part into window
									else
									{
										LICE_ScaledBlit(compositeCanvas, b.first, dstR.left, dstR.top, dstR.right, dstR.bottom, coor.srcx, coor.srcy, coor.srcw, coor.srch, 1, LICE_BLIT_MODE_COPY);
										jsAlphaBlend(windowDC, overlapR.left, overlapR.top, overlapR.right, overlapR.bottom, canvasDC, overlapR.left, overlapR.top, overlapR.right, overlapR.bottom);
									}
								}
							}
						}
					}
				}
				ReleaseDC(hwnd, windowDC);
			}
		}
		//if (c) ShowConsoleMsg(temp);
	}
	
	// NO COMPOSITING -- call original WndProc and return results
	
	if (!gotResultYet)
	{
		#ifdef _WIN32
		result = CallWindowProc((WNDPROC)(intptr_t)w.origProc, hwnd, uMsg, wParam, lParam);
		#else
		result = ((WNDPROC)(intptr_t)w.origProc)(hwnd, uMsg, wParam, lParam);
		#endif

		// If DESTROYING window, must remove from mapWindowData before going ahead.
		// Is WM_DESTROY guaranteed to be the last message that a window receives?
		if (uMsg == WM_DESTROY) //&& !IsWindow(hwnd))
			mapWindowData.erase(hwnd);
	}

	return result;
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
	if (Julian::mapWindowData.count(hwnd) == 0) 
	{
		if (!IsWindow(hwnd)) 
			return ERR_NOT_WINDOW;

		HDC windowDC = (HDC)JS_GDI_GetClientDC(hwnd);
		if (!windowDC) 
			return ERR_WINDOW_HDC;

		// Try to get the original process.
		LONG_PTR origProc = 0;
		#ifdef _WIN32
		origProc = SetWindowLongPtrA(hwnd, GWLP_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
		#else
		origProc = SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
		#endif
		if (!origProc) 
			return ERR_ORIG_WNDPROC;

		Julian::mapWindowData.emplace(hwnd, sWindowData{ origProc }); // , map<UINT, sMsgData>{}, map<LICE_IBitmap*, sBlitRects>{} }); // Insert empty map
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
		if (!IsWindow(hwnd)) 
			return ERR_NOT_WINDOW;

		HDC windowDC = (HDC)JS_GDI_GetClientDC(hwnd);
		if (!windowDC) 
			return ERR_WINDOW_HDC;

		// Try to get the original process.
		LONG_PTR origProc = 0;
#ifdef _WIN32
		origProc = SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
#else
		origProc = SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
#endif
		if (!origProc) 
			return ERR_ORIG_WNDPROC;

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
	//if (existingMessages.empty() && mapWindowData[hwnd].mapBitmaps.empty())
	JS_WindowMessage_RestoreOrigProcAndErase();

	return TRUE;
}

void JS_WindowMessage_ReleaseAll()
{
	using namespace Julian;
	for (auto& m : Julian::mapWindowData) 
		m.second.mapMessages.clear(); // delete intercepts, but leave linked bitmaps alone
	JS_WindowMessage_RestoreOrigProcAndErase();
}

void JS_WindowMessage_ReleaseWindow(void* windowHWND)
{
	using namespace Julian;
	HWND hwnd = (HWND)windowHWND;
	if (mapWindowData.count(hwnd))
		mapWindowData[hwnd].mapMessages.clear(); // delete intercepts, but leave linked bitmaps alone

	JS_WindowMessage_RestoreOrigProcAndErase();
	/*	if (mapWindowData[hwnd].mapBitmaps.empty()) 
			JS_WindowMessage_RestoreOrigProcAndErase(hwnd); // no linked bitmaps either, so can restore original WNDPROC
		else 
			mapWindowData[hwnd].mapMessages.clear(); // delete intercepts, but leave linked bitmaps alone
			*/
}

// Call this function to clean up intercepts, after clearing mapMessages or mapBitmaps.
// WARNING: DO NOT call this function inside a loop over mapWindowData, since this function may delete entries in mapWindowData.
// This function does not do InvalidateRect.
void JS_WindowMessage_RestoreOrigProcAndErase()
{
	using namespace Julian;
	std::set<HWND> toDelete;
	for (auto& m : Julian::mapWindowData) 
		if (m.second.mapBitmaps.empty() && m.second.mapMessages.empty())
			toDelete.insert(m.first);

	for (HWND hwnd : toDelete)
	{
		if (IsWindow(hwnd) && Julian::mapWindowData[hwnd].origProc)
		{
			#ifdef _WIN32
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, Julian::mapWindowData[hwnd].origProc);
			#else			
			SetWindowLong(hwnd, GWL_WNDPROC, Julian::mapWindowData[hwnd].origProc);
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
	string  fileStr	= pathAndFileName;
	HCURSOR cursor	= NULL;
	if ((forceNewLoadOptional && *forceNewLoadOptional)
		|| !(mapFileToMouseCursor.count(fileStr))) // If not loaded yet, must load from file
	{
#ifdef _WIN32 // convert to WideChar for Unicode support
		int wideCharLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pathAndFileName, -1, NULL, 0);
		if (wideCharLength)
		{
			WCHAR* widePath = (WCHAR*) alloca(wideCharLength * sizeof(WCHAR) * 2);
			if (widePath)
			{
				wideCharLength = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, pathAndFileName, -1, widePath, wideCharLength * 2);
				if (wideCharLength)
				{
					cursor = LoadCursorFromFileW(widePath);
				}
			}
		}
#else // swell uses UTF8, so no need to convert
		cursor = SWELL_LoadCursorFromFile(pathAndFileName);
#endif
		if (cursor)
			mapFileToMouseCursor[fileStr] = cursor;
	}
	else
		cursor = mapFileToMouseCursor[fileStr];

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
#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) return nullptr;
#endif
	HDC dc = GetDC((HWND)windowHWND);
	if (dc) Julian::GDIHDCs.emplace((HWND)windowHWND, dc);
	return dc;
}

void* JS_GDI_GetWindowDC(void* windowHWND)
{
#ifdef _WDL_SWELL
	if (!ValidatePtr(windowHWND, "HWND")) return nullptr;
#endif
	HDC dc = GetWindowDC((HWND)windowHWND);
	if (dc) Julian::GDIHDCs.emplace((HWND)windowHWND, dc);
	return dc;
}

void* JS_GDI_GetScreenDC()
{
	HDC dc = GetDC(nullptr);
	if (dc) Julian::GDIHDCs.emplace(nullptr, dc);
	return dc;
}

int JS_GDI_ReleaseDC(void* deviceHDC, void* windowHWNDOptional)
{
	// Return -1 if the HWND/HDC pair has not been created via this extension.
	// Remember that the HWND and HDC can be passed in any order, for compatibility with previous versions.
	pair<HWND, HDC> p = make_pair((HWND)windowHWNDOptional, (HDC)deviceHDC);
	if (!Julian::GDIHDCs.count(p))
	{
		p = make_pair((HWND)deviceHDC, (HDC)windowHWNDOptional);
		if (!Julian::GDIHDCs.count(p))
			return -1;
	}
	Julian::GDIHDCs.erase(p);
#ifdef _WIN32
	return ReleaseDC(p.first, p.second);
#else
	ReleaseDC(p.first, p.second);
	return 1;
#endif
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
#ifdef _WIN32
	return DrawTextUTF8((HDC)deviceHDC, text, len, &r, intMode);
#else
	return DrawText((HDC)deviceHDC, text, len, &r, intMode);
#endif
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
	if (!ValidatePtr(windowHWND, "HWND")) return false;
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
	bool isOK = false;
	if (ValidatePtr(windowHWND, "HWND"))
	{
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
	}
	return isOK;
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Functions for compositing into REAPER's UI

int JS_Composite(HWND hwnd, int dstx, int dsty, int dstw, int dsth, LICE_IBitmap* sysBitmap, int srcx, int srcy, int srcw, int srch, bool autoUpdate = false)
{
	using namespace Julian;
	if (!mLICEBitmaps.count(sysBitmap)) return ERR_NOT_BITMAP;
	if (!mLICEBitmaps[sysBitmap]) return ERR_NOT_SYSBITMAP; // Does this have a linked HDC = Is this a sysbitmap?
	if (!IsWindow(hwnd)) return ERR_NOT_WINDOW;
		
	// If autoUpdate, the composite function will call InvalidateRect to start the blitting, so the destination rect must be calculated.
	// ALSO: If the window and bitmap were already composited, its destination coordinates may be moving, so the invalidated rect must cover the new as well as the previous dst rect.
	RECT cR; // Client rect
	RECT dstR; // Destination rect (that must be Invalidated)
	if (autoUpdate)
	{
		GetClientRect(hwnd, &cR);
		dstR = { dstx, dsty, dstx + dstw, dsty + dsth };
		if (dstw == -1) { dstR.left = 0; dstR.right = cR.right; }
		if (dsth == -1) { dstR.top = 0; dstR.bottom = cR.bottom; }
	}

	// If window not already intercepted, get original window proc and emplace new struct
	if (Julian::mapWindowData.count(hwnd) == 0) 
	{
		#ifdef _WIN32
		LONG_PTR origProc = SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
		if (!origProc || (origProc == (LONG_PTR)JS_WindowMessage_Intercept_Callback)) // Oops, does this window already have the extension callback as wndproc?  Something went awry.
			return ERR_ORIG_WNDPROC;
		if (GetWindowLongPtr(hwnd, GWLP_WNDPROC) != (LONG_PTR)JS_WindowMessage_Intercept_Callback)
			return ERR_NEW_WNDPROC;
		#else
		LONG_PTR origProc = SetWindowLong(hwnd, GWL_WNDPROC, (LONG_PTR)JS_WindowMessage_Intercept_Callback);
		if (!origProc || (origProc == (LONG_PTR)JS_WindowMessage_Intercept_Callback)) 
			return ERR_ORIG_WNDPROC;
		if (GetWindowLong(hwnd, GWL_WNDPROC) != (LONG_PTR)JS_WindowMessage_Intercept_Callback) 
			return ERR_NEW_WNDPROC;
		#endif
		
		Julian::mapWindowData[hwnd] = sWindowData{ origProc };
		Julian::mapWindowData[hwnd].mustInvalidRect = { 0, 0, 0, 0 };
		Julian::mapWindowData[hwnd].doneInvalidRect = { 0, 0, 0, 0 };
		Julian::mapWindowData[hwnd].lastTime = 0;
	}

	// OK, already intercepted.  Are window and bitmap already composited?  Must expand dstR to include previous dst rect, so that old position will also be invalidated, and old image removed.
	else if (autoUpdate && mapWindowData[hwnd].mapBitmaps.count(sysBitmap))
	{
		auto& b = mapWindowData[hwnd].mapBitmaps[sysBitmap];
		RECT prevR{ b.dstx, b.dsty, b.dstx + b.dstw, b.dsty + b.dsth };
		if (b.dstw == -1) { prevR.left = 0; prevR.right = cR.right; }
		if (b.dsth == -1) { prevR.top = 0;  prevR.bottom = cR.bottom; }
		if (!RECT_IS_EMPTY(prevR))
			UNION_RECT(dstR, prevR);
	}

	// OK, hwnd should now be in map. Don't use emplace, since may need to replace previous dst or src RECT of already-linked bitmap
	mapWindowData[hwnd].mapBitmaps[sysBitmap] = sBlitRects{ dstx, dsty, dstw, dsth, srcx, srcy, srcw, srch };
	
	if (autoUpdate)
	{
		BOOL invalidateOK;
		RECT& iR = mapWindowData[hwnd].doneInvalidRect;

		// No invalidates yet in this paint cycle, so must send InvalidateRect
		if (RECT_IS_EMPTY(iR))
		{
			iR = dstR;
			invalidateOK = InvalidateRect(hwnd, &iR, true);
		}
		#ifdef _WIN32
		// WindowsOS: If window is already invalidated, store rect in to-be-invalidated i2R, to be invalidated during WM_PAINT callback 
		//		(This avoids dozens of overlapping Invalidates in case of scripts with dozens of composited bitmaps, such as Area(51).)
		else 
		{
			RECT& i2R = mapWindowData[hwnd].mustInvalidRect;
			if (RECT_IS_EMPTY(i2R))
				i2R = dstR;
			else
				UNION_RECT(i2R, dstR);
			invalidateOK = TRUE;
		}
		#else
		// Linux/macOS: InvalidateRect cannot be called inside WM_PAINT callback, so must invalidate now.  But only necessary if rect expands existing iR.
		else if (dstR.left < iR.left || dstR.top < iR.top || dstR.right > iR.right || dstR.bottom > iR.bottom)
		{
			UNION_RECT(iR, dstR);
			invalidateOK = InvalidateRect(hwnd, &iR, true);
		}
		#endif
		return invalidateOK ? TRUE : ERR_INVALIDATE;
	}
	
	return TRUE;
}


void JS_Composite_Unlink(HWND hwnd, LICE_IBitmap* bitmap = nullptr, bool autoUpdate = false)
{
	using namespace Julian;
	if (mapWindowData.count(hwnd)) 
	{
		auto& w = mapWindowData[hwnd];

		if (!bitmap) // Unlink ALL bitmaps from window
		{
			w.mapBitmaps.clear();

			if (IsWindow(hwnd))
			{
				GetClientRect(hwnd, &w.doneInvalidRect);
				InvalidateRect(hwnd, &w.doneInvalidRect, true);
				w.mustInvalidRect = {0, 0, 0, 0};
			}
		}

		// Unlinking bitmap should also erase blitted image from hwnd.
		// Must first store dst rect coordinates, then erase the link, the InvalidateRect the composited area to remove image.
		else if (w.mapBitmaps.count(bitmap))
		{
			sBlitRects b = w.mapBitmaps[bitmap]; // Store existing dst and src rects

			w.mapBitmaps.erase(bitmap); // Erase link

			if (autoUpdate && IsWindow(hwnd))
			{
				RECT dstR; GetClientRect(hwnd, &dstR); // Start as client rect, contract to destination image
				if (b.dstw != -1) { dstR.left = b.dstx; dstR.right = b.dstx + b.dstw; }
				if (b.dsth != -1) { dstR.top = b.dsty; dstR.bottom = b.dsty + b.dsth; }

				if (!RECT_IS_EMPTY(dstR)) // Was bitmap visible?
				{
					RECT& iR = mapWindowData[hwnd].doneInvalidRect;

					// No invalidates yet in this paint cycle, so must send InvalidateRect
					if (RECT_IS_EMPTY(iR))
					{
						iR = dstR;
						InvalidateRect(hwnd, &iR, true);
					}
					// Already invalidated, but rect expands existing iR, 
					// WARNING: When unlinking bitmaps, RestoreOrigProcAndErase might end up completely removing the intercept, so w.mustInvalidate won't be applied inside WM_PAINT callback
					//		Must therefore invalidate here.
					else if (dstR.left < iR.left || dstR.top < iR.top || dstR.right > iR.right || dstR.bottom > iR.bottom)
					{
						UNION_RECT(iR, dstR);
						InvalidateRect(hwnd, &iR, true);
					}
				}
			}
		}
		JS_WindowMessage_RestoreOrigProcAndErase();
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

// Only applicable to WindowsOS.
int  JS_Composite_Delay(HWND hwnd, double minTime, double maxTime, int maxBitmaps, double* prevMinTimeOut, double* prevMaxTimeOut, int* prevBitmapsOut)
{
	using namespace Julian;
	// Return previous values
	if (mapDelayData.count(hwnd))
	{
		*prevMinTimeOut = mapDelayData[hwnd].minTime;
		*prevMaxTimeOut = mapDelayData[hwnd].maxTime;
		*prevBitmapsOut = mapDelayData[hwnd].maxBitmaps;
	}
	else
	{
		*prevMinTimeOut = 0;
		*prevMaxTimeOut = 0;
		*prevBitmapsOut = 0;
	}

	// Remove delay settings if all 0
	if (minTime == 0 && maxTime == 0)
	{
		mapDelayData.erase(hwnd);
		return 1;
	}

	// Check if new values are acceptable
	else if (minTime >= 0 && maxTime >= minTime && maxBitmaps > 1)
	{
		mapDelayData[hwnd] = sDelayData{ minTime, maxTime, maxBitmaps };
		return 1;
	}
	else
		return 0;
}

/////////////////////////////////////////////

void* JS_LICE_CreateBitmap(bool isSysBitmap, int width, int height)
{
	LICE_IBitmap* bm = LICE_CreateBitmap((BOOL)isSysBitmap, width, height); // If SysBitmap, can BitBlt to/from screen like HDC.
	// Immediately get HDC and store, so that all scripts can use the same HDC.
	if (bm) {
		HDC dc = LICE__GetDC(bm);
		Julian::mLICEBitmaps[bm] = dc;
	}
	return bm;
}

int JS_LICE_ListAllBitmaps(char* listOutNeedBig, int listOutNeedBig_sz)
{
	std::set<HWND> bitmaps;
	for (auto& i : Julian::mLICEBitmaps) bitmaps.insert((HWND)(void*)i.first);
	return ConvertSetHWNDToString(bitmaps, listOutNeedBig, listOutNeedBig_sz);
}

int JS_LICE_ArrayAllBitmaps(double* reaperarray)
{
	// Enumerate through all top-level windows and store ther HWNDs in a set. (Sets are nice, since will automatically check for uniqueness.)
	std::set<HWND> bitmaps;
	for (auto& i : Julian::mLICEBitmaps) bitmaps.insert((HWND)(void*)i.first);
	return ConvertSetHWNDToArray(bitmaps, reaperarray);
}

int JS_LICE_GetHeight(void* bitmap)
{
	using namespace Julian;
	if (mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		return LICE__GetHeight((LICE_IBitmap*)bitmap);
	else 
		return 0;
}

int JS_LICE_GetWidth(void* bitmap)
{
	using namespace Julian;
	if (mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		return LICE__GetWidth((LICE_IBitmap*)bitmap);
	else
		return 0;
}

void* JS_LICE_GetDC(void* bitmap)
{
	using namespace Julian;
	if (mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		return mLICEBitmaps[(LICE_IBitmap*)bitmap];
	else
		return nullptr;
}


void JS_LICE_DestroyBitmap(LICE_IBitmap* bitmap)
{
	using namespace Julian;
	
	// Also delete any occurence of this bitmap from UI Compositing
	if (mLICEBitmaps.count(bitmap)) 
	{
		// DANGEROUS! Unlinking and/or destroying may delete entries in this map,
		//		which can crash iterator!
		// So must store matches temporarily in this set:
		std::set<HWND> linkedWindows;
		for (auto& m : mapWindowData) {
			if (m.second.mapBitmaps.count(bitmap))	{
				linkedWindows.insert(m.first);
			}
		}
		for (auto& w : linkedWindows) 
			JS_Composite_Unlink(w, bitmap, true); // Will also InvalidateRect the dst window, may delete mapWindowData if no intercepts.
		mLICEBitmaps.erase(bitmap);
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

/*	
Standard LICE modes: "COPY" (default if empty string), "MASK", "ADD", "DODGE", "MUL", "OVERLAY" or "HSVADJ", 
	any of which may be combined with "ALPHA" to enable per-pixel alpha blending.
In addition to the standard LICE modes, LICE_Blit also offers:
	* "CHANCOPY_XTOY", with X and Y any of the four channels, A, R, G or B. (CHANCOPY_ATOA is similar to MASK mode.)
	* "BLUR" 
	* "ALPHAMUL", which overwrites the destination with a per-pixel alpha-multiplied copy of the source. (Similar to first clearing the destination with 0x00000000 and then blitting with "COPY,ALPHA".)
*/
void JS_LICE_Blit(void* destBitmap, int dstx, int dsty, void* sourceBitmap, int srcx, int srcy, int width, int height, double alpha, const char* mode)
{	
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)destBitmap) && Julian::mLICEBitmaps.count((LICE_IBitmap*)sourceBitmap))
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
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)destBitmap) && Julian::mLICEBitmaps.count((LICE_IBitmap*)sourceBitmap))
		LICE_RotatedBlit((LICE_IBitmap*)destBitmap, (LICE_IBitmap*)sourceBitmap, dstx, dsty, dstw, dsth, (float)srcx, (float)srcy, (float)srcw, (float)srch, (float)angle, cliptosourcerect, (float)alpha, intMode, (float)rotxcent, (float)rotycent);
}

void JS_LICE_ScaledBlit(void* destBitmap, int dstx, int dsty, int dstw, int dsth, void* sourceBitmap, double srcx, double srcy, double srcw, double srch, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)destBitmap) && Julian::mLICEBitmaps.count((LICE_IBitmap*)sourceBitmap))
		LICE_ScaledBlit((LICE_IBitmap*)destBitmap, (LICE_IBitmap*)sourceBitmap, dstx, dsty, dstw, dsth, (float)srcx, (float)srcy, (float)srcw, (float)srch, (float)alpha, intMode);
}

void JS_LICE_Blur(void* destBitmap, int dstx, int dsty, void* sourceBitmap, int srcx, int srcy, int width, int height)
{
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)destBitmap) && Julian::mLICEBitmaps.count((LICE_IBitmap*)sourceBitmap))
		LICE_Blur((LICE_IBitmap*)destBitmap, (LICE_IBitmap*)sourceBitmap, dstx, dsty, srcx, srcy, width, height);
}

void* JS_LICE_LoadPNG(const char* filename)
{
	LICE_IBitmap* sysbitmap = nullptr;
	LICE_IBitmap* png = nullptr;
	sysbitmap = LICE_CreateBitmap(TRUE, 1, 1); // By default, LICE_LoadPNG does not return a SysBitmap. In order to force the use of SysBitmaps, use must supply own bitmap.
	if (sysbitmap) {
		png = LICE_LoadPNG(filename, sysbitmap);
		if (png != sysbitmap) LICE__Destroy(sysbitmap);
		if (png) {
			HDC dc = LICE__GetDC(png);
			Julian::mLICEBitmaps.emplace(png, dc);
		}
	}
	return png;
}

bool JS_LICE_WritePNG(const char* filename, LICE_IBitmap* bitmap, bool wantAlpha)
{
	return LICE_WritePNG(filename, bitmap, wantAlpha);
}

void* JS_LICE_LoadJPG(const char* filename)
{
	LICE_IBitmap* sysbitmap = nullptr;
	LICE_IBitmap* jpg = nullptr;
	sysbitmap = LICE_CreateBitmap(TRUE, 1, 1); // By default, LICE_LoadPNG does not return a SysBitmap. In order to force the use of SysBitmaps, use must supply own bitmap.
	if (sysbitmap) {
		jpg = LICE_LoadJPG(filename, sysbitmap);
		if (jpg != sysbitmap) LICE__Destroy(sysbitmap);
		if (jpg) {
			HDC dc = LICE__GetDC(jpg);
			Julian::mLICEBitmaps.emplace(jpg, dc);
		}
	}
	return jpg;
}

bool JS_LICE_WriteJPG(const char* filename, LICE_IBitmap* bitmap, int quality, bool forceBaseline)
{
	return LICE_WriteJPG(filename, bitmap, quality, forceBaseline);
}


void JS_LICE_Circle(void* bitmap, double cx, double cy, double r, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_Circle((LICE_IBitmap*)bitmap, (float)cx, (float)cy, (float)r, (LICE_pixel)color, (float)alpha, intMode, antialias);
}

bool JS_LICE_IsFlipped(void* bitmap)
{
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		return LICE__IsFlipped((LICE_IBitmap*)bitmap);
	else
		return false;
}

bool JS_LICE_Resize(void* bitmap, int width, int height)
{
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		return LICE__resize((LICE_IBitmap*)bitmap, width, height);
	else
		return false;
}

void JS_LICE_Arc(void* bitmap, double cx, double cy, double r, double minAngle, double maxAngle, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_Arc((LICE_IBitmap*)bitmap, (float)cx, (float)cy, (float)r, (float)minAngle, (float)maxAngle, (LICE_pixel)color, (float)alpha, intMode, antialias);
}

void JS_LICE_Clear(void* bitmap, int color)
{
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_Clear((LICE_IBitmap*)bitmap, (LICE_pixel)color);
}

bool JS_LICE_Blit_AlphaMultiply(LICE_IBitmap* destBitmap, int dstx, int dsty, LICE_IBitmap* sourceBitmap, int srcx, int srcy, int width, int height, double alpha)
{
	//if (Julian::mLICEBitmaps.count((LICE_IBitmap*)destBitmap) && Julian::mLICEBitmaps.count((LICE_IBitmap*)sourceBitmap))
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
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		return LICE__DrawText((LICE_IFont*)LICEFont, (LICE_IBitmap*)bitmap, text, textLen, &r, 0); // I don't know what UINT dtFlags does, so make 0.
	else
		return 0;
}

void JS_LICE_DrawChar(void* bitmap, int x, int y, char c, int color, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_DrawChar((LICE_IBitmap*)bitmap, x, y, c, (LICE_pixel)color, (float)alpha, intMode);
}

void JS_LICE_MeasureText(const char* string, int* widthOut, int* heightOut)
{
	LICE_MeasureText(string, widthOut, heightOut);
}

void JS_LICE_FillRect(void* bitmap, int x, int y, int w, int h, int color, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_FillRect((LICE_IBitmap*)bitmap, x, y, w, h, (LICE_pixel)color, (float)alpha, intMode);
}

void JS_LICE_RoundRect(void* bitmap, double x, double y, double w, double h, int cornerradius, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_RoundRect((LICE_IBitmap*)bitmap, (float)x, (float)y, (float)w, (float)h, cornerradius, color, (float)alpha, intMode, antialias);
}

void JS_LICE_GradRect(void* bitmap, int dstx, int dsty, int dstw, int dsth, double ir, double ig, double ib, double ia, double drdx, double dgdx, double dbdx, double dadx, double drdy, double dgdy, double dbdy, double dady, const char* mode)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_GradRect((LICE_IBitmap*)bitmap, dstx, dsty, dstw, dsth, (float)ir, (float)ig, (float)ib, (float)ia, (float)drdx, (float)dgdx, (float)dbdx, (float)dadx, (float)drdy, (float)dgdy, (float)dbdy, (float)dady, intMode);
}

void JS_LICE_FillTriangle(void* bitmap, int x1, int y1, int x2, int y2, int x3, int y3, int color, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_FillTriangle((LICE_IBitmap*)bitmap, x1, y1, x2, y2, x3, y3, color, (float)alpha, intMode);
}

void JS_LICE_FillPolygon(void* bitmap, const char* packedX, const char* packedY, int numPoints, int color, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_FillConvexPolygon((LICE_IBitmap*)bitmap, (int32_t*)packedX, (int32_t*)packedY, numPoints, color, (float)alpha, intMode);
}

void JS_LICE_FillCircle(void* bitmap, double cx, double cy, double r, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_FillCircle((LICE_IBitmap*)bitmap, (float)cx, (float)cy, (float)r, color, (float)alpha, intMode, antialias);
}

void JS_LICE_Line(void* bitmap, double x1, double y1, double x2, double y2, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_Line((LICE_IBitmap*)bitmap, (float)x1, (float)y1, (float)x2, (float)y2, (LICE_pixel)color, (float)alpha, intMode, antialias);
}

void JS_LICE_Bezier(void* bitmap, double xstart, double ystart, double xctl1, double yctl1, double xctl2, double yctl2, double xend, double yend, double tol, int color, double alpha, const char* mode, bool antialias)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		LICE_DrawCBezier((LICE_IBitmap*)bitmap, xstart, ystart, xctl1, yctl1, xctl2, yctl2, xend, yend, (LICE_pixel)color, (float)alpha, intMode, antialias, tol);
}

void JS_LICE_GetPixel(void* bitmap, int x, int y, double* colorOut)
{
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
		*colorOut = (double)(LICE_GetPixel((LICE_IBitmap*)bitmap, x, y));
	else
		*colorOut = -1;
}

void JS_LICE_PutPixel(void* bitmap, int x, int y, double color, double alpha, const char* mode)
{
	GETINTMODE
	if (Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap))
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
	if (!Julian::mLICEBitmaps.count((LICE_IBitmap*)bitmap)) return false;

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
#ifdef _WDL_SWELL
	if (!ValidatePtr((HWND)listviewHWND, "HWND")) return -1;
#endif
	return ListView_GetItemCount(listviewHWND);
}

int JS_ListView_GetSelectedCount(HWND listviewHWND)
{
#ifdef _WDL_SWELL
	if (!ValidatePtr((HWND)listviewHWND, "HWND")) return -1;
#endif
	return ListView_GetSelectedCount(listviewHWND);
}

int JS_ListView_GetFocusedItem(HWND listviewHWND, char* textOut, int textOut_sz)
{
#ifdef _WDL_SWELL
	if (!ValidatePtr((HWND)listviewHWND, "HWND")) return -1;
#endif
	int index = ListView_GetNextItem(listviewHWND, -1, LVNI_FOCUSED);
	if (index != -1) {
		ListView_GetItemText(listviewHWND, index, 0, textOut, textOut_sz); }
	else
		textOut[0] = '\0';
	return index;
}

void JS_ListView_EnsureVisible(HWND listviewHWND, int index, bool partialOK)
{
#ifdef _WDL_SWELL
	if (ValidatePtr((HWND)listviewHWND, "HWND"))
#endif
	ListView_EnsureVisible(listviewHWND, index, partialOK);
}

int JS_ListView_EnumSelItems(HWND listviewHWND, int index)
{
#ifdef _WDL_SWELL
	if (!ValidatePtr((HWND)listviewHWND, "HWND")) return -1;
#endif
	// WDL/swell doesn't offer all these flag options, so this function only offers SELECTED:
	return ListView_GetNextItem(listviewHWND, index, LVNI_SELECTED);
}

void JS_ListView_GetItem(HWND listviewHWND, int index, int subItem, char* textOut, int textOut_sz, int* stateOut)
{
#ifdef _WDL_SWELL
	if (!ValidatePtr((HWND)listviewHWND, "HWND")) {	textOut[0] = 0; *stateOut = 0; return; }
#endif
	ListView_GetItemText(listviewHWND, index, subItem, textOut, textOut_sz);
	*stateOut = ListView_GetItemState(listviewHWND, index, LVIS_SELECTED | LVIS_FOCUSED);
	// WIN32 and swell define LVIS_SELECTED and LVIS_FOCUSED differently, so if swell, swap values:
	#ifdef _WDL_SWELL
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
#ifdef _WDL_SWELL
	if (!ValidatePtr((HWND)listviewHWND, "HWND")) return 0;
#endif
	int state = ListView_GetItemState(listviewHWND, index, LVIS_SELECTED | LVIS_FOCUSED);
	// WIN32 and swell define LVIS_SELECTED and LVIS_FOCUSED differently, so if swell, swap values:
	#ifdef _WDL_SWELL
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
#ifdef _WDL_SWELL
	if (!ValidatePtr((HWND)listviewHWND, "HWND")) {	textOut[0] = 0; return; }
#endif
	ListView_GetItemText(listviewHWND, index, subItem, textOut, textOut_sz);
}

int JS_ListView_ListAllSelItems(HWND listviewHWND, char* itemsOutNeedBig, int itemsOutNeedBig_sz)
{
#ifdef _WDL_SWELL
	if (!ValidatePtr((HWND)listviewHWND, "HWND")) { itemsOutNeedBig[0] = 0; return 0; }
#endif
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
	PreviewEntry(int id, PCM_source* src, double gain, bool loop, int startOutputChannel)
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
		m_preg.m_out_chan = startOutputChannel;
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
	int startPreview(PCM_source* src, double gain, bool loop, int startOutputChannel)
	{
		auto entry = std::make_unique<PreviewEntry>(m_preview_id_count, src, gain, loop, startOutputChannel);
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



int Xen_StartSourcePreview(PCM_source* src, double gain, bool loop, int startOutputChannel)
{
	if (g_sourcepreviewman == nullptr)
		g_sourcepreviewman = new PCMSourcePlayerManager;
	if (startOutputChannel < 0 || startOutputChannel > 1000)
		startOutputChannel = 0;
	return (int)g_sourcepreviewman->startPreview(src, gain, loop, startOutputChannel);
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
