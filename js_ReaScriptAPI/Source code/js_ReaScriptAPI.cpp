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
			fprintf(stderr, "Unable to import API functions.\n");
			return 0;
		}
		//		Load each of the undocumented functions.
		if (!((*(void **)&CoolSB_GetScrollInfo) = (void *)rec->GetFunc("CoolSB_GetScrollInfo")))
		{
			MB("Unable to import CoolSB_GetScrollInfo function.", "ERROR: ReaScriptAPI extension", 0);
			return 0;
		}
		if (!((*(void **)&AttachWindowTopmostButton) = (void *)rec->GetFunc("AttachWindowTopmostButton")))
		{
			MB("Unable to import AttachWindowTopmostButton function.", "ERROR: ReaScriptAPI extension", 0);
			return 0;
		}
		if (!((*(void **)&AttachWindowResizeGrip) = (void *)rec->GetFunc("AttachWindowResizeGrip")))
		{
			MB("Unable to import AttachWindowResizeGrip function.", "ERROR: ReaScriptAPI extension", 0);
			return 0;
		}
		if (!((*(void **)&CoolSB_SetScrollPos) = (void *)rec->GetFunc("CoolSB_SetScrollPos")))
		{
			MB("Unable to import CoolSB_SetScrollPos function.", "ERROR: ReaScriptAPI extension", 0);
			return 0;
		}

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
		return 1; // success
	}
	else
		return 0;
}


void JS_ReaScriptAPI_Version(double* versionOut)
{
	*versionOut = 0.95;
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
	const char* t = ((strlen(title) == 0)	   ? nullptr : title);
	return FindWindowEx(parentHWND, c, className, t);
}


BOOL CALLBACK JS_Window_Find_Callback_Child(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows* s = reinterpret_cast<sEnumWindows*>(structPtr);
	int len = GetWindowText(hwnd, s->temp, s->tempLen);
	s->temp[s->tempLen - 1] = '\0'; // Make sure that loooong titles are properly terminated.
	for (int i = 0; (s->temp[i] != '\0') && (i < len); i++) s->temp[i] = (char)tolower(s->temp[i]); // FindWindow is case-insensitive, so this implementation is too
	if (     (s->exact  && (strcmp(s->temp, s->target) == 0)    )
		|| (!(s->exact) && (strstr(s->temp, s->target) != NULL)))
	{
		s->hwnd = hwnd;
		return FALSE;
	}
	else
		return TRUE;
}

BOOL CALLBACK JS_Window_Find_Callback_Top(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows* s = reinterpret_cast<sEnumWindows*>(structPtr);
	int len = GetWindowText(hwnd, s->temp, s->tempLen);
	s->temp[s->tempLen-1] = '\0'; // Make sure that loooong titles are properly terminated.
	for (int i = 0; (s->temp[i] != '\0') && (i < len); i++) s->temp[i] = (char)tolower(s->temp[i]); // FindWindow is case-insensitive, so this implementation is too
	if (     (s->exact  && (strcmp(s->temp, s->target) == 0)    )
		|| (!(s->exact) && (strstr(s->temp, s->target) != NULL)))
	{
		s->hwnd = hwnd;
		return FALSE;
	}
	else
	{
		EnumChildWindows(hwnd, JS_Window_Find_Callback_Child, structPtr);
		if (s->hwnd != NULL) return FALSE;
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
	char temp[API_LEN] = ""; // Will temprarily store titles as well as pointer string, so must be longer than TEMP_LEN.
	sEnumWindows e{ titleLower, exact, temp, sizeof(temp), NULL, NULL, 0, NULL };
	EnumWindows(JS_Window_Find_Callback_Top, reinterpret_cast<LPARAM>(&e));
	return e.hwnd;
}



BOOL CALLBACK JS_Window_ArrayFind_Callback_Child(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows* s = reinterpret_cast<sEnumWindows*>(structPtr);
	char title[TEMP_LEN] = "";
	int len = GetWindowText(hwnd, title, TEMP_LEN);
	title[TEMP_LEN - 1] = '\0'; // Make sure that loooong titles are properly terminated.
	// FindWindow is case-insensitive, so this implementation is too. Convert to lowercase.
	for (int i = 0; (title[i] != '\0') && (i < len); i++) title[i] = (char)tolower(title[i]); 
	// If exact, match entire title, otherwise substring
	if (	 (s->exact  && (strcmp(title, s->target) == 0))
		|| (!(s->exact) && (strstr(title, s->target) != NULL)))
	{
		// Is space available in the reaper.array?
		uint32_t& currentArraySize = ((uint32_t*)(s->reaperarray))[0];  
		if (currentArraySize < ((uint32_t*)s->reaperarray)[1])
		{
			(s->reaperarray)[currentArraySize + 1] = (double)((intptr_t)hwnd);
			currentArraySize++;
		}
		/*
		// Convert pointer to string (leaving two spaces for 0x, and add comma separator)
		sprintf_s(s->temp, s->tempLen - 1, "0x%llX,", (unsigned long long int)hwnd);
		// Concatenate to hwndString
		if (strlen(s->hwndString) + strlen(s->temp) < s->hwndLen - 1)
		{
			strcat(s->hwndString, s->temp);
		}
		*/
	}
	return TRUE;
}

BOOL CALLBACK JS_Window_ArrayFind_Callback_Top(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows* s = reinterpret_cast<sEnumWindows*>(structPtr);
	char title[TEMP_LEN] = "";
	int len = GetWindowText(hwnd, title, TEMP_LEN);
	title[TEMP_LEN - 1] = '\0'; // Make sure that loooong titles are properly terminated.
	// FindWindow is case-insensitive, so this implementation is too. Convert to lowercase.
	for (int i = 0; (title[i] != '\0') && (i < len); i++) title[i] = (char)tolower(title[i]); 
	// If exact, match entire title, otherwise substring
	if (	 (s->exact  && (strcmp(title, s->target) == 0))
		|| (!(s->exact) && (strstr(title, s->target) != NULL)))
	{
		// Is space available in the reaper.array?
		uint32_t& currentArraySize = ((uint32_t*)(s->reaperarray))[0];
		if (currentArraySize < ((uint32_t*)s->reaperarray)[1])
		{
			(s->reaperarray)[currentArraySize + 1] = (double)((intptr_t)hwnd);
			currentArraySize++;
		}
		/*
		// Convert pointer to string (leaving two spaces for 0x, and add comma separator)
		sprintf_s(s->temp, s->tempLen - 1, "0x%llX,", (unsigned long long int)hwnd);
		// Concatenate to hwndString
		if (strlen(s->hwndString) + strlen(s->temp) < s->hwndLen - 1)
		{
			strcat(s->hwndString, s->temp);
		}
		*/
	}
	// Now search all child windows before returning
	EnumChildWindows(hwnd, JS_Window_ArrayFind_Callback_Child, structPtr);
	return TRUE;
}

// Cockos SWELL doesn't provide FindWindow, and FindWindowEx doesn't provide the NULL, NULL top-level mode,
//		so must code own implementation...
// This implemetation adds three features:
//		* Searches child windows as well, so that script GUIs can be found even if docked.
//		* Optionally matches substrings.
//		* Finds ALL windows that match title.
void JS_Window_ArrayFind(const char* title, bool exact, double* reaperarray) //const char* section, const char* key)
{
	using namespace Julian;

 	// FindWindow is case-insensitive, so this implementation is too. 
	// Must first convert title to lowercase:
	char titleLower[API_LEN];
	for (int i = 0; i < API_LEN; i++)
		if (title[i] == '\0') { titleLower[i] = '\0'; break; }
		else				  { titleLower[i] = (char)tolower(title[i]); }
	titleLower[API_LEN-1] = '\0'; // Make sure that loooong titles are properly terminated.

	// To communicate with callback functions, use an sEnumWindows:
	char temp[API_LEN] = ""; // Will temporarily store pointer strings, as well as titles.
	sEnumWindows e{ titleLower, exact, temp, sizeof(temp), NULL, NULL, 0, reaperarray }; // All the info that will be passed to the Enum callback functions.

	EnumWindows(JS_Window_ArrayFind_Callback_Top, reinterpret_cast<LPARAM>(&e));
	
	//SetExtState(section, key, (const char*)hwndString, false);
}



BOOL CALLBACK JS_Window_FindChild_Callback(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows* s = reinterpret_cast<sEnumWindows*>(structPtr);
	int len = GetWindowText(hwnd, s->temp, s->tempLen);
	for (int i = 0; (s->temp[i] != '\0') && (i < len); i++) s->temp[i] = (char)tolower(s->temp[i]); // Convert to lowercase
	if (     (s->exact  && (strcmp(s->temp, s->target) == 0))
		|| (!(s->exact) && (strstr(s->temp, s->target) != NULL)))
	{
		s->hwnd = hwnd;
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
	for (int i = 0; i < API_LEN; i++)
		if (title[i] == '\0') { titleLower[i] = '\0'; break; }
		else				  { titleLower[i] = (char)tolower(title[i]); }

	// To communicate with callback functions, use an sEnumWindows:
	char temp[TEMP_LEN];
	sEnumWindows e{ titleLower, exact, temp, sizeof(temp), NULL, NULL, 0, NULL };
	EnumChildWindows((HWND)parentHWND, JS_Window_FindChild_Callback, reinterpret_cast<LPARAM>(&e));
	return e.hwnd;
}



BOOL CALLBACK JS_Window_ArrayAllChild_Callback(HWND hwnd, LPARAM ptr) //strPtr)
{
	using namespace Julian;
	double* reaperarray = reinterpret_cast<double*>(ptr);
	// Is space available in the reaper.array?
	uint32_t& currentArraySize = ((uint32_t*)(reaperarray))[0];
	if (currentArraySize < ((uint32_t*)reaperarray)[1])
	{
		reaperarray[currentArraySize + 1] = (double)((intptr_t)hwnd);
		currentArraySize++;
		return TRUE;
	}
	else
		return FALSE;
	/*
	char* hwndString = reinterpret_cast<char*>(strPtr);
	char temp[TEMP_LEN] = "";
	sprintf_s(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd); // Print with leading 0x so that Lua tonumber will automatically notice that it is hexadecimal.
	if (strlen(hwndString) + strlen(temp) < EXT_LEN - 1)
	{
		strcat(hwndString, temp);
		return TRUE;
	}
	else
		return FALSE;
	*/
}

void JS_Window_ArrayAllChild(void* parentHWND, double* reaperarray) // const char* section, const char* key) //char* buf, int buf_sz)
{
	using namespace Julian;
	//HWND hwnd = (HWND)parentHWND;
	//char hwndString[EXT_LEN] = "";
	EnumChildWindows((HWND)parentHWND, JS_Window_ArrayAllChild_Callback, reinterpret_cast<LPARAM>(reaperarray)); //hwndString));
	//SetExtState(section, key, (const char*)hwndString, false);
}



BOOL CALLBACK JS_Window_ArrayAllTop_Callback(HWND hwnd, LPARAM lParam)
{
	using namespace Julian;
	double* reaperarray = reinterpret_cast<double*>(lParam);
	// Is space available in the reaper.array?
	uint32_t& currentArraySize = ((uint32_t*)(reaperarray))[0];
	if (currentArraySize < ((uint32_t*)reaperarray)[1])
	{
		reaperarray[currentArraySize + 1] = (double)((intptr_t)hwnd);
		currentArraySize++;
		return TRUE;
	}
	else
		return FALSE;
	/*
	char* hwndString = reinterpret_cast<char*>(strPtr);
	char temp[TEMP_LEN] = "";
	sprintf_s(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd); // Print with leading 0x so that Lua tonumber will automatically notice that it is hexadecimal.
	if (strlen(hwndString) + strlen(temp) < EXT_LEN - 1)
	{
		strcat(hwndString, temp);
		return TRUE;
	}
	else
		return FALSE;
	*/
}

void JS_Window_ArrayAllTop(double* reaperarray) //const char* section, const char* key) //char* buf, int buf_sz)
{
	using namespace Julian;
	//char hwndString[EXT_LEN] = "";
	EnumWindows(JS_Window_ArrayAllTop_Callback, reinterpret_cast<LPARAM>(reaperarray)); //hwndString));
	//SetExtState(section, key, (const char*)hwndString, false);
}



BOOL CALLBACK JS_MIDIEditor_ArrayAll_Callback_Child(HWND hwnd, LPARAM lParam)
{
	using namespace Julian;
	if (MIDIEditor_GetMode(hwnd) != -1) // Is MIDI editor?
	{
		double* reaperarray = reinterpret_cast<double*>(lParam);
		// Is space available in the reaper.array?
		uint32_t& currentArraySize = ((uint32_t*)(reaperarray))[0];
		if (currentArraySize < ((uint32_t*)reaperarray)[1])
		{
			reaperarray[currentArraySize + 1] = (double)((intptr_t)hwnd);
			currentArraySize++;
			return TRUE;
		}
		else
			return FALSE;
		/*
		char* hwndString = reinterpret_cast<char*>(lParam);
		char temp[TEMP_LEN] = "";
		sprintf_s(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd); // Print with leading 0x so that Lua tonumber will automatically notice that it is hexadecimal.
		if (strstr(hwndString, temp) == NULL) // Match with bounding 0x and comma
		{
			if ((strlen(hwndString) + strlen(temp)) < API_LEN - 1)
			{
				strcat(hwndString, temp);
			}
			else
				return FALSE;
		}
		*/
	}
	// else: not MIDI editor, and continue searching
	else
		return TRUE;
}

BOOL CALLBACK JS_MIDIEditor_ArrayAll_Callback_Top(HWND hwnd, LPARAM lParam)
{
	using namespace Julian;
	if (MIDIEditor_GetMode(hwnd) != -1) // Is MIDI editor?
	{
		double* reaperarray = reinterpret_cast<double*>(lParam);
		// Is space available in the reaper.array?
		uint32_t& currentArraySize = ((uint32_t*)(reaperarray))[0];
		if (currentArraySize < ((uint32_t*)reaperarray)[1])
		{
			reaperarray[currentArraySize + 1] = (double)((intptr_t)hwnd);
			currentArraySize++;
			return TRUE;
		}
		else
			return FALSE;
		/*
		char* hwndString = reinterpret_cast<char*>(lParam);
		char temp[TEMP_LEN] = "";
		sprintf_s(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd);
		if (strstr(hwndString, temp) == NULL) // Match with bounding 0x and comma
		{
			if ((strlen(hwndString) + strlen(temp)) < API_LEN - 1)
			{
				strcat(hwndString, temp);
			}
			else
				return FALSE;
		}
		*/
	}

	// Current window is not MIDI editor, so check if any child windows are. For example if docked in docker or main window.
	else 
	{
		EnumChildWindows(hwnd, JS_MIDIEditor_ArrayAll_Callback_Child, lParam);
		return TRUE;
	}
}

void JS_MIDIEditor_ArrayAll(double* reaperarray) // char* buf, int buf_sz)
{
	using namespace Julian;
	//char hwndString[API_LEN] = "";
	// To find docked editors, must also enumerate child windows.
	EnumWindows(JS_MIDIEditor_ArrayAll_Callback_Top, reinterpret_cast<LPARAM>(reaperarray));
	//strcpy_s(buf, buf_sz, hwndString);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The following functions are "List" versions of the "Array" functions, and return the list as an ExtState string


BOOL CALLBACK JS_Window_ListFind_Callback_Child(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows* s = reinterpret_cast<sEnumWindows*>(structPtr);
	int len = GetWindowText(hwnd, s->temp, s->tempLen);
	s->temp[s->tempLen - 1] = '\0'; // Make sure that loooong titles are properly terminated.
	
	// FindWindow is case-insensitive, so this implementation is too. Convert to lowercase.
	for (int i = 0; (s->temp[i] != '\0') && (i < len); i++) s->temp[i] = (char)tolower(s->temp[i]);

	// If exact, match entire title, otherwise substring
	if (	 (s->exact  && (strcmp(s->temp, s->target) == 0))
		|| (!(s->exact) && (strstr(s->temp, s->target) != NULL)))
	{
		// Convert pointer to string (leaving two spaces for 0x, and add comma separator)
		snprintf(s->temp, s->tempLen - 1, "0x%llX,", (unsigned long long int)hwnd);
		// Concatenate to hwndString
		if (strlen(s->hwndString) + strlen(s->temp) < s->hwndLen - 1)
		{
			strcat(s->hwndString, s->temp);
		}
		else
			return FALSE;
	}
	return TRUE;
}

BOOL CALLBACK JS_Window_ListFind_Callback_Top(HWND hwnd, LPARAM structPtr)
{
	using namespace Julian;
	sEnumWindows* s = reinterpret_cast<sEnumWindows*>(structPtr);
	int len = GetWindowText(hwnd, s->temp, s->tempLen);
	s->temp[s->tempLen - 1] = '\0'; // Make sure that loooong titles are properly terminated.
									// FindWindow is case-insensitive, so this implementation is too. Convert to lowercase.
	for (int i = 0; (s->temp[i] != '\0') && (i < len); i++) s->temp[i] = (char)tolower(s->temp[i]);
	// If exact, match entire title, otherwise substring
	if (	 (s->exact  && (strcmp(s->temp, s->target) == 0))
		|| (!(s->exact) && (strstr(s->temp, s->target) != NULL)))
	{
		// Convert pointer to string (leaving two spaces for 0x, and add comma separator)
		snprintf(s->temp, s->tempLen - 1, "0x%llX,", (unsigned long long int)hwnd);
		// Concatenate to hwndString
		if (strlen(s->hwndString) + strlen(s->temp) < s->hwndLen - 1)
		{
			strcat(s->hwndString, s->temp);
		}
		else
			return FALSE;
	}
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
void JS_Window_ListFind(const char* title, bool exact, const char* section, const char* key)
{
	using namespace Julian;

	// FindWindow is case-insensitive, so this implementation is too. 
	// Must first convert title to lowercase:
	char titleLower[API_LEN];
	for (int i = 0; i < API_LEN; i++)
	{
		if (title[i] == '\0') { titleLower[i] = '\0'; break; }
		else				  { titleLower[i] = (char)tolower(title[i]); }
	}
	titleLower[API_LEN - 1] = '\0'; // Make sure that loooong titles are properly terminated.
									// To communicate with callback functions, use an sEnumWindows:
	char temp[API_LEN] = ""; // Will temporarily store pointer strings, as well as titles.
	char hwndString[EXT_LEN] = ""; // Concatenate all pointer strings into this long string.
	sEnumWindows e{ titleLower, exact, temp, sizeof(temp), NULL, hwndString, sizeof(hwndString), NULL }; // All the info that will be passed to the Enum callback functions.

	EnumWindows(JS_Window_ListFind_Callback_Top, reinterpret_cast<LPARAM>(&e));

	SetExtState(section, key, (const char*)hwndString, false);
}



BOOL CALLBACK JS_Window_ListAllChild_Callback(HWND hwnd, LPARAM strPtr)
{
	using namespace Julian;
	char* hwndString = reinterpret_cast<char*>(strPtr);
	char temp[TEMP_LEN] = "";
	snprintf(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd); // Print with leading 0x so that Lua tonumber will automatically notice that it is hexadecimal.
	if (strlen(hwndString) + strlen(temp) < EXT_LEN - 1)
	{
		strcat(hwndString, temp);
		return TRUE;
	}
	else
		return FALSE;
}

void JS_Window_ListAllChild(void* parentHWND, const char* section, const char* key) //char* buf, int buf_sz)
{
	using namespace Julian;
	HWND hwnd = (HWND)parentHWND;
	char hwndString[EXT_LEN] = "";
	EnumChildWindows(hwnd, JS_Window_ListAllChild_Callback, reinterpret_cast<LPARAM>(hwndString));
	SetExtState(section, key, (const char*)hwndString, false);
}



BOOL CALLBACK JS_Window_ListAllTop_Callback(HWND hwnd, LPARAM strPtr)
{
	using namespace Julian;
	char* hwndString = reinterpret_cast<char*>(strPtr);
	char temp[TEMP_LEN] = "";
	snprintf(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd); // Print with leading 0x so that Lua tonumber will automatically notice that it is hexadecimal.
	if (strlen(hwndString) + strlen(temp) < EXT_LEN - 1)
	{
		strcat(hwndString, temp);
		return TRUE;
	}
	else
		return FALSE;
}

void JS_Window_ListAllTop(const char* section, const char* key) //char* buf, int buf_sz)
{
	using namespace Julian;
	char hwndString[EXT_LEN] = "";
	EnumWindows(JS_Window_ListAllTop_Callback, reinterpret_cast<LPARAM>(hwndString));
	SetExtState(section, key, (const char*)hwndString, false);
}



BOOL CALLBACK JS_MIDIEditor_ListAll_Callback_Child(HWND hwnd, LPARAM lParam)
{
	using namespace Julian;
	if (MIDIEditor_GetMode(hwnd) != -1) // Is MIDI editor?
	{
		char* hwndString = reinterpret_cast<char*>(lParam);
		char temp[TEMP_LEN] = "";
		snprintf(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd); // Print with leading 0x so that Lua tonumber will automatically notice that it is hexadecimal.
		if (strstr(hwndString, temp) == NULL) // Match with bounding 0x and comma
		{
			if ((strlen(hwndString) + strlen(temp)) < API_LEN - 1)
			{
				strcat(hwndString, temp);
			}
			else
				return FALSE;
		}
	}
	return TRUE; // Always search further, unless hwndString is getting too long
}

BOOL CALLBACK JS_MIDIEditor_ListAll_Callback_Top(HWND hwnd, LPARAM lParam)
{
	using namespace Julian;
	if (MIDIEditor_GetMode(hwnd) != -1) // Is MIDI editor?
	{
		char* hwndString = reinterpret_cast<char*>(lParam);
		char temp[TEMP_LEN] = "";
		snprintf(temp, TEMP_LEN - 1, "0x%llX,", (unsigned long long int)hwnd);
		if (strstr(hwndString, temp) == NULL) // Match with bounding 0x and comma
		{
			if ((strlen(hwndString) + strlen(temp)) < API_LEN - 1)
			{
				strcat(hwndString, temp);
			}
			else
				return FALSE;
		}
	}
	else // Check if any child windows are MIDI editor. For example if docked in docker or main window.
	{
		EnumChildWindows(hwnd, JS_MIDIEditor_ListAll_Callback_Child, lParam);
	}
	return TRUE; // Always search further, unless hwndString is getting too long
}

void JS_MIDIEditor_ListAll(char* buf, int buf_sz)
{
	using namespace Julian;
	char hwndString[API_LEN] = "";
	// To find docked editors, must also enumerate child windows.
	EnumWindows(JS_MIDIEditor_ListAll_Callback_Top, reinterpret_cast<LPARAM>(hwndString));
	strncpy(buf, hwndString, buf_sz);
	buf[buf_sz - 1] = '\0';
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
#ifdef _WIN32
	bool OK = false;
	if (IsWindow(windowHWND))
	{
		if (SetWindowLongPtr(windowHWND, GWL_EXSTYLE, GetWindowLongPtr(windowHWND, GWL_EXSTYLE) | WS_EX_LAYERED))
		{
			if (strchr(mode, 'A'))
			{
				if (SetLayeredWindowAttributes(windowHWND, 0, (BYTE)(value * 255), LWA_ALPHA))
					OK = true;
			}
			else
			{
				UINT v = (UINT)value;
				if (SetLayeredWindowAttributes(windowHWND, (COLORREF)(((v & 0xFF0000) >> 16) | (v & 0x00FF00) | ((v & 0x0000FF) << 16)), 0, LWA_COLORKEY))
					OK = true;
			}
		}
	}
	return OK;
#elif __linux__
	if (strchr(mode, 'C') || (!IsWindow(windowHWND)))
		return false;
	else
	{
		GdkWindow* w = (GdkWindow*)windowHWND->m_oswindow;
		gdk_window_set_opacity(w, value);
		return true;
	}
#elif __APPLE__
	if (strchr(mode, 'C') || (!IsWindow(windowHWND)))
		return false;
	else
	{
		JS_Window_SetOpacity_ObjC((void*)windowHWND, value);
		return true;
	}
#endif
}


bool JS_Window_SetTitle(void* windowHWND, const char* title)
{
	return !!SetWindowText((HWND)windowHWND, title);
}

void JS_Window_GetTitle(void* windowHWND, char* buf, int buf_sz)
{
	GetWindowText((HWND)windowHWND, buf, buf_sz);
}

#ifndef __APPLE__
void JS_Window_GetClassName(HWND windowHWND, char* buf, int buf_sz)
{
	GetClassName(windowHWND, buf, buf_sz);
}
#endif

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




bool JS_WindowMessage_ListIntercepts(void* windowHWND, char* buf, int buf_sz)
{
	using namespace Julian;
	buf[0] = '\0';
	
	if (mapWindowToData.count((HWND)windowHWND))
	{
		auto& messages = mapWindowToData[(HWND)windowHWND].messages;
		for (const auto& it : messages)
		{
			if (strlen(buf) < (UINT)buf_sz - 32)
			{
				if (mapMsgToWM_.count(it.first))
					sprintf(strchr(buf, 0), "%s:", mapMsgToWM_[it.first].c_str());
				else
					sprintf(strchr(buf, 0), "0x%04X:", it.first);
				if ((it.second).passthrough)
					strcat(buf, "passthrough,\0");
				else
					strcat(buf, "block,\0");
			}
			else
				return false;
		}
		
		char* lastComma{ strrchr(buf, ',') }; // Remove final comma
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
	return LICE_CreateBitmap((BOOL)isSysBitmap, width, height); // Always use mode = 1 for SysBitmap, so that can BitBlt to/from screen like HDC.
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

bool JS_Window_RemoveXPStyle(void* windowHWND, bool remove)
{
	return !!RemoveXPStyle((HWND)windowHWND, (BOOL)remove);
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


///////////////////////////////////////////////////////////////////////

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
	if (src == nullptr || numframes<1 || numchans < 1 || samplerate < 1.0)
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
