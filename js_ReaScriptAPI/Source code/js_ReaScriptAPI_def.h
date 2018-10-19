#pragma once

#include "js_ReaScriptAPI.h"

// The macro and struct in this file are copied from SWS's ReaScript.cpp.)

// Macro to construct a comma-separated list of all the variants of a function name that are required for plugin_register(), and in the order required by the APIdef struct in which these variants are stored.
// APIFUNC(funcName) becomes (void*)funcName, "funcName", (void*)__vararg_funcName, "APIvararg_funcName", "API_funcName", "APIdef_funcName"
#define APIFUNC(x)  (void*)x,#x,(void*)__vararg_ ## x,"APIvararg_" #x "","API_" #x "","APIdef_" #x ""


// Struct to store info such as function name and help text for each function that the extensions intends to expose as API.
// This info will be used by REAPER's plugin_register functions to register the functions. 
// NOTE: REAPER requires the return values, paramaters and help text to be one long \0-separated string. 
//		The parm_names, parm_types and help field will there have to be concatenated before registration, and stored in the defstring field.
struct APIdef
{
	void* func; // pointer to the function that other extensions use
	const char* func_name;
	void* func_vararg; // pointer to the wrapper function that ReaScript API calls
	const char* regkey_vararg; // "APIvararg_funcName" - for 
	const char* regkey_func; // "API_funcName"
	const char* regkey_def; // "APIdef_funcName" 
	const char* ret_val; // return value type, as string
	const char* parm_types;
	const char* parm_names;
	const char* help;
	char* defstring; // For APIdef... Will be constructed and assigned while registering function

};


///////////////////////////////////////////////////////////////////////////////
//
// Add functions you want to export in the table below (+ related #include on
// top). To distinguish SWS and native functions, make sure function names have
// a prefix like "SWS_", "FNG_", etc.
// Your functions must be made dumb-proof, they must not trust any parameter.
// However, your do not need to ValidatePr() REAPER pointer parameters (such as 
// MediaTrack*, MediaItem*, etc): REAPER does this for you when a script runs
// an exported function.
// REAPER pointer parameters are validated against the prior ReaProject* param,
// (or the current project if absent/NULL). For ex., in the following function:
// void bla(ReaProject* p1, MediaTrack* t1, ReaProject* p2, MediaItem* i2),
// t1 will be validated against p1, but i2 will be validated against p2.
// Your must interpret NULL ReaProject* params as "the current project".
//
// When defining/documenting API function parameters:
//  - if a (char*,int) pair is encountered, name them buf, buf_sz
//  - if a (const char*,int) pair is encountered, buf, buf_sz as well
//  - if a lone basicType *, use varNameOut or varNameIn or
//    varNameInOptional (if last parameter(s))
// At the moment (REAPER v5pre6) the supported parameter types are:
//  - int, int*, bool, bool*, double, double*, char*, const char*
//  - AnyStructOrClass* (handled as an opaque pointer)
// At the moment (REAPER v5pre6) the supported return types are:
//  - int, bool, double, const char*
//  - AnyStructOrClass* (handled as an opaque pointer)
//
///////////////////////////////////////////////////////////////////////////////
/*
Julian remarks:

Under the hood, REAPER converts all ReaScript API functions to this standard format before calling the C++ function:
	void* func(void** arglist, int numparams)
In the REAPER IDE, the user doesn't need to type such a strange-looking vararg function.  Instead, the user can type standard Lua functions with

In the case of int, bool and void* parameters and return values, these values are stored in the void* variable itself.  

In the case of doubles, the sizeof(double) is larger than sizeof(void*), so cannot be stored in a void* parameter.  
		Instead, the void* argument is a pointer to the actual double value: *(double*)arglist[1]
		Similarly, a double return value should be stored in the *
		If you want to return a double value, don't return it as the C++ function's return value. Instead, include a "double* myVariableOut" in the arguments,


a more helpful version is presented
	to the user the user uses sees hepthe names and types of the parameters and return values that the user sees in the IDE are registered by plugin_register(APIdef_...).

So all functions must either be in this format, or get a wrapper function such as this:
		static void* __vararg_TEST_Window_GetRect(void** arglist, int numparms)
		{
			return (void*)(INT_PTR)TEST_Window_GetRect((void*)arglist[0], (int*)arglist[1], (int*)arglist[2], (int*)arglist[3], (int*)arglist[4]);
		}

*/

APIdef aAPIdefs[] =
{
	{ APIFUNC(JS_ReaScriptAPI_Version), "void", "double*", "versionOut", "Returns the version of the js_ReaScriptAPI extension.", },

	{ APIFUNC(JS_Window_GetRect), "bool", "void*,int*,int*,int*,int*", "windowHWND,leftOut,topOut,rightOut,bottomOut", "Retrieves the coordinates of the bounding rectangle of the specified window. The dimensions are given in screen coordinates relative to the upper-left corner of the screen.\nNOTE: The pixel at (right, bottom) lies immediately outside the rectangle.", },
	{ APIFUNC(JS_Window_ScreenToClient), "void", "void*,int,int,int*,int*", "windowHWND,x,y,xOut,yOut", "Converts the screen coordinates of a specified point on the screen to client-area coordinates.", },
	{ APIFUNC(JS_Window_ClientToScreen), "void", "void*,int,int,int*,int*", "windowHWND,x,y,xOut,yOut", "Converts the client-area coordinates of a specified point to screen coordinates.", },
	{ APIFUNC(JS_Window_GetClientRect), "bool", "void*,int*,int*,int*,int*", "windowHWND,leftOut,topOut,rightOut,bottomOut", "Retrieves the coordinates of the client area rectangle of the specified window. The dimensions are given in screen coordinates relative to the upper-left corner of the screen.\nNOTE 1: Unlike the C++ function GetClientRect, this function returns the actual coordinates, not the width and height.\nNOTE 2: The pixel at (right, bottom) lies immediately outside the rectangle.", },

	{ APIFUNC(JS_Window_FromPoint), "void*", "int,int", "x,y", "Retrieves a HWND to the window that contains the specified point.", },
	{ APIFUNC(JS_Window_GetParent), "void*", "void*", "windowHWND", "Retrieves a HWND to the specified window's parent or owner.\nReturns NULL if the window is unowned or if the function otherwise fails.", },
	{ APIFUNC(JS_Window_IsChild), "bool", "void*,void*", "parentHWND,childHWND", "Determines whether a window is a child window or descendant window of a specified parent window.", },
	{ APIFUNC(JS_Window_GetRelated), "void*", "void*,const char*", "windowHWND,relation", "Retrieves a handle to a window that has the specified relationship (Z-Order or owner) to the specified window.\nrelation: \"LAST\", \"NEXT\", \"PREV\", \"OWNER\" or \"CHILD\".\n(Refer to documentation for Win32 C++ function GetWindow.)", },

	{ APIFUNC(JS_Window_SetFocus), "void", "void*", "windowHWND", "Sets the keyboard focus to the specified window.", },
	{ APIFUNC(JS_Window_GetFocus), "void*", "", "", "Retrieves a HWND to the window that has the keyboard focus, if the window is attached to the calling thread's message queue.", },
	{ APIFUNC(JS_Window_SetForeground), "void", "void*", "windowHWND", "Brings the specified window into the foreground, activates the window, and directs keyboard input to it.", },
	{ APIFUNC(JS_Window_GetForeground), "void*", "", "", "Retrieves a HWND to the foreground window (the window with which the user is currently working).", },

	{ APIFUNC(JS_Window_Enable), "void", "void*,bool", "windowHWND,enable", "Enables or disables mouse and keyboard input to the specified window or control.", },
	{ APIFUNC(JS_Window_Destroy), "void", "void*", "windowHWND", "Destroys the specified window.", },
	{ APIFUNC(JS_Window_Show), "void", "void*,const char*", "windowHWND,state", "Sets the specified window's show state.\n\nParameters:\n * state: Either \"SHOW\", \"SHOWNA\", \"SHOWMINIMIZED\",  or \"HIDE\".", },
	{ APIFUNC(JS_Window_IsVisible), "bool", "void*", "windowHWND", "Determines the visibility state of the window.", },
	{ APIFUNC(JS_Window_IsWindow), "bool", "void*", "windowHWND", "Determines whether the specified window handle identifies an existing window.", },

	//{ APIFUNC(JS_Window_FindEx), "void*", "void*,void*,const char*,const char*", "parentHWND,childHWND,className,title", "Returns a handle to a child window whose class and title match the specified strings.\n\nParameters: * childWindow: The function searches child windows, beginning with the window *after* the specified child window. If childHWND is equal to parentHWND, the search begins with the first child window of parentHWND.\n * title: An empty string, \"\", will match all windows. (Search is not case sensitive.)\n\nWARNING: May not be fully implemented yet in MacOS and Linux.", },
	{ APIFUNC(JS_Window_Find), "void*", "const char*,bool", "title,exact", "Returns a HWND to the top-level window whose title matches the specified string. This function does not search child window, and is not case sensitive.\n\nParameters:\n * exact: Match entire title exactly, or match substring of title.", },
	{ APIFUNC(JS_Window_FindChild), "void*", "void*,const char*,bool", "parentHWND,title,exact", "Returns a HWND to the child window whose title matches the specified string.\n\nParameters:\n * exact: Match entire title exactly, or match substring of title.", },
	{ APIFUNC(JS_Window_ArrayAllChild), "void", "void*,void*", "parentHWND,reaperarray", "Returns all child windows of the specified parent.\n\nThe addresses are stored in the provided reaper.array, and can be converted to REAPER objects (HWNDs) by the function JS_Window_HandleFromAddress.", },
	{ APIFUNC(JS_Window_ArrayAllTop), "void", "void*", "reaperarray", "Returns all top-level windows.\n\nThe addresses are stored in the provided reaper.array, and can be converted to REAPER objects (HWNDs) by the function JS_Window_HandleFromAddress.", },
	{ APIFUNC(JS_Window_ArrayFind), "void", "const char*,bool,void*", "title,exact,reaperarray", "Returns all windows, whether top-level or child, whose titles match the specified string.\n\nThe addresses are stored in the provided reaper.array, and can be converted to REAPER objects (HWNDs) by the function JS_Window_HandleFromAddress.\n\nParameters: * exact: Match entire title exactly, or match substring of title.", },
	{ APIFUNC(JS_Window_ListAllChild), "void", "void*,const char*,const char*", "parentHWND,section,key", "Returns a list of HWNDs of all child windows of the specified parent.\nThe list is formatted as a comma-separated (and terminated) string of hexadecimal values.\nEach value is an address that can be converted to a HWND by the function Window_HandleFromAddress.\n\nParameters:\n * section, key: Since the list string can sometimes be much longer than the maximum length of strings that can be returned by the Lua API, the list will instead by stored as a temporary ExtState specified by section and key.", },
	{ APIFUNC(JS_Window_ListAllTop), "void", "const char*,const char*", "section,key", "Returns a list of HWNDs of all top-level windows.\nThe list is formatted as a comma-separated (and terminated) string of hexadecimal values.\nEach value is an address that can be converted to a HWND by the function Window_HandleFromAddress.\n\nParameters:\n * section, key: Since the list string can sometimes be much longer than the maximum length of strings that can be returned by the Lua API, the list will instead by stored as a temporary ExtState specified by section and key.", },
	{ APIFUNC(JS_Window_ListFind), "void", "const char*,bool,const char*,const char*", "title,exact,section,key", "Returns a list of HWNDs of all windows (whether top-level or child) whose titles match the specified string.\nThe list is formatted as a comma-separated (and terminated) string of hexadecimal values.\nEach value is an address that can be converted to a HWND by the function Window_HandleFromAddress.\n\nParameters:\n * section, key: Since the list string can sometimes be much longer than the maximum length of strings that can be returned by the Lua API, the list will instead by stored as a temporary ExtState specified by section and key.\n * exact: Match entire title exactly, or match substring of title.", },

	{ APIFUNC(JS_MIDIEditor_ListAll), "void", "char*,int", "buf,buf_sz", "Returns a list of HWNDs of all open MIDI windows (whether docked or not).\n * The list is formatted as a comma-separated (and terminated) string of hexadecimal values.\n * Each value is an address that can be converted to a HWND by the function Window_HandleFromAddress.", },
	{ APIFUNC(JS_MIDIEditor_ArrayAll), "void", "void*", "reaperarray", "Returns the addresses of all open MIDI windows (whether docked or not).\n * The addresses are stored in the provided reaper.array.\n * Each address can be converted to a REAPER object (HWND) by the function JS_Window_HandleFromAddress.", },

	{ APIFUNC(JS_Window_Resize), "void", "void*,int,int", "windowHWND,width,height", "Changes the dimensions of the specified window, keeping the top left corner position constant.\n * If resizing script GUIs, call gfx.update() after resizing.", },
	{ APIFUNC(JS_Window_Move), "void", "void*,int,int", "windowHWND,left,top", "Changes the position of the specified window, keeping its size constant.\n * For a top-level window, the position is relative to the upper-left corner of the screen.\n * For a child window, they are relative to the upper-left corner of the parent window's client area.", },
	{ APIFUNC(JS_Window_SetPosition), "void", "void*,int,int,int,int", "windowHWND,left,top,width,height", "Sets the window position and size.", },
	{ APIFUNC(JS_Window_SetZOrder), "void", "void*,const char*,void*", "windowHWND,ZOrder,insertAfterHWND", "Sets the window Z order.\n\nParameters:\n * ZOrder: \"INSERT_AFTER\", \"BOTTOM\", \"TOPMOST\", \"NOTOPMOST\" or \"TOP\" ).\n * InsertAfterHWND: If ZOrder is INSERT_AFTER, insertAfterHWND must be a handle to the window to precede windowHWND in the Z order; otherwise, insertAfterHWND is ignored.", },
	{ APIFUNC(JS_Window_GetLongPtr), "void*", "void*,const char*", "windowHWND,info", "Returns information about the specified window.\n\ninfo: \"USERDATA\", \"WNDPROC\", \"DLGPROC\", \"ID\", \"EXSTYLE\" or \"STYLE\".\n\nFor documentation about the types of information returned, refer to the Win32 function GetWindowLongPtr."},
	{ APIFUNC(JS_Window_SetOpacity), "bool", "void*,const char*,double", "windowHWND,mode,value", "Sets the window opacity.\n\nParameters:\nmode: either \"ALPHA\" or \"COLOR\". \nvalue: If ALPHA, the specified value may range from zero to one, and will apply to the entire window. \nIf COLOR, value specifies a 0xRRGGBB color, and all pixels in this color will be made transparent. (All mouse clicks over transparent pixels will pass through, too).\n\nWARNING: COLOR mode is only available in Windows, not Linux or MacOS.", },

	{ APIFUNC(JS_Window_GetTitle), "void", "void*,char*,int", "windowHWND,buf,buf_sz", "Returns the title (if any) of the specified window.", },
	{ APIFUNC(JS_Window_SetTitle), "bool", "void*,const char*", "windowHWND,title", "Changes the title of the specified window. Returns true if successful.", },
#ifndef __APPLE__
    { APIFUNC(JS_Window_GetClassName), "void", "void*,char*,int", "windowHWND,buf,buf_sz", "WARNING: May not be fully implemented on MacOS and Linux.", },
#endif
    { APIFUNC(JS_Window_HandleFromAddress), "void*", "double", "address", "Converts an address to a handle (such as a HWND) that can be utilized by REAPER and other API functions.", },
	{ APIFUNC(JS_Window_AddressFromHandle), "void", "void*,double*", "handle,addressOut", "", },
	{ APIFUNC(JS_PtrFromStr), "void*", "const char*", "s", "", },

	{ APIFUNC(JS_WindowMessage_Post), "bool", "void*,const char*,int,int,int,int", "windowHWND,message,wParamLow,wParamHigh,lParamLow,lParamHigh", "Posts a message in the message queue associated with the thread that created the specified window, and returns without waiting.\n\nParameters:\n * message: String containing a single message name, such as \"WM_SETCURSOR\", or in hexadecimal format, \"0x0020\".\n (For a list of message types that are valid cross-platform, refer to swell-types.h. Only these will be recognized by WM_ name.)\n\nNotes:\n * For more information about parameter values, refer to documentation for the Win32 C++ function PostMessage.\n * Messages should only be sent to windows that were created from the main thread.\n * The message will be sent directly to the window, skipping interception by scripts.\n * Useful for simulating mouse clicks and calling mouse modifier actions from scripts.", },
	{ APIFUNC(JS_WindowMessage_Send), "int", "void*,const char*,int,int,int,int", "windowHWND,message,wParamLow,wParamHigh,lParamLow,lParamHigh", "Posts a message in the message queue associated with the thread that created the specified window, and returns without waiting.\n\nParameters:\n * message: String containing a single message name, such as \"WM_SETCURSOR\", or in hexadecimal format, \"0x0020\".\n (For a list of message types that are valid cross-platform, refer to swell-types.h. Only these will be recognized by WM_ name.)\n\nReturns:\n * Unlike JS_WindowMessage_Post, Send returns an LRESULT.\n\nNotes:\n * For more information about parameter and return values, refer to documentation for the Win32 C++ function SendMessage.\n * Messages should only be sent to windows that were created from the main thread.\n * The message will be sent directly to the window, skipping interception by scripts.\n * Useful for simulating mouse clicks and calling mouse modifier actions from scripts.", },
	{ APIFUNC(JS_WindowMessage_Peek), "bool", "void*,const char*,bool*,double*,int*,int*,int*,int*", "windowHWND,message,passedThroughOut,timeOut,wParamLowOut,wParamHighOut,lParamLowOut,lParamHighOut", "Polls the state of an intercepted message.\n\nParameters:\n * message: String containing a single message name, such as \"WM_SETCURSOR\", or in hexadecimal format, \"0x0020\".\n (For a list of message types that are valid cross-platform, refer to swell-types.h. Only these will be recognized by WM_ name.)\n\nReturns:\n * A retval of false indicates that the message type is not being intercepted in the specified window.\n * All messages are timestamped. A time of 0 indicates that no message if this type has been intercepted yet.\n * For more information about wParam and lParam for different message types, refer to Win32 C++ documentation.\n * For example, in the case of mousewheel, returns mousewheel delta, modifier keys, x position and y position.\n * wParamHigh, lParamLow and lParamHigh are signed, whereas wParamLow is unsigned.", },
	{ APIFUNC(JS_WindowMessage_Intercept), "int", "void*,const char*,bool", "windowHWND,messages,passThrough", "Intercepts window messages to specified window.\n\nParameters:\n * message: a single message type to be intercepted, either in WM_ or hexadecimal format. For example \"WM_SETCURSOR\" or \"0x0020\".\n * passThrough: Whether message should be blocked (false) or passed through (true) to the window.\n    For more information on message codes, refer to the Win32 C++ API documentation.\n    For a list of message types that are valid cross-platform, refer to swell-types.h. Only these will be recognized by WM_ name.\n\nReturns:\n * 1: Success.\n * 0: The message type is already being intercepted by another script.\n * -2: message string could not be parsed.\n * -3: Failure getting original window process / window not valid.\n\nNotes:\n * Intercepted messages can be polled using JS_WindowMessage_Peek.\n * Intercepted messages can be edited, if necessary, and then forwarded to their original destination using JS_WindowMessage_Post or JS_WindowMessage_Send.\n * To check whether a message type is being blocked or passed through, Peek the message type, or retrieve the entire List of intercepts.", },
	{ APIFUNC(JS_WindowMessage_InterceptList), "int", "void*,const char*", "windowHWND,messages", "Intercepts window messages to specified window.\n\nParameters:\n * messages: comma-separated string of message types to be intercepted (either in WM_ or hexadecimal format), each with a \"block\" or \"passthrough\" modifier to specify whether the message should be blocked or passed through to the window. For example \"WM_SETCURSOR:block, 0x0201:passthrough\".\n    For more information on message codes, refer to the Win32 C++ API documentation.\n    For a list of message types that are valid cross-platform, refer to swell-types.h. Only these will be recognized by WM_ name.\n\nReturns:\n * 1: Success.\n * 0: The message type is already being intercepted by another script.\n * -1: windowHWND is not a valid window.\n * -2: message string could not be parsed.\n * -3: Failure getting original window process.\n\nNotes:\n * Intercepted messages can be polled using JS_WindowMessage_Peek.\n * Intercepted messages can be edited, if necessary, and then forwarded to their original destination using JS_WindowMessage_Post or JS_WindowMessage_Send.\n * To check whether a message type is being blocked or passed through, Peek the message type, or retrieve the entire List of intercepts.", },
	{ APIFUNC(JS_WindowMessage_ListIntercepts), "bool", "void*,char*,int", "windowHWND,buf,buf_sz", "Returns a string with a list of all message types currently being intercepted for the specified window.", },
	{ APIFUNC(JS_WindowMessage_Release), "int", "void*,const char*", "windowHWND,messages", "Release intercepts of specified message types.\n\nParameters:\n * messages: \"WM_SETCURSOR,WM_MOUSEHWHEEL\" or \"0x0020,0x020E\", for example.", },
	{ APIFUNC(JS_WindowMessage_ReleaseWindow), "void", "void*", "windowHWND", "Release script intercepts of window messages for specified window.", },
	{ APIFUNC(JS_WindowMessage_ReleaseAll), "void", "", "", "Release script intercepts of window messages for all windows.", },

	{ APIFUNC(JS_Mouse_GetState), "int", "int", "flags", "Retrieves the states of mouse buttons and modifiers keys.\n\nParameters:\n * flags, state: The parameter and the return value both use the same format as gfx.mouse_cap. I.e., to get the states of the left mouse button and the ctrl key, use flags = 0b00000101.", },
	{ APIFUNC(JS_Mouse_SetPosition), "bool", "int,int", "x,y", "Moves the mouse cursor to the specified coordinates.", },
	{ APIFUNC(JS_Mouse_LoadCursor), "void*", "int", "cursorNumber", "Loads a cursor by number.\ncursorNumber: Same as used for gfx.setcursor, and includes some of Windows' predefined cursors (with numbers > 32000; refer to documentation for the Win32 C++ function LoadCursor), and REAPER's own cursors (with numbers < 2000). \nIf successful, returns a handle to the cursor, which can be used in JS_Mouse_SetCursor.", },
	{ APIFUNC(JS_Mouse_LoadCursorFromFile), "void*", "const char*", "pathAndFileName", "Loads a cursor from a .cur file.\nIf successful, returns a handle to the cursor, which can be used in JS_Mouse_SetCursor.", },
	{ APIFUNC(JS_Mouse_SetCursor), "void", "void*", "cursorHandle", "Sets the mouse cursor.  (Only lasts while script is running, and for a single \"defer\" cycle.)", },

	{ APIFUNC(JS_Window_GetScrollInfo), "bool", "void*,const char*,int*,int*,int*,int*,int*", "windowHWND,scrollbar,positionOut,pageSizeOut,minOut,maxOut,trackPosOut", "Retrieves the scroll information of a window.\n\nParameters:\n * scrollbar: \"v\" (or \"SB_VERT\", or \"VERT\") for vertical scroll, \"h\" (or \"SB_HORZ\" or \"HORZ\") for horizontal.\n\nReturns:\n * Leftmost or topmost visible pixel position, as well as the visible page size, the range minimum and maximum, and scroll box tracking position.", },
	{ APIFUNC(JS_Window_SetScrollPos), "bool", "void*,const char*,int", "windowHWND,scrollbar,position", "Parameters:\n * scrollbar: \"v\" (or \"SB_VERT\", or \"VERT\") for vertical scroll, \"h\" (or \"SB_HORZ\" or \"HORZ\") for horizontal.\n\nNOTE: API functions can scroll REAPER's windows, but cannot zoom them.  Instead, use actions such as \"View: Zoom to one loop iteration\"."},

	{ APIFUNC(JS_GDI_GetClientDC), "void*", "void*", "windowHWND", "Returns the device context for the client area of the specified window.", },
	{ APIFUNC(JS_GDI_GetWindowDC), "void*", "void*", "windowHWND", "Returns the device context for the entire window, including title bar and frame.", },
	{ APIFUNC(JS_GDI_GetScreenDC), "void*", "", "", "Returns a device context for the entire screen.\n\nWARNING: Only available on Windows, not Linux or MacOS.", },
	{ APIFUNC(JS_GDI_ReleaseDC), "void", "void*,void*", "windowHWND,deviceHDC", "", },

	{ APIFUNC(JS_GDI_CreateFillBrush), "void*", "int", "color", "", },
	{ APIFUNC(JS_GDI_CreatePen), "void*", "int,int", "width,color", "", },
	{ APIFUNC(JS_GDI_CreateFont), "void*", "int,int,int,bool,bool,bool,const char*", "height,weight,angle,italic,underline,strikeOut,fontName", "Parameters:\n * weight: 0 - 1000, with 0 = auto, 400 = normal and 700 = bold.\n * angle: the angle, in tenths of degrees, between the text and the x-axis of the device.\n * fontName: If empty string \"\", uses first font that matches the other specified attributes.\n\nNote: Text color must be set separately.", },
	{ APIFUNC(JS_GDI_SelectObject), "void*", "void*,void*", "deviceHDC,GDIObject", "Activates a font, pen, or fill brush for subsequent drawing in the specified device context.", },
	{ APIFUNC(JS_GDI_DeleteObject), "void", "void*", "GDIObject", "", },

	{ APIFUNC(JS_GDI_FillRect), "void", "void*,int,int,int,int", "deviceHDC,left,top,right,bottom", "", },
	{ APIFUNC(JS_GDI_FillRoundRect), "void", "void*,int,int,int,int,int,int", "deviceHDC,left,top,right,bottom,xrnd,yrnd", "", },
	{ APIFUNC(JS_GDI_FillPolygon), "void", "void*,const char*,const char*,int", "deviceHDC,packedX,packedY,numPoints", "packedX and packedY are strings of points, each packed as \"<i4\".", },
	{ APIFUNC(JS_GDI_FillEllipse), "void", "void*,int,int,int,int", "deviceHDC,left,top,right,bottom", "", },

	{ APIFUNC(JS_GDI_GetSysColor), "int", "const char*", "GUIElement", "", },
	{ APIFUNC(JS_GDI_SetTextBkMode), "void", "void*,int", "deviceHDC,mode", "", },
	{ APIFUNC(JS_GDI_SetTextBkColor), "void", "void*,int", "deviceHDC,color", "", },
	{ APIFUNC(JS_GDI_SetTextColor), "void", "void*,int", "deviceHDC,color", "", },
	{ APIFUNC(JS_GDI_GetTextColor), "int", "void*", "deviceHDC", "", },
	{ APIFUNC(JS_GDI_DrawText), "int", "void*,const char*,int,int,int,int,int,const char*", "deviceHDC,text,len,left,top,right,bottom,align)", "Parameters:\n * align: Combination of: \"TOP\", \"VCENTER\", \"LEFT\", \"HCENTER\", \"RIGHT\", \"BOTTOM\", \"WORDBREAK\", \"SINGLELINE\", \"NOCLIP\", \"CALCRECT\", \"NOPREFIX\" or \"ELLIPSIS\"", },

	{ APIFUNC(JS_GDI_SetPixel), "void", "void*,int,int,int", "deviceHDC,x,y,color", "", },
	//{ APIFUNC(JS_GDI_MoveTo), "void", "void*,int,int", "deviceHDC,x,y", "", },
	//{ APIFUNC(JS_GDI_LineTo), "void", "void*,int,int", "deviceHDC,x,y", "", },
	{ APIFUNC(JS_GDI_Line), "void", "void*,int,int,int,int", "deviceHDC,x1,y1,x2,y2", "", },
	{ APIFUNC(JS_GDI_Polyline), "void", "void*,const char*,const char*,int", "deviceHDC,packedX,packedY,numPoints", "packedX and packedY are strings of points, each packed as \"<i4\".", },
	{ APIFUNC(JS_GDI_Blit), "void", "void*,int,int,void*,int,int,int,int", "destHDC,dstx,dsty,sourceHDC,srcx,srxy,width,height", "Blits between two device contexts, which may include LICE \"system bitmaps\"." , },
	{ APIFUNC(JS_GDI_StretchBlit), "void", "void*,int,int,int,int,void*,int,int,int,int", "destHDC,dstx,dsty,dstw,dsth,sourceHDC,srcx,srxy,srcw,srch", "Blits between two device contexts, which may include LICE \"system bitmaps\"." , },

	{ APIFUNC(JS_LICE_CreateBitmap), "void*", "bool,int,int", "isSysBitmap,width,height", "", },
	{ APIFUNC(JS_LICE_GetHeight), "int", "void*", "bitmap", "", },
	{ APIFUNC(JS_LICE_GetWidth), "int", "void*", "bitmap", "", },
	{ APIFUNC(JS_LICE_GetDC), "void*", "void*", "bitmap", "", },
	{ APIFUNC(JS_LICE_DestroyBitmap), "void", "void*", "bitmap", "", },
	{ APIFUNC(JS_LICE_LoadPNG), "void*", "const char*", "filename", "Returns a (non-system) LICE bitmap containing the PNG.", },
	{ APIFUNC(JS_LICE_Blit), "void", "void*,int,int,void*,int,int,int,int,double,const char*", "destBitmap,dstx,dsty,sourceBitmap,srcx,srcy,width,height,alpha,mode", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".", },
	{ APIFUNC(JS_LICE_RotatedBlit), "void", "void*,int,int,int,int,void*,double,double,double,double,double,double,double,bool,double,const char*", "destBitmap,dstx,dsty,dstw,dsth,sourceBitmap,srcx,srcy,srcw,srch,angle,rotxcent,rotycent,cliptosourcerect,alpha,mode", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".", },
	{ APIFUNC(JS_LICE_ScaledBlit), "void", "void*,int,int,int,int,void*,double,double,double,double,double,const char*", "destBitmap,dstx,dsty,dstw,dsth,srcBitmap,srcx,srcy,srcw,srch,alpha,mode", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".", },
	{ APIFUNC(JS_LICE_IsFlipped), "bool", "void*", "bitmap", "", },
	{ APIFUNC(JS_LICE_Resize), "void", "void*,int,int", "bitmap,width,height", "", },
	{ APIFUNC(JS_LICE_Clear), "void", "void*,int", "bitmap,color", "", },

	{ APIFUNC(JS_LICE_CreateFont), "void*", "", "", "", },
	{ APIFUNC(JS_LICE_DestroyFont), "void", "void*", "LICEFont", "", },
	{ APIFUNC(JS_LICE_SetFontFromGDI), "void", "void*,void*,const char*", "LICEFont,GDIFont,moreFormats", "Converts a GDI font into a LICE font.\n\nThe font can be modified by the following flags, in a comma-separated list:\n\"VERTICAL\", \"BOTTOMUP\", \"NATIVE\", \"BLUR\", \"INVERT\", \"MONO\", \"SHADOW\" or \"OUTLINE\".", },
	{ APIFUNC(JS_LICE_SetFontColor), "void", "void*,int", "LICEFont,color", "", },
	{ APIFUNC(JS_LICE_SetFontBkColor), "void", "void*,int", "LICEFont,color", "", },
	{ APIFUNC(JS_LICE_DrawText), "int", "void*,void*,const char*,int,int,int,int,int", "bitmap,LICEFont,text,textLen,x1,y1,x2,y2", "", },
	{ APIFUNC(JS_LICE_DrawChar), "void", "void*,int,int,char,int,double,int", "bitmap,x,y,c,color,alpha,mode)", "", },

	{ APIFUNC(JS_LICE_GradRect), "void", "void*,int,int,int,int,double,double,double,double,double,double,double,double,double,double,double,double,const char*", "bitmap,dstx,dsty,dstw,dsth,ir,ig,ib,ia,drdx,dgdx,dbdx,dadx,drdy,dgdy,dbdy,dady,mode", "", },
	{ APIFUNC(JS_LICE_FillRect), "void", "void*,int,int,int,int,int,double,const char*", "bitmap,x,y,w,h,color,alpha,mode", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".\n\nLICE color format: 0xAARRGGBB (AA is only used in ALPHA mode).", },
	{ APIFUNC(JS_LICE_FillTriangle), "void", "void*,int,int,int,int,int,int,int,double,const char*", "bitmap,x1,y1,x2,y2,x3,y3,color,alpha,mode", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".\n\nLICE color format: 0xAARRGGBB (AA is only used in ALPHA mode).", },
	{ APIFUNC(JS_LICE_FillPolygon), "void", "void*,const char*,const char*,int,int,double,const char*", "bitmap,packedX,packedY,numPoints,color,alpha,mode", "packedX and packedY are two strings of coordinates, each packed as \"<i4\".\n\nLICE modes : \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".\n\nLICE color format: 0xAARRGGBB (AA is only used in ALPHA mode).", },
	{ APIFUNC(JS_LICE_FillCircle), "void", "void*,double,double,double,int,double,const char*,bool", "bitmap,cx,cy,r,color,alpha,mode,antialias", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".\n\nLICE color format: 0xAARRGGBB (AA is only used in ALPHA mode).", },

	{ APIFUNC(JS_LICE_Line), "void", "void*,double,double,double,double,int,double,const char*,bool", "bitmap,x1,y1,x2,y2,color,alpha,mode,antialias", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".\n\nLICE color format: 0xAARRGGBB (AA is only used in ALPHA mode).", },
	{ APIFUNC(JS_LICE_Bezier), "void", "void*,double,double,double,double,double,double,double,double,double,int,double,const char*,bool", "bitmap,xstart,ystart,xctl1,yctl1,xctl2,yctl2,xend,yend,tol,color,alpha,mode,antialias", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".\n\nLICE color format: 0xAARRGGBB (AA is only used in ALPHA mode).", },

	{ APIFUNC(JS_LICE_Arc), "void", "void*,double,double,double,double,double,int,double,const char*,bool", "bitmap,cx,cy,r,minAngle,maxAngle,color,alpha,mode,antialias", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".\n\nLICE color format: 0xAARRGGBB (AA is only used in ALPHA mode).", },
	{ APIFUNC(JS_LICE_Circle), "void", "void*,double,double,double,int,double,const char*,bool", "bitmap,cx,cy,r,color,alpha,mode,antialias", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".\n\nLICE color format: 0xAARRGGBB (AA is only used in ALPHA mode).", },
	{ APIFUNC(JS_LICE_RoundRect), "void", "void*,double,double,double,double,int,int,double,const char*,bool", "bitmap,x,y,w,h,cornerradius,color,alpha,mode,antialias", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".\n\nLICE color format: 0xAARRGGBB (AA is only used in ALPHA mode).", },

	{ APIFUNC(JS_LICE_GetPixel), "int", "void*,int,int", "bitmap,x,y", "Returns the color of the specified pixel.", },
	{ APIFUNC(JS_LICE_PutPixel), "void", "void*,int,int,int,double,const char*", "bitmap,x,y,color,alpha,mode", "LICE modes: \"COPY\" (default if empty string), \"MASK\", \"ADD\", \"DODGE\", \"MUL\", \"OVERLAY\" or \"HSVADJ\", any of which may be combined with \"ALPHA\".\n\nLICE color format: 0xAARRGGBB (AA is only used in ALPHA mode).", },
	
	{ APIFUNC(JS_Window_AttachTopmostPin), "void", "void*", "windowHWND", "", },
	{ APIFUNC(JS_Window_AttachResizeGrip), "void", "void*", "windowHWND", "", },
	{ APIFUNC(JS_Window_RemoveXPStyle), "bool", "void*,bool", "windowHWND,remove", "", },

	{ APIFUNC(JS_Int), "void", "void*,int,int*", "address,offset,intOut", "Returns the 4-byte signed integer at address[offset]. Offset is added as steps of 4 bytes each.", },
	{ APIFUNC(JS_Byte), "void", "void*,int,int*", "address,offset,byteOut", "Returns the unsigned byte at address[offset]. Offset is added as steps of 1 byte each.", },
	{ APIFUNC(JS_Double), "void", "void*,int,double*", "address,offset,doubleOut", "Returns the 8-byte floating point value at address[offset]. Offset is added as steps of 8 bytes each.", },
	
	{ APIFUNC(Xen_AudioWriter_Create), "AudioWriter*", "const char*,int,int", "filename,numchans,samplerate", "Creates writer for 32 bit floating point WAV", },
	{ APIFUNC(Xen_AudioWriter_Destroy), "void", "AudioWriter*", "writer", "Destroys writer", },
	{ APIFUNC(Xen_AudioWriter_Write), "int", "AudioWriter*,int,void*,int", "writer,numframes,data,offset", "Write interleaved audio data to disk", },
	{ APIFUNC(Xen_GetMediaSourceSamples), "int", "PCM_source*,void*,int,int,int,double,double", "src,destbuf,destbufoffset,numframes,numchans,samplerate,sourceposition", "Get interleaved audio data from media source", }
};

///////////////////////////////////////////////////////////////////////////////
