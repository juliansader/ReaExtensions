#pragma once

namespace Julian
{
	using namespace std; 

	HINSTANCE ReaScriptAPI_Instance;
	double REAPER_VERSION = 0;

	constexpr bool ENV32BIT = (sizeof(void*) == 4);
	constexpr int TEMP_LEN  = 65;	// For temporary storage of pointer strings.
	constexpr int API_LEN   = 1024;	// Maximum length of strings passed between Lua and API (when not using NeedXXX suffix).
	constexpr int EXT_LEN   = 16000;	// What is the maximum length of ExtState strings? 
	constexpr int MAX_CHARS_PER_HWND = 19*4; // * (MB_LEN_MAX); // Maximum possble chars used by each HWND when sprintf'ing to string in format "0x%llx,".
	constexpr int MAX_CHARS_PER_INT = (1 + sizeof(int) * 3) * 4; // Maximum possble chars used by each int when sprintf'ing to string in format "%u,".

	bool GOTREALLOC;

	// API functions that return strings (with const char* return value), will return pointers to this char array.
	// REAPER immediately converts this array into Lua strings, 
	char longString[EXT_LEN];

	///////////////////////////////////////////////////////////////////////////////////////////////
	// GDI and LICE stuff
	// Store all created HDCs and IBitmaps so that can check whether these exist, when 

	// Bitmaps can use up lots of RAM, so to ensure that all bitmaps are destroyed when REAPER exits,
	//		all active bitmaps are stored here, and will be destroyed by the REAPER_PLUGIN_ENTRYPOINT function.
	LICE_IBitmap* 			compositeCanvas;
	map<LICE_IBitmap*, HDC> mLICEBitmaps;
	set<pair<HWND,HDC>> 	GDIHDCs;

	// To avoid having to re-load a cursor from file every time that a script is executed,
	//		the HCURSURS will be stored in this map.
	// WDL/swell doesn't provide a cross-platform function to remove cursors from memory,
	//		so if a REAPER session is run for many days and a script that loads a cursor from file is executed thousands of times,
	//		memory usage may climb. 
	map<std::string, HCURSOR> mapFileToMouseCursor;


	//////////////////////////////////////////////////////
	// DIRECT MEMORY ACCESS
	//Store the size of allocated memory for Mem functions
	map<void*, int> mapMallocToSize;


	///////////////////////////////////////////////////////////////
	// KEYBOARD INTERCEPT
	accelerator_register_t sAccelerator{ JS_VKeys_Callback, true };


	/////////////////////////////////////////////
	// WINDOW CLASS
	// Remember to initialize the hInstance!
#ifdef _WIN32skipthis // can't get wchar to work with class names yet, so skip this and use plain char
	std::map<std::string, wchar_t*> mapClassNames;
#else
	std::map<std::string, char*> mapClassNames;
#endif


	/////////////////////////////////////////////
	// GDI Objects
	std::set<HGDIOBJ> setGDIObjects;

	//////////////////////////////////////////////////////////////////////////////////////
	// Find functions: Some global variables that will be used when searching for windows.
	// Since these variables are global, all functions and their callbacks can access the variables without having to pass them via lParams.
	/*namespace find
	{
		set<HWND>	foundHWNDs;
		char		findTitle[1024]; // Title text that must be matched
		bool		findExact;  // Match exactly, or substring?
		char		tempFindTitle[1024];   // Temprarily store window titles returned by GetWindowText

		HWND		foundHwnd;  // HWND that was found (for single-window versions of functions)

		char*		hwndString; // List of all matching HWNDs (for List version of functions)
		unsigned int	hwndLen;
		double*		reaperarray; // Array of all matching HWNDs (for Array version of functions), in reaper.array format (i.e. with alloc size and used size in first entry)
	}*/

	// While windows are being enumerated, this struct stores the information
	//		such as the title text that must be matched, as well as the list of all matching window HWNDs.
	struct sEnumWindows
	{
		const char* target; // Title text that must be matched
		bool		exact;  // Match exactly, or substring?
		char*		temp;   // Temporarily store window titles returned by GetWindowText
		unsigned int	tempLen;
		set<HWND>*	foundHWNDs;  // HWND that was found (for single-window versions of functions)
		//double*		reaperarray; // Array of all matching HWNDs (for Array version of functions), in reaper.array format (i.e. with alloc size and used size in first entry)
	};


	///////////////////////////////////////////////////////////////////////////////////////
	// Window Message intercept stuff

	// Error codes for WindowMessage_Intercept
	constexpr int ERR_ALREADY_INTERCEPTED = 0;
	constexpr int ERR_NOT_WINDOW = -1;
	constexpr int ERR_PARSING = -2;
	constexpr int ERR_ORIG_WNDPROC = -3;
	constexpr int ERR_NOT_BITMAP = -4;
	constexpr int ERR_NOT_SYSBITMAP = -5;
	constexpr int ERR_WINDOW_HDC = -6;
	constexpr int ERR_NEW_WNDPROC = -7;
	constexpr int ERR_INVALIDATE = -8;

	// This struct is used to store the data of intercepted messages.
	//		In addition to the standard wParam and lParam, a unique timestamp is added.
	struct sMsgData
	{
		bool passthrough;
		double time;
		WPARAM wParam;
		LPARAM lParam;
	};
	/*
	// When posting a message that is being intercepted, store its info in this set,
	//		so that can be skipped.
	struct sPostedMsg
	{
		HWND hwnd;
		UINT uMsg;
		WPARAM wParam;
		LPARAM lParam;
	};
	bool operator==(const sPostedMsg& msg1, const sPostedMsg& msg2)
	{
		return ((msg1.wParam == msg2.wParam)
			&& (msg1.lParam == msg2.lParam)
			&& (msg1.uMsg == msg2.uMsg)
			&& (msg1.hwnd == msg2.hwnd));
	};
	std::multiset<sPostedMsg> setPostedMessages;
	*/

	// This struct is used to store the data of linked bitmaps for compositing.
	struct sBlitRects 
	{
		int dstx;
		int dsty;
		int dstw;
		int dsth;
		int srcx;
		int srcy;
		int srcw;
		int srch;
	};

	// This struct and map store the data of each HWND that is being intercepted.
	// (Each window can only be intercepted by one script at a time.)
	struct sWindowData
	{
		LONG_PTR origProc;
		std::map<UINT, sMsgData> mapMessages; // Most recent msg values received
		std::map<LICE_IBitmap*, sBlitRects> mapBitmaps; // bitmaps linked to this window for compositing
		RECT 	 mustInvalidRect; // On WindowsOS, the rect that must be invalidated in callback;  on Linux and macOS the rect that has *already* been invalidated in this paint cycle.
		RECT 	 doneInvalidRect;
		double	 lastTime;
		UINT_PTR timerID;
	};
	const bool BLOCK = false;

	// THE MAIN MAP FOR ALL INTERCEPTS
	// Each window that is being intercepted, will be mapped to its data struct.
	std::map <HWND, sWindowData> mapWindowData;

	struct sDelayData
	{
		double	minTime;
		double	maxTime;
		int		maxBitmaps;
	};
	std::map<HWND, sDelayData> mapDelayData;

	// This map contains all the WM_ messages in swell-types.h. These can be assumed to be valid cross-platform.
	std::map<std::string, UINT> mapWM_toMsg
	{
		pair<std::string, UINT>("WM_CREATE", WM_CREATE),
		pair<std::string, UINT>("WM_DESTROY", WM_DESTROY),
		pair<std::string, UINT>("WM_MOVE", WM_MOVE),
		pair<std::string, UINT>("WM_SIZE", WM_SIZE),
		pair<std::string, UINT>("WM_ACTIVATE", WM_ACTIVATE),
		//pair<std::string, UINT>("WM_SETREDRAW", 0x000B), // Not implemented cross-platform
		//pair<std::string, UINT>("WM_SETTEXT", 0x000C),
		pair<std::string, UINT>("WM_PAINT", WM_PAINT),
		pair<std::string, UINT>("WM_CLOSE", WM_CLOSE),
		pair<std::string, UINT>("WM_ERASEBKGND", WM_ERASEBKGND),
		pair<std::string, UINT>("WM_SHOWWINDOW", WM_SHOWWINDOW),
		pair<std::string, UINT>("WM_ACTIVATEAPP", WM_ACTIVATEAPP),
		pair<std::string, UINT>("WM_SETCURSOR", WM_SETCURSOR),
		pair<std::string, UINT>("WM_MOUSEACTIVATE", WM_MOUSEACTIVATE),
		pair<std::string, UINT>("WM_GETMINMAXINFO", WM_GETMINMAXINFO),
		pair<std::string, UINT>("WM_DRAWITEM", WM_DRAWITEM),
		pair<std::string, UINT>("WM_SETFONT", WM_SETFONT),
		pair<std::string, UINT>("WM_GETFONT", WM_GETFONT),
		//pair<std::string, UINT>("WM_GETOBJECT", 0x003D),
		pair<std::string, UINT>("WM_COPYDATA", WM_COPYDATA),
		pair<std::string, UINT>("WM_NOTIFY", WM_NOTIFY),
		pair<std::string, UINT>("WM_CONTEXTMENU", WM_CONTEXTMENU),
		pair<std::string, UINT>("WM_STYLECHANGED", WM_STYLECHANGED),
		pair<std::string, UINT>("WM_DISPLAYCHANGE", WM_DISPLAYCHANGE),
		pair<std::string, UINT>("WM_NCDESTROY", WM_NCDESTROY),
		pair<std::string, UINT>("WM_NCCALCSIZE", WM_NCCALCSIZE),
		pair<std::string, UINT>("WM_NCHITTEST", WM_NCHITTEST),
		pair<std::string, UINT>("WM_NCPAINT", WM_NCPAINT),
		pair<std::string, UINT>("WM_NCMOUSEMOVE", WM_NCMOUSEMOVE),
		pair<std::string, UINT>("WM_NCLBUTTONDOWN", WM_NCLBUTTONDOWN),
		pair<std::string, UINT>("WM_NCLBUTTONUP", WM_NCLBUTTONUP),
		pair<std::string, UINT>("WM_NCLBUTTONDBLCLK", WM_NCLBUTTONDBLCLK),
		pair<std::string, UINT>("WM_NCRBUTTONDOWN", WM_NCRBUTTONDOWN),
		pair<std::string, UINT>("WM_NCRBUTTONUP", WM_NCRBUTTONUP),
		pair<std::string, UINT>("WM_NCRBUTTONDBLCLK", WM_NCRBUTTONDBLCLK),
		pair<std::string, UINT>("WM_NCMBUTTONDOWN", WM_NCMBUTTONDOWN),
		pair<std::string, UINT>("WM_NCMBUTTONUP", WM_NCMBUTTONUP),
		pair<std::string, UINT>("WM_NCMBUTTONDBLCLK", WM_NCMBUTTONDBLCLK),
		pair<std::string, UINT>("WM_KEYFIRST", WM_KEYFIRST),
		pair<std::string, UINT>("WM_KEYDOWN", WM_KEYDOWN),
		pair<std::string, UINT>("WM_KEYUP", WM_KEYUP),
		pair<std::string, UINT>("WM_CHAR", WM_CHAR),
		pair<std::string, UINT>("WM_DEADCHAR", WM_DEADCHAR),
		pair<std::string, UINT>("WM_SYSKEYDOWN", WM_SYSKEYDOWN),
		pair<std::string, UINT>("WM_SYSKEYUP", WM_SYSKEYUP),
		pair<std::string, UINT>("WM_SYSCHAR", WM_SYSCHAR),
		pair<std::string, UINT>("WM_SYSDEADCHAR", WM_SYSDEADCHAR),
		pair<std::string, UINT>("WM_KEYLAST", WM_KEYLAST),
		pair<std::string, UINT>("WM_INITDIALOG", WM_INITDIALOG),
		pair<std::string, UINT>("WM_COMMAND", WM_COMMAND),
		pair<std::string, UINT>("WM_SYSCOMMAND", WM_SYSCOMMAND),
		pair<std::string, UINT>("SC_CLOSE", SC_CLOSE),
		pair<std::string, UINT>("WM_TIMER", WM_TIMER),
		pair<std::string, UINT>("WM_HSCROLL", WM_HSCROLL),
		pair<std::string, UINT>("WM_VSCROLL", WM_VSCROLL),
		pair<std::string, UINT>("WM_INITMENUPOPUP", WM_INITMENUPOPUP),
		pair<std::string, UINT>("WM_GESTURE", WM_GESTURE),
		pair<std::string, UINT>("WM_MOUSEFIRST", WM_MOUSEFIRST),
		pair<std::string, UINT>("WM_MOUSEMOVE", WM_MOUSEMOVE),
		pair<std::string, UINT>("WM_LBUTTONDOWN", WM_LBUTTONDOWN),
		pair<std::string, UINT>("WM_LBUTTONUP", WM_LBUTTONUP),
		pair<std::string, UINT>("WM_LBUTTONDBLCLK", WM_LBUTTONDBLCLK),
		pair<std::string, UINT>("WM_RBUTTONDOWN", WM_RBUTTONDOWN),
		pair<std::string, UINT>("WM_RBUTTONUP", WM_RBUTTONUP),
		pair<std::string, UINT>("WM_RBUTTONDBLCLK", WM_RBUTTONDBLCLK),
		pair<std::string, UINT>("WM_MBUTTONDOWN", WM_MBUTTONDOWN),
		pair<std::string, UINT>("WM_MBUTTONUP", WM_MBUTTONUP),
		pair<std::string, UINT>("WM_MBUTTONDBLCLK", WM_MBUTTONDBLCLK),
		pair<std::string, UINT>("WM_MOUSEWHEEL", WM_MOUSEWHEEL),
		pair<std::string, UINT>("WM_MOUSEHWHEEL", WM_MOUSEHWHEEL),
		pair<std::string, UINT>("WM_MOUSELAST", WM_MOUSELAST),
		pair<std::string, UINT>("WM_CAPTURECHANGED", WM_CAPTURECHANGED),
		pair<std::string, UINT>("WM_DROPFILES", WM_DROPFILES),
		pair<std::string, UINT>("WM_USER", WM_USER),
		
		pair<std::string, UINT>("BM_GETCHECK", BM_GETCHECK),
		pair<std::string, UINT>("BM_SETCHECK", BM_SETCHECK),
		pair<std::string, UINT>("BM_GETIMAGE", BM_GETIMAGE),
		pair<std::string, UINT>("BM_SETIMAGE", BM_SETIMAGE),

		pair<std::string, UINT>("CB_ADDSTRING", CB_ADDSTRING),
		pair<std::string, UINT>("CB_DELETESTRING", CB_DELETESTRING),
		pair<std::string, UINT>("CB_GETCOUNT", CB_GETCOUNT),
		pair<std::string, UINT>("CB_GETCURSEL", CB_GETCURSEL),
		pair<std::string, UINT>("CB_GETLBTEXT", CB_GETLBTEXT),
		pair<std::string, UINT>("CB_GETLBTEXTLEN", CB_GETLBTEXTLEN),
		pair<std::string, UINT>("CB_INSERTSTRING", CB_INSERTSTRING),
		pair<std::string, UINT>("CB_RESETCONTENT", CB_RESETCONTENT),
		pair<std::string, UINT>("CB_FINDSTRING", CB_FINDSTRING),
		pair<std::string, UINT>("CB_SETCURSEL", CB_SETCURSEL),
		pair<std::string, UINT>("CB_GETITEMDATA", CB_GETITEMDATA),
		pair<std::string, UINT>("CB_SETITEMDATA", CB_SETITEMDATA),
		pair<std::string, UINT>("CB_FINDSTRINGEXACT", CB_FINDSTRINGEXACT),
		pair<std::string, UINT>("CB_INITSTORAGE", CB_INITSTORAGE)

	};

	// Reverse map of mapWM_toMsg. Will be constructed from mapWM_toMsg in REAPER_PLUGIN_ENTRYPOINT.
	std::map<UINT, std::string> mapMsgToWM_;
}
