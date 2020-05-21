#pragma once

#ifdef _WIN32
#pragma warning(disable:4800) // disable "forcing value to bool..." warnings
#endif

static void* __vararg_JS_ReaScriptAPI_Version(void** arglist, int numparms)
{
	JS_ReaScriptAPI_Version((double*)arglist[0]);
	return NULL;
}

static void* __vararg_JS_Localize(void** arglist, int numparms)
{
	JS_Localize((const char*)arglist[0], (const char*)arglist[1], (char*)arglist[2], (int)(intptr_t)arglist[3]);
	return NULL;
}

/*/////////////////////////////////////////////////////////////////////////////////

static void* __vararg_JS_Zip_Add(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Zip_Add((char*)arglist[0], (char*)arglist[1]);
}

*//////////////////////////////////////////////////////////////////////////////////

static void* __vararg_JS_VKeys_GetState(void** arglist, int numparms)
{
	JS_VKeys_GetState(arglist[0] ? *(double*)arglist[0] : 0.0, (char*)arglist[1], (int)(intptr_t)arglist[2]);
	return NULL;
}

static void* __vararg_JS_VKeys_GetDown(void** arglist, int numparms)
{
	JS_VKeys_GetDown(arglist[0] ? *(double*)arglist[0] : 0.0, (char*)arglist[1], (int)(intptr_t)arglist[2]);
	return NULL;
}

static void* __vararg_JS_VKeys_GetUp(void** arglist, int numparms)
{
	JS_VKeys_GetUp(arglist[0] ? *(double*)arglist[0] : 0.0, (char*)arglist[1], (int)(intptr_t)arglist[2]);
	return NULL;
}

static void* __vararg_JS_VKeys_Intercept(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_VKeys_Intercept((int)(intptr_t)arglist[0], (int)(intptr_t)arglist[1]);
}

////////////////////////////////////////////////////////////////////////////

static void* __vararg_JS_Composite(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Composite((HWND)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (LICE_IBitmap*)arglist[5], (int)(intptr_t)arglist[6], (int)(intptr_t)arglist[7], (int)(intptr_t)arglist[8], (int)(intptr_t)arglist[9], numparms > 10 ? ((bool)arglist[10] && *(bool*)arglist[10]) : false);
}

static void* __vararg_JS_Composite_Unlink(void** arglist, int numparms)
{
	JS_Composite_Unlink((HWND)arglist[0], numparms > 1 ? (LICE_IBitmap*)arglist[1] : nullptr, numparms > 2 ? ((bool)arglist[2] && *(bool*)arglist[2]) : false);
	return nullptr;
}

static void* __vararg_JS_Composite_ListBitmaps(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Composite_ListBitmaps((HWND)arglist[0], (char*)arglist[1], (int)(intptr_t)arglist[2]);
}

static void* __vararg_JS_Composite_Delay(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Composite_Delay((HWND)arglist[0], arglist[1] ? *(double*)arglist[1] : 0, arglist[2] ? *(double*)arglist[2] : 0, (int)(intptr_t)arglist[3],  (double*)arglist[4], (double*)arglist[5], (int*)arglist[6]);
}

////////////////////////////////////////////////////////////////////////////

static void* __vararg_JS_Mem_Alloc(void** arglist, int numparms)
{
	return JS_Mem_Alloc((int)(intptr_t)arglist[0]);
}

static void* __vararg_JS_Mem_Free(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Mem_Free(arglist[0]);
}

static void* __vararg_JS_Mem_FromString(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Mem_FromString(arglist[0], (int)(intptr_t)arglist[1], (const char*)arglist[2], (int)(intptr_t)arglist[3]);
}

static void* __vararg_JS_String(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_String(arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (char*)arglist[3], (int)(intptr_t)arglist[4]);
}

static void* __vararg_JS_Int(void** arglist, int numparms)
{
	JS_Int(arglist[0], (int)(intptr_t)arglist[1], (int*)arglist[2]);
	return NULL;
}

static void* __vararg_JS_Byte(void** arglist, int numparms)
{
	JS_Byte(arglist[0], (int)(intptr_t)arglist[1], (int*)arglist[2]);
	return NULL;
}

static void* __vararg_JS_Double(void** arglist, int numparms)
{
	JS_Double(arglist[0], (int)(intptr_t)arglist[1], (double*)arglist[2]);
	return NULL;
}


///////////////////////////////////////////////////////////////////////////

static void* __vararg_JS_Window_Create(void** arglist, int numparms)
{
	return JS_Window_Create((const char*)arglist[0], (const char*)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (int)(intptr_t)arglist[5], numparms > 6 ? (char*)arglist[6] : nullptr, numparms > 7 ? arglist[7] : nullptr);
}

static void* __vararg_JS_Dialog_BrowseForSaveFile(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Dialog_BrowseForSaveFile((const char*)arglist[0], (const char*)arglist[1], (const char*)arglist[2], (const char*)arglist[3], (char*)arglist[4], (int)(intptr_t)arglist[5]);
}

static void* __vararg_JS_Dialog_BrowseForOpenFiles(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Dialog_BrowseForOpenFiles((const char*)arglist[0], (const char*)arglist[1], (const char*)arglist[2], (const char*)arglist[3], (bool)arglist[4], (char*)arglist[5], (int)(intptr_t)arglist[6]);
}

static void* __vararg_JS_Dialog_BrowseForFolder(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Dialog_BrowseForFolder((const char*)arglist[0], (const char*)arglist[1], (char*)arglist[2], (int)(intptr_t)arglist[3]);
}

////////////////////////////////////////////////////////////////////////////

static void* __vararg_JS_Window_GetRect(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Window_GetRect((void*)arglist[0], (int*)arglist[1], (int*)arglist[2], (int*)arglist[3], (int*)arglist[4]);
}

static void* __vararg_JS_Window_ScreenToClient(void** arglist, int numparms)
{
  JS_Window_ScreenToClient((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int*)arglist[3], (int*)arglist[4]);
  return NULL;
}

static void* __vararg_JS_Window_ClientToScreen(void** arglist, int numparms)
{
  JS_Window_ClientToScreen((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int*)arglist[3], (int*)arglist[4]);
  return NULL;
}

static void* __vararg_JS_Window_GetClientRect(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Window_GetClientRect((void*)arglist[0], (int*)arglist[1], (int*)arglist[2], (int*)arglist[3], (int*)arglist[4]);
}

static void* __vararg_JS_Window_GetClientSize(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_GetClientSize((void*)arglist[0], (int*)arglist[1], (int*)arglist[2]);
}

static void* __vararg_JS_Window_MonitorFromRect(void** arglist, int numparms)
{
	JS_Window_MonitorFromRect((int)(intptr_t)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (bool)arglist[4], (int*)arglist[5], (int*)arglist[6], (int*)arglist[7], (int*)arglist[8]);
	return nullptr;
}

static void* __vararg_JS_Window_GetViewportFromRect(void** arglist, int numparms)
{
	JS_Window_GetViewportFromRect((int)(intptr_t)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (bool)arglist[4], (int*)arglist[5], (int*)arglist[6], (int*)arglist[7], (int*)arglist[8]);
	return nullptr;
}

static void* __vararg_JS_Window_Update(void** arglist, int numparms)
{
	JS_Window_Update((HWND)arglist[0]);
	return nullptr;
}

static void* __vararg_JS_Window_InvalidateRect(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_InvalidateRect((HWND)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (bool)arglist[5]);
}

static void* __vararg_JS_Window_FromPoint(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Window_FromPoint((int)(intptr_t)arglist[0], (int)(intptr_t)arglist[1]);
}

static void* __vararg_JS_Window_GetParent(void** arglist, int numparms)
{
  return (void*)JS_Window_GetParent((void*)arglist[0]);
}

static void* __vararg_JS_Window_SetParent(void** arglist, int numparms)
{
  return (void*)JS_Window_SetParent((void*)arglist[0], (void*)arglist[1]);
}

static void* __vararg_JS_Window_IsChild(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Window_IsChild((void*)arglist[0], (void*)arglist[1]);
}

static void* __vararg_JS_Window_GetRelated(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Window_GetRelated((void*)arglist[0], (const char*)arglist[1]);
}

static void* __vararg_JS_Window_FindChildByID(void** arglist, int numparms)
{
	return (void*)JS_Window_FindChildByID((HWND)arglist[0], (int)(intptr_t)arglist[1]);
}


static void* __vararg_JS_Window_SetFocus(void** arglist, int numparms)
{
  JS_Window_SetFocus((void*)arglist[0]);
  return NULL;
}

static void* __vararg_JS_Window_GetFocus(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Window_GetFocus();
}

static void* __vararg_JS_Window_SetForeground(void** arglist, int numparms)
{
  JS_Window_SetForeground((void*)arglist[0]);
  return NULL;
}

static void* __vararg_JS_Window_GetForeground(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Window_GetForeground();
}

static void* __vararg_JS_Window_EnableMetal(void** arglist, int numparms)
{
  return(void*)(intptr_t)JS_Window_EnableMetal(arglist[0]);
}

static void* __vararg_JS_Window_Enable(void** arglist, int numparms)
{
  JS_Window_Enable((void*)arglist[0], (bool)arglist[1]);
  return NULL;
}

static void* __vararg_JS_Window_Destroy(void** arglist, int numparms)
{
  JS_Window_Destroy((void*)arglist[0]);
  return NULL;
}

static void* __vararg_JS_Window_Show(void** arglist, int numparms)
{
  JS_Window_Show((void*)arglist[0], (const char*)arglist[1]);
  return NULL;
}

static void* __vararg_JS_Window_IsVisible(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Window_IsVisible((void*)arglist[0]);
}

static void* __vararg_JS_Window_FindEx(void** arglist, int numparms)
{
	return (void*)JS_Window_FindEx((HWND)arglist[0], (HWND)arglist[1], (const char*)arglist[2], (const char*)arglist[3]);
}

static void* __vararg_JS_Window_FindTop(void** arglist, int numparms)
{
  return (void*)JS_Window_FindTop((const char*)arglist[0], (bool)arglist[1]);
}

static void* __vararg_JS_Window_Find(void** arglist, int numparms)
{
	return (void*)JS_Window_Find((const char*)arglist[0], (bool)arglist[1]);
}

static void* __vararg_JS_Window_FindChild(void** arglist, int numparms)
{
  return (void*)JS_Window_FindChild((void*)arglist[0], (const char*)arglist[1], (bool)arglist[2]);
}

////////////////////////////////////////////////

static void* __vararg_JS_Window_ArrayAllChild(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_ArrayAllChild((void*)arglist[0], (double*)arglist[1]);
}

static void* __vararg_JS_Window_ArrayAllTop(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_ArrayAllTop((double*)arglist[0]);
}

static void* __vararg_JS_Window_ArrayFind(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_ArrayFind((const char*)arglist[0], (bool)arglist[1], (double*)arglist[2]);
}

static void* __vararg_JS_MIDIEditor_ArrayAll(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_MIDIEditor_ArrayAll((double*)arglist[0]);
}

///////////////////////////////////////////////

static void* __vararg_JS_Window_ListAllChild(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_ListAllChild((void*)arglist[0], (char*)arglist[1], (int)(intptr_t)arglist[2]);
}

static void* __vararg_JS_Window_ListAllTop(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_ListAllTop((char*)arglist[0], (int)(intptr_t)arglist[1]);
}

static void* __vararg_JS_Window_ListFind(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_ListFind((const char*)arglist[0], (bool)arglist[1], (char*)arglist[2], (int)(intptr_t)arglist[3]);
}

static void* __vararg_JS_MIDIEditor_ListAll(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_MIDIEditor_ListAll((char*)arglist[0], (int)(intptr_t)arglist[1]);
}

////////////////////////////////////////////////
// Functions using SetWindowPos

/*static void* __vararg_JS_Window_SetPos(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_SetPos((void*)arglist[0], (const char*)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (int)(intptr_t)arglist[5], (const char*)arglist[6]);
}*/

static void* __vararg_JS_Window_Resize(void** arglist, int numparms)
{
  JS_Window_Resize((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2]);
  return NULL;
}

static void* __vararg_JS_Window_Move(void** arglist, int numparms)
{
  JS_Window_Move((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2]);
  return NULL;
}

static void* __vararg_JS_Window_SetPosition(void** arglist, int numparms)
{
	return (void*)JS_Window_SetPosition((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (numparms > 5) ? (char*)arglist[5] : nullptr, (numparms > 6) ? (char*)arglist[6] : nullptr);
}

static void* __vararg_JS_GetLevel(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_GetLevel(arglist[0]);
}

static void* __vararg_JS_Window_SetZOrder(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_SetZOrder((void*)arglist[0], (const char*)arglist[1], (numparms > 2) ? (void*)arglist[2] : 0);
}

static void* __vararg_JS_Window_GetLongPtr(void** arglist, int numparms)
{
	return JS_Window_GetLongPtr((void*)arglist[0], (const char*)arglist[1]);
}

static void* __vararg_JS_Window_GetLong(void** arglist, int numparms)
{
	JS_Window_GetLong((void*)arglist[0], (const char*)arglist[1], (double*)arglist[2]);
	return nullptr;
}

static void* __vararg_JS_Window_SetLong(void** arglist, int numparms)
{
	JS_Window_SetLong((void*)arglist[0], (const char*)arglist[1], arglist[2] ? *(double*)arglist[2] : 0.0, (double*)arglist[3]);
	return nullptr;
}

static void* __vararg_JS_Window_SetStyle(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_SetStyle((void*)arglist[0], (char*)arglist[1]);
}

static void* __vararg_JS_Window_SetOpacity(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_SetOpacity((HWND)arglist[0], (const char*)arglist[1], arglist[2] ? *(double*)arglist[2] : 0.0);
}

static void* __vararg_JS_Window_GetTitle(void** arglist, int numparms)
{
	JS_Window_GetTitle((void*)arglist[0], (char*)arglist[1], (int)(intptr_t)arglist[2]);
	return NULL;
}

static void* __vararg_JS_Window_SetTitle(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_SetTitle((void*)arglist[0], (const char*)arglist[1]);
}

static void* __vararg_JS_Window_GetClassName(void** arglist, int numparms)
{
	JS_Window_GetClassName((HWND)arglist[0], (char*)arglist[1], (int)(intptr_t)arglist[2]);
	return nullptr;
}

static void* __vararg_JS_Window_IsWindow(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Window_IsWindow((void*)arglist[0]);
}

static void* __vararg_JS_Window_HandleFromAddress(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Window_HandleFromAddress(arglist[0] ? *(double*)arglist[0] : 0.0);
}

static void* __vararg_JS_Window_AddressFromHandle(void** arglist, int numparms)
{
	JS_Window_AddressFromHandle(arglist[0], (double*)arglist[1]);
	return NULL;
}

static void* __vararg_JS_ArrayFromAddress(void** arglist, int numparms)
{
	return (void*)JS_ArrayFromAddress(arglist[0] ? *(double*)arglist[0] : 0.0);
}

static void* __vararg_JS_AddressFromArray(void** arglist, int numparms)
{
	JS_AddressFromArray((double*)arglist[0], (double*)arglist[1]);
	return NULL;
}

static void* __vararg_JS_WindowMessage_Post(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_WindowMessage_Post((void*)arglist[0], (const char*)arglist[1], arglist[2] ? *(double*)arglist[2] : 0.0, (int)(intptr_t)arglist[3], arglist[4] ? *(double*)arglist[4] : 0.0, (int)(intptr_t)arglist[5]);
}

static void* __vararg_JS_Window_OnCommand(void** arglist, int numparms)
{
  return (void*)JS_Window_OnCommand((void*)arglist[0], (int)(intptr_t)arglist[1]);
}

static void* __vararg_JS_WindowMessage_Send(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_WindowMessage_Send((void*)arglist[0], (const char*)arglist[1], arglist[2] ? *(double*)arglist[2] : 0.0, (int)(intptr_t)arglist[3], arglist[4] ? *(double*)arglist[4] : 0.0, (int)(intptr_t)arglist[5]);
}

static void* __vararg_JS_WindowMessage_Peek(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_WindowMessage_Peek((void*)arglist[0], (const char*)arglist[1], (bool*)arglist[2], (double*)arglist[3], (int*)arglist[4], (int*)arglist[5], (int*)arglist[6], (int*)arglist[7]);
}

static void* __vararg_JS_WindowMessage_Intercept(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_WindowMessage_Intercept((void*)arglist[0], (const char*)arglist[1], (bool)arglist[2]);
}

static void* __vararg_JS_WindowMessage_InterceptList(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_WindowMessage_InterceptList((void*)arglist[0], (const char*)arglist[1]);
}

static void* __vararg_JS_WindowMessage_PassThrough(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_WindowMessage_PassThrough(arglist[0], (const char*)arglist[1], (bool)arglist[2]);
}

static void* __vararg_JS_WindowMessage_ListIntercepts(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_WindowMessage_ListIntercepts((void*)arglist[0], (char*)arglist[1], (int)(intptr_t)arglist[2]);
}

static void* __vararg_JS_WindowMessage_Release(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_WindowMessage_Release((void*)arglist[0], (const char*)arglist[1]);
}

static void* __vararg_JS_WindowMessage_ReleaseWindow(void** arglist, int numparms)
{
  JS_WindowMessage_ReleaseWindow((void*)arglist[0]);
  return NULL;
}

static void* __vararg_JS_WindowMessage_ReleaseAll(void** arglist, int numparms)
{
  JS_WindowMessage_ReleaseAll();
  return NULL;
}

static void* __vararg_JS_Mouse_GetState(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Mouse_GetState((int)(intptr_t)arglist[0]);
}
/*
static void* __vararg_JS_Mouse_GetHistory(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Mouse_GetHistory((int)(intptr_t)arglist[0]);
}

static void* __vararg_JS_Mouse_ClearHistory(void** arglist, int numparms)
{
	JS_Mouse_ClearHistory();
	return nullptr;
}*/

static void* __vararg_JS_Mouse_SetPosition(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Mouse_SetPosition((int)(intptr_t)arglist[0], (int)(intptr_t)arglist[1]);
}

static void* __vararg_JS_Mouse_LoadCursor(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Mouse_LoadCursor((int)(intptr_t)arglist[0]);
}

static void* __vararg_JS_Mouse_LoadCursorFromFile(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_Mouse_LoadCursorFromFile((const char*)arglist[0], numparms > 1 ? (bool*)arglist[1] : NULL);
}

static void* __vararg_JS_Mouse_SetCursor(void** arglist, int numparms)
{
  JS_Mouse_SetCursor((void*)arglist[0]);
  return NULL;
}

static void* __vararg_JS_Mouse_GetCursor(void** arglist, int numparms)
{
	return JS_Mouse_GetCursor();
}

///////////////////////////////////////////////////////////

static void* __vararg_JS_Window_GetScrollInfo(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_GetScrollInfo((void*)arglist[0], (const char*)arglist[1], (int*)arglist[2], (int*)arglist[3], (int*)arglist[4], (int*)arglist[5], (int*)arglist[6]);
}

static void* __vararg_JS_Window_SetScrollPos(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_Window_SetScrollPos((void*)arglist[0], (const char*)arglist[1], (int)(intptr_t)arglist[2]);
}
///////////////////////////////////////////////////////////


static void* __vararg_JS_GDI_GetClientDC(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_GDI_GetClientDC((void*)arglist[0]);
}

static void* __vararg_JS_GDI_GetWindowDC(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_GDI_GetWindowDC((void*)arglist[0]);
}

static void* __vararg_JS_GDI_GetScreenDC(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_GDI_GetScreenDC();
}

static void* __vararg_JS_GDI_ReleaseDC(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_GDI_ReleaseDC((void*)arglist[0], (void*)arglist[1]);
}

/*
static void* __vararg_JS_GDI_DrawFocusRect(void** arglist, int numparms)
{
  JS_GDI_DrawFocusRect((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4]);
  return NULL;
}
*/

static void* __vararg_JS_GDI_CreateFillBrush(void** arglist, int numparms)
{
	return JS_GDI_CreateFillBrush((int)(intptr_t)arglist[0]);
}

static void* __vararg_JS_GDI_CreatePen(void** arglist, int numparms)
{
	return JS_GDI_CreatePen((int)(intptr_t)arglist[0], (int)(intptr_t)arglist[1]);
}

static void* __vararg_JS_GDI_CreateFont(void** arglist, int numparms)
{
	return JS_GDI_CreateFont((int)(intptr_t)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (bool)arglist[3], (bool)arglist[4], (bool)arglist[5], (const char*)arglist[6]);
}

static void* __vararg_JS_GDI_SelectObject(void** arglist, int numparms)
{
	return JS_GDI_SelectObject(arglist[0], arglist[1]);
}

static void* __vararg_JS_GDI_DeleteObject(void** arglist, int numparms)
{
	JS_GDI_DeleteObject(arglist[0]);
	return NULL;
}


static void* __vararg_JS_GDI_FillRect(void** arglist, int numparms)
{
	JS_GDI_FillRect((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4]);
	return NULL;
}

static void* __vararg_JS_GDI_FillRoundRect(void** arglist, int numparms)
{
	JS_GDI_FillRoundRect((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (int)(intptr_t)arglist[5], (int)(intptr_t)arglist[6]);
	return NULL;
}

static void* __vararg_JS_GDI_Line(void** arglist, int numparms)
{
	JS_GDI_Line((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4]);
	return NULL;
}

static void* __vararg_JS_GDI_Polyline(void** arglist, int numparms)
{
	JS_GDI_Polyline((void*)arglist[0], (const char*)arglist[1], (const char*)arglist[2], (int)(intptr_t)arglist[3]);
	return NULL;
}

static void* __vararg_JS_GDI_FillEllipse(void** arglist, int numparms)
{
	JS_GDI_FillEllipse((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4]);
	return NULL;
}

static void* __vararg_JS_GDI_FillPolygon(void** arglist, int numparms)
{
	JS_GDI_FillPolygon((void*)arglist[0], (const char*)arglist[1], (const char*)arglist[2], (int)(intptr_t)arglist[3]);
	return NULL;
}



static void* __vararg_JS_GDI_GetSysColor(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_GDI_GetSysColor((const char*)arglist[0]);
}

static void* __vararg_JS_GDI_SetTextBkMode(void** arglist, int numparms)
{
  JS_GDI_SetTextBkMode((void*)arglist[0], (int)(intptr_t)arglist[1]);
  return NULL;
}

static void* __vararg_JS_GDI_SetTextBkColor(void** arglist, int numparms)
{
  JS_GDI_SetTextBkColor((void*)arglist[0], (int)(intptr_t)arglist[1]);
  return NULL;
}

static void* __vararg_JS_GDI_SetTextColor(void** arglist, int numparms)
{
  JS_GDI_SetTextColor((void*)arglist[0], (int)(intptr_t)arglist[1]);
  return NULL;
}

static void* __vararg_JS_GDI_GetTextColor(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_GDI_GetTextColor((void*)arglist[0]);
}

static void* __vararg_JS_GDI_DrawText(void** arglist, int numparms)
{
  return (void*)(intptr_t)JS_GDI_DrawText((void*)arglist[0], (const char*)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (int)(intptr_t)arglist[5], (int)(intptr_t)arglist[6], (const char*)arglist[7]);
}

static void* __vararg_JS_GDI_SetPixel(void** arglist, int numparms)
{
  JS_GDI_SetPixel((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3]);
  return NULL;
}

/*
static void* __vararg_JS_GDI_MoveTo(void** arglist, int numparms)
{
  JS_GDI_MoveTo((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2]);
  return NULL;
}

static void* __vararg_JS_GDI_LineTo(void** arglist, int numparms)
{
  JS_GDI_LineTo((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2]);
  return NULL;
}
*/


static void* __vararg_JS_GDI_Blit(void** arglist, int numparms)
{
	JS_GDI_Blit((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (void*)arglist[3], (int)(intptr_t)arglist[4], (int)(intptr_t)arglist[5], (int)(intptr_t)arglist[6], (int)(intptr_t)arglist[7], (numparms > 8) ? (const char*)arglist[8] : nullptr);
	return NULL;
}

static void* __vararg_JS_GDI_StretchBlit(void** arglist, int numparms)
{
	JS_GDI_StretchBlit((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (void*)arglist[5], (int)(intptr_t)arglist[6], (int)(intptr_t)arglist[7], (int)(intptr_t)arglist[8], (int)(intptr_t)arglist[9], (numparms > 10) ? (const char*)arglist[10] : nullptr);
	return NULL;
}


////////////////////////////////////////////////////////////////////

static void* __vararg_JS_LICE_CreateBitmap(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_CreateBitmap((bool)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2]);
}

static void* __vararg_JS_LICE_ListAllBitmaps(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_ListAllBitmaps((char*)arglist[0], (int)(intptr_t)arglist[1]);
}
static void* __vararg_JS_LICE_ArrayAllBitmaps(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_ArrayAllBitmaps((double*)arglist[0]);
}

static void* __vararg_JS_LICE_GetHeight(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_GetHeight((void*)arglist[0]);
}

static void* __vararg_JS_LICE_GetWidth(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_GetWidth((void*)arglist[0]);
}

static void* __vararg_JS_LICE_GetDC(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_GetDC((void*)arglist[0]);
}

static void* __vararg_JS_LICE_DestroyBitmap(void** arglist, int numparms)
{
	JS_LICE_DestroyBitmap((LICE_IBitmap*)arglist[0]);
	return NULL;
}

static void* __vararg_JS_LICE_LoadPNG(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_LoadPNG((const char*)arglist[0]);
}

static void* __vararg_JS_LICE_WritePNG(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_WritePNG((const char*)arglist[0], (LICE_IBitmap*)arglist[1], (bool)arglist[2]);
}

static void* __vararg_JS_LICE_LoadJPG(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_LoadJPG((const char*)arglist[0]);
}

static void* __vararg_JS_LICE_WriteJPG(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_WriteJPG((const char*)arglist[0], (LICE_IBitmap*)arglist[1], (int)(intptr_t)arglist[2], (numparms>3 && arglist[3]) ? *(bool*)arglist[3] : false);
}

static void* __vararg_JS_LICE_Blit(void** arglist, int numparms)
{
	JS_LICE_Blit((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (void*)arglist[3], (int)(intptr_t)arglist[4], (int)(intptr_t)arglist[5], (int)(intptr_t)arglist[6], (int)(intptr_t)arglist[7], arglist[8] ? *(double*)arglist[8] : 0.0, (const char*)arglist[9]);
	return NULL;
}

static void* __vararg_JS_LICE_RotatedBlit(void** arglist, int numparms)
{
	JS_LICE_RotatedBlit((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (void*)arglist[5], arglist[6] ? *(double*)arglist[6] : 0.0, arglist[7] ? *(double*)arglist[7] : 0.0, arglist[8] ? *(double*)arglist[8] : 0.0, arglist[9] ? *(double*)arglist[9] : 0.0, arglist[10] ? *(double*)arglist[10] : 0.0, arglist[11] ? *(double*)arglist[11] : 0.0, arglist[12] ? *(double*)arglist[12] : 0.0, (bool)arglist[13], arglist[14] ? *(double*)arglist[14] : 0.0, (const char*)arglist[15]);
	return NULL;
}

static void* __vararg_JS_LICE_ScaledBlit(void** arglist, int numparms)
{
	JS_LICE_ScaledBlit((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (void*)arglist[5], arglist[6] ? *(double*)arglist[6] : 0.0, arglist[7] ? *(double*)arglist[7] : 0.0, arglist[8] ? *(double*)arglist[8] : 0.0, arglist[9] ? *(double*)arglist[9] : 0.0, arglist[10] ? *(double*)arglist[10] : 0.0, (const char*)arglist[11]);
	return NULL;
}

static void* __vararg_JS_LICE_Blur(void** arglist, int numparms)
{
	JS_LICE_Blur((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (void*)arglist[3], (int)(intptr_t)arglist[4], (int)(intptr_t)arglist[5], (int)(intptr_t)arglist[6], (int)(intptr_t)arglist[7]);
	return NULL;
}

static void* __vararg_JS_LICE_IsFlipped(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_IsFlipped((void*)arglist[0]);
}

static void* __vararg_JS_LICE_Resize(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_Resize((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2]);
}

static void* __vararg_JS_LICE_Clear(void** arglist, int numparms)
{
	JS_LICE_Clear((void*)arglist[0], (int)(intptr_t)arglist[1]);
	return NULL;
}

static void* __vararg_JS_LICE_CreateFont(void** arglist, int numparms)
{
	return JS_LICE_CreateFont();
}

static void* __vararg_JS_LICE_DestroyFont(void** arglist, int numparms)
{
	JS_LICE_DestroyFont((void*)arglist[0]);
	return NULL;
}

static void* __vararg_JS_LICE_SetFontFromGDI(void** arglist, int numparms)
{
	JS_LICE_SetFontFromGDI((void*)arglist[0], (void*)arglist[1], (const char*)arglist[2]);
	return NULL;
}

static void* __vararg_JS_LICE_SetFontColor(void** arglist, int numparms)
{
	JS_LICE_SetFontColor((void*)arglist[0], (int)(intptr_t)arglist[1]);
	return NULL;
}

static void* __vararg_JS_LICE_SetFontBkColor(void** arglist, int numparms)
{
	JS_LICE_SetFontBkColor((void*)arglist[0], (int)(intptr_t)arglist[1]);
	return NULL;
}

static void* __vararg_JS_LICE_MeasureText(void** arglist, int numparms)
{
	JS_LICE_MeasureText((const char*)arglist[0], (int*)arglist[1], (int*)arglist[2]);
	return NULL;
}

static void* __vararg_JS_LICE_DrawText(void** arglist, int numparms)
{
	JS_LICE_DrawText((void*)arglist[0], (void*)arglist[1], (const char*)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (int)(intptr_t)arglist[5], (int)(intptr_t)arglist[6], (int)(intptr_t)arglist[7]);
	return NULL;
}

static void* __vararg_JS_LICE_DrawChar(void** arglist, int numparms)
{
	JS_LICE_DrawChar((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (char)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], arglist[5] ? *(double*)arglist[5] : 0.0, (const char*)arglist[6]);
	return NULL;
}

static void* __vararg_JS_LICE_GradRect(void** arglist, int numparms)
{
	JS_LICE_GradRect((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4],
					arglist[5] ? *(double*)arglist[5] : 0.0, arglist[6] ? *(double*)arglist[6] : 0.0, arglist[7] ? *(double*)arglist[7] : 0.0, arglist[8] ? *(double*)arglist[8] : 0.0, arglist[9] ? *(double*)arglist[9] : 0.0, arglist[10] ? *(double*)arglist[10] : 0.0, arglist[11] ? *(double*)arglist[11] : 0.0, arglist[12] ? *(double*)arglist[12] : 0.0, arglist[13] ? *(double*)arglist[13] : 0.0, arglist[14] ? *(double*)arglist[14] : 0.0, arglist[15] ? *(double*)arglist[15] : 0.0, arglist[16] ? *(double*)arglist[16] : 0.0,
					(const char*)arglist[17]);
	return NULL;
}

static void* __vararg_JS_LICE_FillRect(void** arglist, int numparms)
{
	JS_LICE_FillRect((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (int)(intptr_t)arglist[5], arglist[6] ? *(double*)arglist[6] : 0.0, (const char*)arglist[7]);
	return NULL;
}

static void* __vararg_JS_LICE_FillTriangle(void** arglist, int numparms)
{
	JS_LICE_FillTriangle((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (int)(intptr_t)arglist[5], (int)(intptr_t)arglist[6], (int)(intptr_t)arglist[7], arglist[8] ? *(double*)arglist[8] : 0.0, (const char*)arglist[9]);
	return NULL;
}

static void* __vararg_JS_LICE_FillPolygon(void** arglist, int numparms)
{
	JS_LICE_FillPolygon((void*)arglist[0], (const char*)arglist[1], (const char*)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], arglist[5] ? *(double*)arglist[5] : 0.0, (const char*)arglist[6]);
	return NULL;
}

static void* __vararg_JS_LICE_FillCircle(void** arglist, int numparms)
{
	JS_LICE_FillCircle((void*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0, arglist[2] ? *(double*)arglist[2] : 0.0, arglist[3] ? *(double*)arglist[3] : 0.0, (int)(intptr_t)arglist[4], arglist[5] ? *(double*)arglist[5] : 0.0, (const char*)arglist[6], (bool)arglist[7]);
	return NULL;
}

static void* __vararg_JS_LICE_Line(void** arglist, int numparms)
{
	JS_LICE_Line((void*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0, arglist[2] ? *(double*)arglist[2] : 0.0, arglist[3] ? *(double*)arglist[3] : 0.0, arglist[4] ? *(double*)arglist[4] : 0.0, (int)(intptr_t)arglist[5], arglist[6] ? *(double*)arglist[6] : 0.0, (const char*)arglist[7], (bool)arglist[8]);
	return NULL;
}

static void* __vararg_JS_LICE_Bezier(void** arglist, int numparms)
{
	JS_LICE_Bezier((void*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0, arglist[2] ? *(double*)arglist[2] : 0.0, arglist[3] ? *(double*)arglist[3] : 0.0, arglist[4] ? *(double*)arglist[4] : 0.0, arglist[5] ? *(double*)arglist[5] : 0.0, arglist[6] ? *(double*)arglist[6] : 0.0, arglist[7] ? *(double*)arglist[7] : 0.0, arglist[8] ? *(double*)arglist[8] : 0.0, arglist[9] ? *(double*)arglist[9] : 0.0, (int)(intptr_t)arglist[10], arglist[11] ? *(double*)arglist[11] : 0.0, (const char*)arglist[12], (bool)arglist[13]);
	return NULL;
}

static void* __vararg_JS_LICE_Arc(void** arglist, int numparms)
{
	JS_LICE_Arc((void*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0, arglist[2] ? *(double*)arglist[2] : 0.0, arglist[3] ? *(double*)arglist[3] : 0.0, arglist[4] ? *(double*)arglist[4] : 0.0, arglist[5] ? *(double*)arglist[5] : 0.0, (int)(intptr_t)arglist[6], arglist[7] ? *(double*)arglist[7] : 0.0, (const char*)arglist[8], (bool)arglist[9]);
	return NULL;
}

static void* __vararg_JS_LICE_Circle(void** arglist, int numparms)
{
	JS_LICE_Circle((void*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0, arglist[2] ? *(double*)arglist[2] : 0.0, arglist[3] ? *(double*)arglist[3] : 0.0, (int)(intptr_t)arglist[4], arglist[5] ? *(double*)arglist[5] : 0.0, (const char*)arglist[6], (bool)arglist[7]);
	return NULL;
}

static void* __vararg_JS_LICE_RoundRect(void** arglist, int numparms)
{
	JS_LICE_RoundRect((void*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0, arglist[2] ? *(double*)arglist[2] : 0.0, arglist[3] ? *(double*)arglist[3] : 0.0, arglist[4] ? *(double*)arglist[4] : 0.0, (int)(intptr_t)arglist[5], (int)(intptr_t)arglist[6], arglist[7] ? *(double*)arglist[7] : 0.0, (const char*)arglist[8], (bool)arglist[9]);
	return NULL;
}

static void* __vararg_JS_LICE_GetPixel(void** arglist, int numparms)
{
	JS_LICE_GetPixel((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (double*)arglist[3]);
	return NULL;
}

static void* __vararg_JS_LICE_PutPixel(void** arglist, int numparms)
{
	JS_LICE_PutPixel((void*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], arglist[3] ? *(double*)arglist[3] : 0.0, arglist[4] ? *(double*)arglist[4] : 0.0, (const char*)arglist[5]);
	return NULL;
}



static void* __vararg_JS_LICE_SetAlphaFromColorMask(void** arglist, int numparms)
{
	JS_LICE_SetAlphaFromColorMask((LICE_IBitmap*)arglist[0], (LICE_pixel)(intptr_t)arglist[1]);
	return NULL;
}

static void* __vararg_JS_LICE_AlterBitmapHSV(void** arglist, int numparms)
{
	JS_LICE_AlterBitmapHSV((LICE_IBitmap*)arglist[0], arglist[1] ? *(double*)arglist[1] : 0.0, arglist[2] ? *(double*)arglist[2] : 0.0, arglist[3] ? *(double*)arglist[3] : 0.0);
	return NULL;
}

static void* __vararg_JS_LICE_AlterRectHSV(void** arglist, int numparms)
{
	JS_LICE_AlterRectHSV((LICE_IBitmap*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], arglist[5] ? *(double*)arglist[5] : 0.0, arglist[6] ? *(double*)arglist[6] : 0.0, arglist[7] ? *(double*)arglist[7] : 0.0);
	return NULL;
}

static void* __vararg_JS_LICE_ProcessRect(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_LICE_ProcessRect((LICE_IBitmap*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], (const char*)arglist[5], (numparms > 6 && arglist[6]) ? *(double*)arglist[6] : 0.0);
}


//////////////////////////////////////////////////////////////////////////////////

static void* __vararg_JS_Window_AttachTopmostPin(void** arglist, int numparms)
{
	JS_Window_AttachTopmostPin(arglist[0]);
	return NULL;
}

static void* __vararg_JS_Window_AttachResizeGrip(void** arglist, int numparms)
{
	JS_Window_AttachResizeGrip(arglist[0]);
	return NULL;
}


//////////////////////////////////////////////////////////////


static void* __vararg_JS_ListView_GetItemCount(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_ListView_GetItemCount((HWND)arglist[0]);
}

static void* __vararg_JS_ListView_GetSelectedCount(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_ListView_GetSelectedCount((HWND)arglist[0]);
}

static void* __vararg_JS_ListView_GetFocusedItem(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_ListView_GetFocusedItem((HWND)arglist[0], (char*)arglist[1], (int)(intptr_t)arglist[2]);
}

static void* __vararg_JS_ListView_EnumSelItems(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_ListView_EnumSelItems((HWND)arglist[0], (int)(intptr_t)arglist[1]);
}

static void* __vararg_JS_ListView_EnsureVisible(void** arglist, int numparms)
{
	JS_ListView_EnsureVisible((HWND)arglist[0], (int)(intptr_t)arglist[1], (bool)(intptr_t)arglist[2]);
	return nullptr;
}

static void* __vararg_JS_ListView_GetItem(void** arglist, int numparms)
{
	JS_ListView_GetItem((HWND)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (char*)arglist[3], (int)(intptr_t)arglist[4], (int*)arglist[5]);
	return NULL;
}

static void* __vararg_JS_ListView_GetItemText(void** arglist, int numparms)
{
	JS_ListView_GetItemText((HWND)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2], (char*)arglist[3], (int)(intptr_t)arglist[4]);
	return NULL;
}

static void* __vararg_JS_ListView_GetItemState(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_ListView_GetItemState((HWND)arglist[0], (int)(intptr_t)arglist[1]);
}

static void* __vararg_JS_ListView_ListAllSelItems(void** arglist, int numparms)
{
	return (void*)(intptr_t)JS_ListView_ListAllSelItems((HWND)arglist[0], (char*)arglist[1], (int)(intptr_t)arglist[2]);
}


//////////////////////////////////////////////////////////////////////

static void* __vararg_Xen_AudioWriter_Create(void** arglist, int numparms)
{
	if (numparms > 2)
		return Xen_AudioWriter_Create((const char*)arglist[0], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[2]);
	return nullptr;
}

static void* __vararg_Xen_AudioWriter_Destroy(void** arglist, int numparms)
{
	if (numparms > 0)
		Xen_AudioWriter_Destroy((AudioWriter*)arglist[0]);
	return nullptr;
}

static void* __vararg_Xen_AudioWriter_Write(void** arglist, int numparms)
{
	int result = 0;
	if (numparms > 3)
		result = Xen_AudioWriter_Write(
		(AudioWriter*)arglist[0], (double*)arglist[2], (int)(intptr_t)arglist[1], (int)(intptr_t)arglist[3]);
	return (void*)(intptr_t)result;
}

static void* __vararg_Xen_GetMediaSourceSamples(void** arglist, int numparms)
{
	int result = 0;
	if (numparms > 6)
		result = Xen_GetMediaSourceSamples((PCM_source*)arglist[0], (double*)arglist[1], (int)(intptr_t)arglist[2],
		(int)(intptr_t)arglist[3], (int)(intptr_t)arglist[4], *(double*)arglist[5], *(double*)arglist[6]);
	return (void*)(intptr_t)result;
}

static void* __vararg_Xen_StartSourcePreview(void** arglist, int numparms)
{
	int result = 0;
	if (numparms == 4)
	{
		int outchanindex = 0;
		if (arglist[3] != nullptr)
			outchanindex = *(int*)arglist[3];
		result = Xen_StartSourcePreview((PCM_source*)arglist[0], *(double*)arglist[1], (int)(INT_PTR)arglist[2],
				outchanindex);
	}
	return (void*)(INT_PTR)result;
}

static void* __vararg_Xen_StopSourcePreview(void** arglist, int numparms)
{
	int result = 0;
	if (numparms > 0)
	{
		result = Xen_StopSourcePreview((int)(INT_PTR)arglist[0]);
	}
	return (void*)(INT_PTR)result;
}
