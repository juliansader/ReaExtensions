#pragma once

namespace Julian
{
	using namespace std; 

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

	// Bitmaps can use up lots of RAM, so to ensure that all bitmaps are destroyed when REAPER exits,
	//		all active bitmaps are stored here, and will be destroyed by the REAPER_PLUGIN_ENTRYPOINT function.
	set<LICE_IBitmap*> LICEBitmaps;


	set<HWND>	foundHWNDs;
	char		findTitle[1024]; // Title text that must be matched
	bool		findExact;  // Match exactly, or substring?
	char		tempTitle[1024];   // Temprarily store window titles returned by GetWindowText

	HWND		hwnd;  // HWND that was found (for single-window versions of functions)

	char*		hwndString; // List of all matching HWNDs (for List version of functions)
	unsigned int	hwndLen;
	double*		reaperarray; // Array of all matching HWNDs (for Array version of functions), in reaper.array format (i.e. with alloc size and used size in first entry)

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
	/*
	struct sEnumWindows
	{
		const char* target; // Title text that must be matched
		bool		exact;  // Match exactly, or substring?
		char*		temp;   // Temprarily store window titles returned by GetWindowText
		unsigned int	tempLen;
		HWND		hwnd;  // HWND that was found (for single-window versions of functions)
		char*		hwndString; // List of all matching HWNDs (for List version of functions)
		unsigned int	hwndLen;
		double*		reaperarray; // Array of all matching HWNDs (for Array version of functions), in reaper.array format (i.e. with alloc size and used size in first entry)
	};
	*/

	// Error codes for WindowMessage_Intercept
	constexpr int ERR_ALREADY_INTERCEPTED = 0;
	constexpr int ERR_NOT_WINDOW = -1;
	constexpr int ERR_PARSING = -2;
	constexpr int ERR_ORIGPROC = -3;


	// This struct is used to store the data of intercepted messages.
	//		In addition to the standard wParam and lParam, a unique timestamp is added.
	struct sMsgData
	{
		bool passthrough;
		double time;
		WPARAM wParam;
		LPARAM lParam;
	};

	// This struct and map store the data of each HWND that is being intercepted.
	// (Each window can only be intercepted by one script at a time.)
	// For each window, three bitfields summarize the up/down states of most of the keyboard. 
	// UPDATE: Apparently, REAPER's windows don't receive keystrokes via the message queue, and the keyboard can therefore not be intercepted.  
	struct sWindowData
	{
		WNDPROC origProc;
		std::map<UINT, sMsgData> messages;
		//int keysBitfieldBACKtoHOME	= 0; // Virtual key codes VK_BACK (backspace) to VK_HOME
		//int keysBitfieldAtoSLEEP	= 0;
		//int keysBitfieldLEFTto9		= 0;
	};
	const bool BLOCK = false;

	////////////////////////////////////////////////////////////////////////////
	// THE MAIN MAP FOR ALL INTERCEPTS
	// Each window that is being intercepted, will be mapped to its data struct.
	std::map <HWND, sWindowData> mapWindowToData;

	// This map contains all the WM_ messages in swell-types.h. These can be assumed to be valid cross-platform.
	std::map<std::string, UINT> mapWM_toMsg
	{
		pair<std::string, UINT>("WM_CREATE", 0x0001),
		pair<std::string, UINT>("WM_DESTROY", 0x0002),
		pair<std::string, UINT>("WM_MOVE", 0x0003),
		pair<std::string, UINT>("WM_SIZE", 0x0005),
		pair<std::string, UINT>("WM_ACTIVATE", 0x0006),
		//pair<std::string, UINT>("WM_SETREDRAW", 0x000B), // Not implemented cross-platform
		//pair<std::string, UINT>("WM_SETTEXT", 0x000C),
		pair<std::string, UINT>("WM_PAINT", 0x000F),
		pair<std::string, UINT>("WM_CLOSE", 0x0010),
		pair<std::string, UINT>("WM_ERASEBKGND", 0x0014),
		pair<std::string, UINT>("WM_SHOWWINDOW", 0x0018),
		pair<std::string, UINT>("WM_ACTIVATEAPP", 0x001C),
		pair<std::string, UINT>("WM_SETCURSOR", 0x0020),
		pair<std::string, UINT>("WM_MOUSEACTIVATE", 0x0021),
		pair<std::string, UINT>("WM_GETMINMAXINFO", 0x0024),
		pair<std::string, UINT>("WM_DRAWITEM", 0x002B),
		pair<std::string, UINT>("WM_SETFONT", 0x0030),
		pair<std::string, UINT>("WM_GETFONT", 0x0031),
		//pair<std::string, UINT>("WM_GETOBJECT", 0x003D),
		pair<std::string, UINT>("WM_COPYDATA", 0x004A),
		pair<std::string, UINT>("WM_NOTIFY", 0x004E),
		pair<std::string, UINT>("WM_CONTEXTMENU", 0x007B),
		pair<std::string, UINT>("WM_STYLECHANGED", 0x007D),
		pair<std::string, UINT>("WM_DISPLAYCHANGE", 0x007E),
		pair<std::string, UINT>("WM_NCDESTROY", 0x0082),
		pair<std::string, UINT>("WM_NCCALCSIZE", 0x0083),
		pair<std::string, UINT>("WM_NCHITTEST", 0x0084),
		pair<std::string, UINT>("WM_NCPAINT", 0x0085),
		pair<std::string, UINT>("WM_NCMOUSEMOVE", 0x00A0),
		pair<std::string, UINT>("WM_NCLBUTTONDOWN", 0x00A1),
		pair<std::string, UINT>("WM_NCLBUTTONUP", 0x00A2),
		pair<std::string, UINT>("WM_NCLBUTTONDBLCLK", 0x00A3),
		pair<std::string, UINT>("WM_NCRBUTTONDOWN", 0x00A4),
		pair<std::string, UINT>("WM_NCRBUTTONUP", 0x00A5),
		pair<std::string, UINT>("WM_NCRBUTTONDBLCLK", 0x00A6),
		pair<std::string, UINT>("WM_NCMBUTTONDOWN", 0x00A7),
		pair<std::string, UINT>("WM_NCMBUTTONUP", 0x00A8),
		pair<std::string, UINT>("WM_NCMBUTTONDBLCLK", 0x00A9),
		pair<std::string, UINT>("WM_KEYFIRST", 0x0100),
		pair<std::string, UINT>("WM_KEYDOWN", 0x0100),
		pair<std::string, UINT>("WM_KEYUP", 0x0101),
		pair<std::string, UINT>("WM_CHAR", 0x0102),
		pair<std::string, UINT>("WM_DEADCHAR", 0x0103),
		pair<std::string, UINT>("WM_SYSKEYDOWN", 0x0104),
		pair<std::string, UINT>("WM_SYSKEYUP", 0x0105),
		pair<std::string, UINT>("WM_SYSCHAR", 0x0106),
		pair<std::string, UINT>("WM_SYSDEADCHAR", 0x0107),
		pair<std::string, UINT>("WM_KEYLAST", 0x0108),
		pair<std::string, UINT>("WM_INITDIALOG", 0x0110),
		pair<std::string, UINT>("WM_COMMAND", 0x0111),
		pair<std::string, UINT>("WM_SYSCOMMAND", 0x0112),
		pair<std::string, UINT>("SC_CLOSE", 0xF060),
		pair<std::string, UINT>("WM_TIMER", 0x0113),
		pair<std::string, UINT>("WM_HSCROLL", 0x0114),
		pair<std::string, UINT>("WM_VSCROLL", 0x0115),
		pair<std::string, UINT>("WM_INITMENUPOPUP", 0x0117),
		pair<std::string, UINT>("WM_GESTURE", 0x0119),
		pair<std::string, UINT>("WM_MOUSEFIRST", 0x0200),
		pair<std::string, UINT>("WM_MOUSEMOVE", 0x0200),
		pair<std::string, UINT>("WM_LBUTTONDOWN", 0x0201),
		pair<std::string, UINT>("WM_LBUTTONUP", 0x0202),
		pair<std::string, UINT>("WM_LBUTTONDBLCLK", 0x0203),
		pair<std::string, UINT>("WM_RBUTTONDOWN", 0x0204),
		pair<std::string, UINT>("WM_RBUTTONUP", 0x0205),
		pair<std::string, UINT>("WM_RBUTTONDBLCLK", 0x0206),
		pair<std::string, UINT>("WM_MBUTTONDOWN", 0x0207),
		pair<std::string, UINT>("WM_MBUTTONUP", 0x0208),
		pair<std::string, UINT>("WM_MBUTTONDBLCLK", 0x0209),
		pair<std::string, UINT>("WM_MOUSEWHEEL", 0x020A),
		pair<std::string, UINT>("WM_MOUSEHWHEEL", 0x020E),
		pair<std::string, UINT>("WM_MOUSELAST", 0x020A),
		pair<std::string, UINT>("WM_CAPTURECHANGED", 0x0215),
		pair<std::string, UINT>("WM_DROPFILES", 0x0233),
		pair<std::string, UINT>("WM_USER", 0x0400)
	};

	// Reverse map of mapWM_toMsg. Will be constructed from mapWM_toMsg in REAPER_PLUGIN_ENTRYPOINT.
	std::map<UINT, std::string> mapMsgToWM_;
}
