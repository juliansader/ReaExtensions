#pragma once

extern "C" REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec);

void  JS_ReaScriptAPI_Version(double* versionOut);

void  JS_Localize(const char* USEnglish, const char* LangPackSection, char* translationOut, int translationOut_sz);

void* JS_Mem_Alloc(int sizeBytes);
bool  JS_Mem_Free(void* mallocPointer);
bool  JS_Mem_FromString(void* mallocPointer, int offset, const char* packedString, int stringLength);
bool  JS_String(void* pointer, int offset, int lengthChars, char* bufOutNeedBig, int bufOutNeedBig_sz);
void  JS_Int(void* pointer, int offset, int* intOut);
void  JS_Byte(void* pointer, int offset, int* byteOut);
void  JS_Double(void* pointer, int offset, double* doubleOut);

int   JS_Dialog_BrowseForSaveFile(const char* windowTitle, const char* initialFolder, const char* initialFile, const char* extensionList, char* fileNameOutNeedBig, int fileNameOutNeedBig_sz);
int   JS_Dialog_BrowseForFolder(const char* caption, const char* initialFolder, char* folderOutNeedBIg, int folderOutNeedBig_sz);
int   JS_Dialog_BrowseForOpenFiles(const char* windowTitle, const char* initialFolder, const char* initialFile, const char* extensionList, bool allowMultiple, char* fileNamesOutNeedBig, int fileNamesOutNeedBig_sz);

bool  JS_Window_GetRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut);
bool  JS_Window_GetClientRect(void* windowHWND, int* leftOut, int* topOut, int* rightOut, int* bottomOut);
bool  JS_Window_GetClientSize(void* windowHWND, int* widthOut, int* heightOut);
void  JS_Window_ScreenToClient(void* windowHWND, int x, int y, int* xOut, int* yOut);
void  JS_Window_ClientToScreen(void* windowHWND, int x, int y, int* xOut, int* yOut);
void  JS_Window_MonitorFromRect(int x1, int y1, int x2, int y2, bool wantWork, int* leftOut, int* topOut, int* rightOut, int* bottomOut);

void* JS_Window_FromPoint(int x, int y);

void* JS_Window_GetParent(void* windowHWND);
bool  JS_Window_IsChild(void* parentHWND, void* childHWND);
void* JS_Window_GetRelated(void* windowHWND, const char* relation);
HWND  JS_Window_FindChildByID(HWND parent, int ID);  // Functions that receive and return void* are in SWS style. Newer functions use HWND and let vararg wrapper typecast.

HWND  JS_Window_FindEx(HWND parentHWND, HWND childHWND, const char* className, const char* title);
void* JS_Window_Find(const char* title, bool exact);
void* JS_Window_FindChild(void* parentHWND, const char* title, bool exact);
int   JS_Window_ArrayAllChild(void* parentHWND, double* reaperarray);
int   JS_Window_ArrayAllTop(double* reaperarray);
int   JS_Window_ArrayFind(const char* title, bool exact, double* reaperarray);
int   JS_MIDIEditor_ArrayAll(double* reaperarray);
int   JS_Window_ListAllChild(void* parentHWND, char* listOutNeedBig, int listOutNeedBig_sz);
int   JS_Window_ListAllTop(char* listOutNeedBig, int listOutNeedBig_sz);
int   JS_Window_ListFind(const char* title, bool exact, char* listOutNeedBig, int listOutNeedBig_sz);
int   JS_MIDIEditor_ListAll(char* listOutNeedBig, int listOutNeedBig_sz);

void  JS_Window_Move(void* windowHWND, int left, int top);
void  JS_Window_Resize(void* windowHWND, int width, int height);
void  JS_Window_SetPosition(void* windowHWND, int left, int top, int width, int height);
void  JS_Window_SetZOrder(void* windowHWND, const char* ZOrder, void* insertAfterHWND);
void* JS_Window_GetLongPtr(void* windowHWND, const char* info);
void  JS_Window_GetLong(void* windowHWND, const char* info, double* retvalOut);
bool  JS_Window_SetOpacity_ObjC(HWND windowHWND, double alpha);
bool  JS_Window_SetOpacity(HWND windowHWND, const char* mode, double value);

void  JS_Window_SetFocus(void* windowHWND);
void* JS_Window_GetFocus();
void  JS_Window_SetForeground(void* windowHWND);
void* JS_Window_GetForeground();

void  JS_Window_Enable(void* windowHWND, bool enable);
void  JS_Window_Destroy(void* windowHWND);
void  JS_Window_Show(void* windowHWND, const char* state);
bool  JS_Window_IsVisible(void* windowHWND);

bool  JS_Window_SetTitle(void* windowHWND, const char* title);
void  JS_Window_GetTitle(void* windowHWND, char* titleOut, int titleOut_sz);

void  JS_Window_GetClassName(HWND windowHWND, char* classOut, int classOut_sz);

void* JS_Window_HandleFromAddress(double address);
void  JS_Window_AddressFromHandle(void* handle, double* addressOut);
bool  JS_Window_IsWindow(void* windowHWND);

int   JS_WindowMessage_Intercept(void* windowHWND, const char* message, bool passThrough);
int   JS_WindowMessage_InterceptList(void* windowHWND, const char* messages);
int   JS_WindowMessage_PassThrough(void* windowHWND, const char* message, bool passThrough);
bool  JS_WindowMessage_ListIntercepts(void* windowHWND, char* listOutNeedBig, int listOutNeedBig_sz);
bool  JS_WindowMessage_Post(void* windowHWND, const char* message, int wParamLow, int wParamHigh, int lParamLow, int lParamHigh);
int   JS_WindowMessage_Send(void* windowHWND, const char* message, int wParamLow, int wParamHigh, int lParamLow, int lParamHigh);
bool  JS_WindowMessage_Peek(void* windowHWND, const char* message, bool* passedThroughOut, double* timeOut, int* wParamLowOut, int* wParamHighOut, int* lParamLowOut, int* lParamHighOut);
int   JS_WindowMessage_Release(void* windowHWND, const char* messages);
void  JS_WindowMessage_ReleaseWindow(void* windowHWND);
void  JS_WindowMessage_ReleaseAll();
bool  JS_Window_OnCommand(void* windowHWND, int commandID);

int   JS_Mouse_GetState(int flags);
bool  JS_Mouse_SetPosition(int x, int y);
void* JS_Mouse_LoadCursor(int cursorNumber);
void* JS_Mouse_LoadCursorFromFile(const char* pathAndFileName, bool* forceNewLoadOptional);
void  JS_Mouse_SetCursor(void* cursorHandle);

bool  JS_Window_GetScrollInfo(void* windowHWND, const char* scrollbar, int* positionOut, int* pageSizeOut, int* minOut, int* maxOut, int* trackPosOut);
bool  JS_Window_SetScrollPos(void* windowHWND, const char* scrollbar, int position);

void* JS_GDI_GetClientDC(void* windowHWND);
void* JS_GDI_GetWindowDC(void* windowHWND);
void* JS_GDI_GetScreenDC();
void  JS_GDI_ReleaseDC(void* windowHWND, void* deviceHDC);

void* JS_GDI_CreateFillBrush(int color);
void* JS_GDI_CreatePen(int width, int color);
void* JS_GDI_CreateFont(int height, int weight, int angle, bool italic, bool underline, bool strikeOut, const char* fontName);
void* JS_GDI_SelectObject(void* deviceHDC, void* GDIObject);
void  JS_GDI_DeleteObject(void* GDIObject);

int   JS_GDI_GetSysColor(const char* GUIElement);
void  JS_GDI_SetTextColor(void* deviceHDC, int color);
int   JS_GDI_GetTextColor(void* deviceHDC);
void  JS_GDI_SetTextBkMode(void* deviceHDC, int mode);
void  JS_GDI_SetTextBkColor(void* deviceHDC, int color);
int   JS_GDI_DrawText(void* deviceHDC, const char *text, int len, int left, int top, int right, int bottom, const char* align);

void  JS_GDI_FillRoundRect(void* deviceHDC, int x, int y, int x2, int y2, int xrnd, int yrnd);
void  JS_GDI_FillRect(void* deviceHDC, int left, int top, int right, int bottom);
void  JS_GDI_FillPolygon(void* deviceHDC, const char* packedX, const char* packedY, int numPoints);
void  JS_GDI_FillEllipse(void* deviceHDC, int left, int top, int right, int bottom);

void  JS_GDI_SetPixel(void* deviceHDC, int x, int y, int color);
void  JS_GDI_Line(void* deviceHDC, int x1, int y1, int x2, int y2);
void  JS_GDI_Polyline(void* deviceHDC, const char* packedX, const char* packedY, int numPoints);

void  JS_GDI_Blit(void* destHDC, int dstx, int dsty, void* sourceHDC, int srcx, int srcy, int width, int height);
void  JS_GDI_StretchBlit(void* destHDC, int dstx, int dsty, int dstw, int dsth, void* sourceHDC, int srcx, int srcy, int srcw, int srch);

void* JS_LICE_CreateBitmap(bool isSysBitmap, int width, int height);
int	  JS_LICE_GetHeight(void* bitmap);
int   JS_LICE_GetWidth(void* bitmap);
void* JS_LICE_GetDC(void* bitmap);
void  JS_LICE_DestroyBitmap(void* bitmap);

void  JS_LICE_Blit(void* destBitmap, int dstx, int dsty, void* sourceBitmap, int srcx, int srcy, int width, int height, double alpha, const char* mode);
void  JS_LICE_RotatedBlit(void* destBitmap, int dstx, int dsty, int dstw, int dsth, void* sourceBitmap, double srcx, double srcy, double srcw, double srch, double angle, double rotxcent, double rotycent, bool cliptosourcerect, double alpha, const char* mode);
void  JS_LICE_ScaledBlit(void* destBitmap, int dstx, int dsty, int dstw, int dsth, void* sourceBitmap, double srcx, double srcy, double srcw, double srch, double alpha, const char* mode);

void* JS_LICE_LoadPNG(const char* filename);
bool  JS_LICE_IsFlipped(void* bitmap);
bool  JS_LICE_Resize(void* bitmap, int width, int height);
void  JS_LICE_Clear(void* bitmap, int color);

void* JS_LICE_CreateFont();
void  JS_LICE_DestroyFont(void* LICEFont);
void  JS_LICE_SetFontFromGDI(void* LICEFont, void* GDIFont, const char* moreFormats);
void  JS_LICE_SetFontBkColor(void* LICEFont, int color);
void  JS_LICE_SetFontColor(void* LICEFont, int color);
int   JS_LICE_DrawText(void* bitmap, void* LICEFont, const char* text, int textLen, int x1, int y1, int x2, int y2);
void  JS_LICE_DrawChar(void* bitmap, int x, int y, char c, int color, double alpha, const char* mode);
void  JS_LICE_MeasureText(const char* string, int* widthOut, int* heightOut);

void  JS_LICE_GradRect(void* bitmap, int dstx, int dsty, int dstw, int dsth, double ir, double ig, double ib, double ia, double drdx, double dgdx, double dbdx, double dadx, double drdy, double dgdy, double dbdy, double dady, const char* mode);
void  JS_LICE_FillRect(void* bitmap, int x, int y, int w, int h, int color, double alpha, const char* mode);
void  JS_LICE_FillTriangle(void* bitmap, int x1, int y1, int x2, int y2, int x3, int y3, int color, double alpha, const char* mode);
void  JS_LICE_FillPolygon(void* bitmap, const char* packedX, const char* packedY, int numPoints, int color, double alpha, const char* mode);
void  JS_LICE_FillCircle(void* bitmap, double cx, double cy, double r, int color, double alpha, const char* mode, bool antialias);

void  JS_LICE_Line(void* bitmap, double x1, double y1, double x2, double y2, int color, double alpha, const char* mode, bool antialias);
void  JS_LICE_Bezier(void* bitmap, double xstart, double ystart, double xctl1, double yctl1, double xctl2, double yctl2, double xend, double yend, double tol, int color, double alpha, const char* mode, bool antialias);
void  JS_LICE_Arc(void* bitmap, double cx, double cy, double r, double minAngle, double maxAngle, int color, double alpha, const char* mode, bool antialias);
void  JS_LICE_Circle(void* bitmap, double cx, double cy, double r, int color, double alpha, const char* mode, bool antialias);
void  JS_LICE_RoundRect(void* bitmap, double x, double y, double w, double h, int cornerradius, int color, double alpha, const char* mode, bool antialias);

int   JS_LICE_GetPixel(void* bitmap, int x, int y);
void  JS_LICE_PutPixel(void* bitmap, int x, int y, int color, double alpha, const char* mode);

HWND  JS_Window_AttachTopmostPin(HWND windowHWND);
void  JS_Window_AttachResizeGrip(void* windowHWND);

int   JS_ListView_GetItemCount(HWND listviewHWND);
int   JS_ListView_GetSelectedCount(HWND listviewHWND);
int   JS_ListView_GetFocusedItem(HWND listviewHWND, char* textOut, int textOut_sz);
int   JS_ListView_EnumSelItems(HWND listviewHWND, int index);
void  JS_ListView_GetItem(HWND listviewHWND, int index, int subItem, char* textOut, int textOut_sz, int* stateOut);
int   JS_ListView_GetItemState(HWND listviewHWND, int index);
void  JS_ListView_GetItemText(HWND listviewHWND, int index, int subItem, char* textOut, int textOut_sz);
int   JS_ListView_ListAllSelItems(HWND listviewHWND, char* itemsOutNeedBig, int itemsOutNeedBig_sz);

class AudioWriter;

AudioWriter* Xen_AudioWriter_Create(const char* filename, int numchans, int samplerate);
void Xen_AudioWriter_Destroy(AudioWriter* aw);
int Xen_AudioWriter_Write(AudioWriter* aw, double* data, int numframes, int offset);
int Xen_GetMediaSourceSamples(PCM_source* src, double* destbuf, int destbufoffset, int numframes, int numchans, double samplerate, double positioninfile);
