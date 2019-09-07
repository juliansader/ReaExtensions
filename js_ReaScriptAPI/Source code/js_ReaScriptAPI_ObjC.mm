#import <Cocoa/Cocoa.h>
#import <objc/objc-runtime.h>

// The NSWindow is the container of the NSView child windows.
void* JS_GetNSWindowFromSwellHWND(void* hwnd)
{
	NSWindow* window = NULL;
	if (hwnd)
	{
		if ([(id)hwnd isKindOfClass:[NSView class]])
		{
			NSView* view = (NSView*)hwnd;
			window = (NSWindow*)[view window]; // The view’s window object, if it is installed in a window.
		}
		else if ([(id)hwnd isKindOfClass:[NSWindow class]])
			window = (NSWindow*)hwnd;
	}
	return window;
}

bool JS_Window_SetOpacity_ObjC(void* hwnd, double alpha)
{
	NSWindow* window = (NSWindow*)JS_GetNSWindowFromSwellHWND(hwnd);
	
	if (window)
	{
		CGFloat opacity = (CGFloat)alpha;
		[window setOpaque:NO];
		//[window setWantsLayer:YES];
		[window setAlphaValue:opacity];
		//[window setBackgroundColor:[NSColor clearColor]];
		//[window.layer setBackgroundColor:[NSColor clearColor]];
		return true;
	}
	else
		return false;
}

// Get the NSWindowLevel of a swell HWND.
int JS_GetLevel_ObjC(void* hwnd)
{
	NSWindow* window = (NSWindow*)JS_GetNSWindowFromSwellHWND(hwnd);
	
	if (window)
		return (int)[window level];
	else
		return 0;
}

// swell doesn't provide full functionality for SetWindowPos. In particular, TOPMOST and NOTOPMOST aren't implemented.
// So I have tried to code my own.
// This was necessary for JS_Window_Create, since swell automatically gives the window a level of 0, which is below REAPER's floating windows.
bool JS_Window_SetZOrder_ObjC(void* hwnd, void* insertAfterHWND)
/*
	Here are some of the standard macOS window levels:
	backstopMenu: -20
	normalWindow: 0
	floatingWindow: 3
	tornOffMenuWindow: 3
	modalPanelWindow: 8
	utilityWindow: 19
	dockWindow: 20
	mainMenuWindow: 24
	statusWindow: 25
	popUpMenuWindow: 101
	overlayWindow: 102
	
	REAPER uses a non-standard level for its floating windows, namely 1, 
		which is just slightly above the main window at 0 = NSNormalWindowLevel.
*/
{
	NSWindow* window = (NSWindow*)JS_GetNSWindowFromSwellHWND(hwnd);
	
	if (window)
	{
		NSWindow* afterThisWindow = NULL; // Declare here, since can't inside switch
		
		switch ((intptr_t)insertAfterHWND)
		{
			case -1: //HWND_TOPMOST:
				[window setLevel: NSFloatingWindowLevel]; // = 3
				return true;
			case -2: //HWND_NOTOPMOST:
				[window setLevel: 1]; // Standard level for REAPER's floating windows
				return true;
			case 0: //HWND_TOP:
				if ([window level] < 1)
					[window setLevel: 1];
				[window orderWindow:NSWindowAbove relativeTo:0]; // Bring to top within current level
				return true;
			case 1: //HWND_BELOW:
				[window setLevel: NSNormalWindowLevel]; // Below REAPER's floating windoww, same level as main
				[window orderWindow:NSWindowBelow relativeTo:0];
				return true;
			default: // insertAfter is a target window
				afterThisWindow = (NSWindow*)JS_GetNSWindowFromSwellHWND(insertAfterHWND);
				if (afterThisWindow)
				{
					[window setLevel: [afterThisWindow level]];
					[window orderWindow:NSWindowBelow relativeTo:[afterThisWindow windowNumber]];
					return true;
				}
		}
	}
   	return false;
}
