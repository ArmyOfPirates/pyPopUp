/*
pyPopUp
(c) 2013 ArmyOfPirates

A simple popup system for python on windows.
*/

#include "popup.h"

static void startFade(){
	if(wait_while_idle && isIdle())
	{
		idleTimer.Start(1000);
		return;
	}
	opacity = default_opacity;
	fadeTimer.Start(25);
}

static void fadeStep(){
	opacity -= 5;
	if(opacity <= 0){
		EnterCriticalSection( &cs );
		fadeTimer.Stop();
		ShowWindow(m_hWnd, SW_HIDE);
		if(!popup_queue.empty()){
			show(popup_queue[0].text, popup_queue[0].time);
			popup_queue.erase(popup_queue.begin());
		}
		LeaveCriticalSection( &cs );
	}
	else{
		SetLayeredWindowAttributes(m_hWnd, 0, (255 * opacity) / 100, LWA_ALPHA);
	}
}

static void checkIdle(){
	if(!isIdle())
	{
		idleTimer.Stop();
		showTimer.Start(default_time); // get interval from timer?
	}
}

static void checkAllowed(){
	if(popupAllowed())
	{
		EnterCriticalSection( &cs );
		waitTimer.Stop();
		if(!popup_queue.empty()){
			show(popup_queue[0].text, popup_queue[0].time);
			popup_queue.erase(popup_queue.begin());
		}
		LeaveCriticalSection( &cs );
	}
}

static LRESULT WINAPI CustomWndProc(HWND lhWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdcStatic;
	switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
	case WM_CTLCOLORSTATIC:
		hdcStatic = (HDC) wParam; 
		SetTextColor(hdcStatic, textColor);    
		SetBkMode (hdcStatic, TRANSPARENT);
		return (LRESULT)GetStockObject(NULL_BRUSH);
	}
	return DefWindowProc(lhWnd, message, wParam, lParam);
}

static bool popupAllowed()
{
    if(!do_not_disturb)
		return true;
	if(isVistaOrHigher)
	{
		QUERY_USER_NOTIFICATION_STATE quns;
		SHQueryUserNotificationState(&quns);
		return (quns == QUNS_ACCEPTS_NOTIFICATIONS);
	}
	else
	{
		HWND hWnd = GetForegroundWindow();

		if(!hWnd)
			return false;

		if(hWnd == GetDesktopWindow() || hWnd == GetShellWindow())//if(!IsWindowVisible(hWnd) || IsIconic(hWnd) || !IsZoomed(hWnd))
			return false;

		RECT rc;
		if(!GetWindowRect(hWnd,&rc))
			return false;

		return !(rc.right - rc.left  >= GetSystemMetrics(SM_CXSCREEN) &&  rc.bottom - rc.top >= GetSystemMetrics(SM_CYSCREEN));
	}
}

static bool IsWindowsVistaOrHigher()
{
   OSVERSIONINFO osvi;
   ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
   osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
   GetVersionEx(&osvi);
   return osvi.dwMajorVersion >= 6;
}

static bool isIdle()
{
	LASTINPUTINFO inp;
	DWORD lastinp;
	inp.cbSize = sizeof(inp);
	GetLastInputInfo(&inp);
	lastinp = GetTickCount() - inp.dwTime;
	if(lastinp > 60000)
		return true;
	return false;
}

static void position_popup(int width, int height){
	int x, y;
	HWND hwnd;
	HMONITOR hMonitor;
	MONITORINFO mi;
	RECT        rc, prc;
	
	if(follow_active_screen){
		hwnd = GetForegroundWindow();
	}
	else{
		hwnd = m_hWnd;
	}
	GetWindowRect(hwnd, &prc);
	
	// 
	// get the nearest monitor to the passed rect.
	// 
	hMonitor = MonitorFromRect(&prc, MONITOR_DEFAULTTONEAREST);

	// 
	// get the work area or entire monitor rect. 
	// 
	mi.cbSize = sizeof(mi);
	GetMonitorInfo(hMonitor, &mi);

	if (use_workarea)
		rc = mi.rcWork;
	else
		rc = mi.rcMonitor;
		
	switch(current_position){
	case POSITION_CUSTOM:
		x = pos_x;
		y = pos_y;
		break;
	case POSITION_CENTER:
        x = rc.left + (rc.right  - rc.left - width) / 2;
        y = rc.top  + (rc.bottom - rc.top  - height) / 2;
		break;
	case POSITION_TOP_LEFT:
		x = rc.left + margin;
		y = rc.top + margin;
		break;
	case POSITION_TOP_RIGHT:
		x = rc.right - margin - width;
		y = rc.top + margin;
		break;
	case POSITION_BOTTOM_LEFT:
		x = rc.left + margin;
		y = rc.bottom - margin - height;
		break;
	case POSITION_BOTTOM_RIGHT:
		x = rc.right - margin - width;
		y = rc.bottom - margin - height;
		break;
	}
	SetWindowPos(m_hWnd, HWND_TOPMOST, x, y, width, height, 0);
}

static void show(LPWSTR text, Py_ssize_t time){
	if(!popupAllowed())
	{
		pqueue item;
		item.text = text;
		item.time = (unsigned int)time;
		popup_queue.insert(popup_queue.begin(), item);
		waitTimer.Start(1000);
		return;
	}
	HDC hdc;
	HRGN hRegion;
	HFONT hFontOld;
	RECT box = {};
	int pcch;
	pcch = lstrlen(text);
	//LPWSTR lpszString1 = new WCHAR[pcch];
	//lpszString1 = (LPWSTR)text;
	
	hdc = GetDC(hwndLabel);
	hFontOld = (HFONT)SelectObject(hdc, hfFont);
	//GetTextExtentPoint32 does not do multiline
	DrawText(hdc, text, pcch, &box, DT_CALCRECT);
	hfFont = (HFONT)SelectObject(hdc, hFontOld);
	
	ReleaseDC(NULL, hdc);
	
	SetWindowPos(hwndLabel, 0, 0, 0, box.right, box.bottom, SWP_NOZORDER | SWP_NOMOVE);
	position_popup(box.right+18, box.bottom+4);
	hRegion = CreateRoundRectRgn (0, 0, box.right+18, box.bottom+4, 15, 15);
	SetWindowRgn(m_hWnd, hRegion, TRUE);
	
	SendMessage(hwndLabel, WM_SETTEXT, NULL, (LPARAM) text);
	
	fadeTimer.Stop();
	SetLayeredWindowAttributes(m_hWnd, 0, (255 * default_opacity) / 100, LWA_ALPHA);
	InvalidateRect(m_hWnd, NULL, TRUE);
	UpdateWindow(m_hWnd);
	ShowWindow(m_hWnd, SW_SHOW);
	showTimer.Start((unsigned int)time, false, true);
}

static unsigned __stdcall boot(void* pArguments){
	HINSTANCE hInstance = HINST_THISCOMPONENT;
	HDC hdc;
	HFONT hFontOld;
	WCHAR *g_wcpAppName  = L"pyPopUp";
	InitializeCriticalSection( &cs );
	
	WNDCLASSEX wc = {sizeof(WNDCLASSEX),               // cbSize
					  0,                               // style
					  CustomWndProc,                   // lpfnWndProc
					  0,                               // cbClsExtra
					  0,                               // cbWndExtra
					  hInstance,                       // hInstance
					  LoadIcon(NULL, IDI_APPLICATION), // hIcon
					  LoadCursor(NULL, IDC_ARROW),     // hCursor
					  (HBRUSH) COLOR_WINDOW,           // hbrBackground
					  NULL,                            // lpszMenuName
					  g_wcpAppName,                    // lpszClassName
					  LoadIcon(NULL, IDI_APPLICATION)};// hIconSm

	RegisterClassEx(&wc);

	m_hWnd = CreateWindowEx(
		WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
		g_wcpAppName,
		g_wcpAppName,
		WS_POPUP | WS_DISABLED,
		0, // x
		0, // y
		0, // width
		0, // height
		NULL,
		NULL,
		hInstance,
		NULL);
	
	SetLayeredWindowAttributes(m_hWnd, 0, (255 * default_opacity) / 100, LWA_ALPHA);

	hwndLabel = CreateWindow(L"STATIC",L"",
		WS_VISIBLE | WS_CHILD | SS_LEFT,
		9, 0, 0, 0,
		m_hWnd, (HMENU)IDC_POPUP_TEXT, hInstance, NULL);
		
	hfFont = CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
            DEFAULT_PITCH, L"");
	SendMessage(hwndLabel, WM_SETFONT, (WPARAM)hfFont, NULL);
	textColor = RGB(0, 0, 0);
		
	hdc = GetDC(hwndLabel);
	SetBkMode(hdc, TRANSPARENT);
	hFontOld = (HFONT)SelectObject(hdc, hfFont);
	hfFont = (HFONT)SelectObject(hdc, hFontOld);
	ReleaseDC(NULL, hdc);
	
	ShowWindow(m_hWnd, SW_SHOW);
	ShowWindow(m_hWnd, SW_HIDE);
	
	showTimer.OnTimedEvent = startFade;
	fadeTimer.OnTimedEvent = fadeStep;
	idleTimer.OnTimedEvent = checkIdle;
	waitTimer.OnTimedEvent = checkAllowed;

    MSG msg;
	startup_done = true;
    while(GetMessage(&msg, m_hWnd, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
	return 0;
}

/* Python functions */

static PyObject *
popup_setOption(PyObject *self, PyObject *args)
{
	PyObject *ptype, *pvalue = NULL;
	if(!PyArg_UnpackTuple(args, "setOption", 2, 2, &ptype, &pvalue)){
		return NULL;
	}
	int type = (int)PyInt_AsSsize_t(ptype);
	int intvalue;
	HBRUSH brush;
	HGDIOBJ oldbrush;
	
	switch(type){
	case OPTION_OPACITY:
		if(!PyInt_Check(pvalue)){
			PyErr_SetString(PyExc_TypeError, "expecting integer");
			return NULL;
		}
		intvalue = (int)PyInt_AsSsize_t(pvalue);
		if(intvalue < 0 || intvalue > 100){
			PyErr_SetString(PyExc_ValueError, "expecting integer in range 0-100");
			return NULL;
		}
		default_opacity = intvalue;
		break;
	case OPTION_USE_WORKAREA:
		if(!PyInt_Check(pvalue)){
			PyErr_SetString(PyExc_ValueError, "expecting True/False/1/0");
			return NULL;
		}
		use_workarea = (PyInt_AsSsize_t(pvalue)!=0); // C4800
		break;
	case OPTION_FOLLOW_ACTIVE_SCREEN:
		if(!PyInt_Check(pvalue)){
			PyErr_SetString(PyExc_ValueError, "expecting True/False/1/0");
			return NULL;
		}
		follow_active_screen = (PyInt_AsSsize_t(pvalue)!=0); // C4800
		break;
	case OPTION_TEXT_COLOR:
		if(!PyTuple_Check(pvalue)){
			return NULL;
		}
		if(PyTuple_Size(pvalue) != 3){
			PyErr_SetString(PyExc_ValueError, "expecting seperate RGB values in each tuple...");
			return NULL;
		}
		textColor = RGB(PyInt_AsSsize_t(PyTuple_GetItem(pvalue, 0)), PyInt_AsSsize_t(PyTuple_GetItem(pvalue, 1)), PyInt_AsSsize_t(PyTuple_GetItem(pvalue, 2)));
		InvalidateRect(m_hWnd, NULL, TRUE);
		UpdateWindow(m_hWnd);
		break;
	case OPTION_BACKGROUND_COLOR:
		if(!PyTuple_Check(pvalue)){
			return NULL;
		}
		if(PyTuple_Size(pvalue) != 3){
			PyErr_SetString(PyExc_ValueError, "expecting seperate RGB values in each tuple...");
			return NULL;
		}
		brush = CreateSolidBrush(RGB(PyInt_AsSsize_t(PyTuple_GetItem(pvalue, 0)), PyInt_AsSsize_t(PyTuple_GetItem(pvalue, 1)), PyInt_AsSsize_t(PyTuple_GetItem(pvalue, 2))));
		oldbrush = (HGDIOBJ)GetClassLongPtr(m_hWnd, GCLP_HBRBACKGROUND);
		SetClassLongPtr(m_hWnd, GCLP_HBRBACKGROUND, (LONG)brush);
		DeleteObject(oldbrush);
		InvalidateRect(m_hWnd, NULL, TRUE);
		UpdateWindow(m_hWnd);
		break;
	case OPTION_DEFAULT_TIME:
		if(!PyInt_Check(pvalue)){
			PyErr_SetString(PyExc_TypeError, "expecting integer");
			return NULL;
		}
		intvalue = (int)PyInt_AsSsize_t(pvalue);
		if(intvalue <= 0){
			PyErr_SetString(PyExc_ValueError, "time must be greater than zero");
			return NULL;
		}
		default_time = intvalue;
		break;
	case OPTION_MARGIN:
		if(!PyInt_Check(pvalue)){
			PyErr_SetString(PyExc_TypeError, "expecting integer");
			return NULL;
		}
		intvalue = (int)PyInt_AsSsize_t(pvalue);
		if(intvalue < 0){
			PyErr_SetString(PyExc_ValueError, "margin must be positive or zero");
			return NULL;
		}
		margin = intvalue;
		break;
	case OPTION_WAIT_WHEN_USER_IDLE:
		if(!PyInt_Check(pvalue)){
			PyErr_SetString(PyExc_ValueError, "expecting True/False/1/0");
			return NULL;
		}
		wait_while_idle = (PyInt_AsSsize_t(pvalue)!=0); // C4800
		break;
	case OPTION_DO_NOT_DISTURB:
		if(!PyInt_Check(pvalue)){
			PyErr_SetString(PyExc_ValueError, "expecting True/False/1/0");
			return NULL;
		}
		do_not_disturb = (PyInt_AsSsize_t(pvalue)!=0); // C4800
		break;
	default:
		PyErr_SetString(PyExc_ValueError, "Unknown option");
	}
	Py_RETURN_NONE;
}

static PyObject *
popup_setPosition(PyObject *self, PyObject *args)
{
	PyObject *ptype, *px, *py = NULL;
	if(!PyArg_UnpackTuple(args, "setPosition", 1, 3, &ptype, &px, &py)){
		return NULL;
	}
	int type = (int)PyInt_AsSsize_t(ptype);
	switch(type){
	case POSITION_CUSTOM:
		if(!PyInt_Check(px) || !PyInt_Check(px)){
			PyErr_SetString(PyExc_TypeError, "expecting two integers for X and Y");
			return NULL;
		}
		pos_x = (int)PyInt_AsSsize_t(px);
		pos_y = (int)PyInt_AsSsize_t(py);
	case POSITION_CENTER:
	case POSITION_TOP_LEFT:
	case POSITION_TOP_RIGHT:
	case POSITION_BOTTOM_LEFT:
	case POSITION_BOTTOM_RIGHT:
		current_position = type;
		break;
	default:
		PyErr_SetString(PyExc_ValueError, "Unknown position type");
	}
	Py_RETURN_NONE;
}

static PyObject *
popup_setFont(PyObject *self, PyObject *args)
{
	PyObject *name, *size = NULL;
	if (!PyArg_UnpackTuple(args, "setFont", 2, 2, &name, &size)) {
		return NULL;
	}
	if(!PyUnicode_Check(name)){
		PyErr_SetString(PyExc_TypeError, "expecting an unicode string");
		return NULL;
	}
	if(!PyInt_Check(size)){
		PyErr_SetString(PyExc_TypeError, "expecting an integer");
		return NULL;
	}
	unsigned int fsize = (unsigned int)PyInt_AsSsize_t(size);
	if(fsize == 0){
		PyErr_SetString(PyExc_ValueError, "size must be bigger than 0");
		return NULL;
	}
	LPCWSTR font = (LPCWSTR)PyUnicode_AS_UNICODE(name);
	HFONT tempFont = CreateFont(fsize, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, font);
	SendMessage(hwndLabel, WM_SETFONT, (WPARAM)tempFont, NULL);
	DeleteObject(hfFont);
	hfFont = tempFont;
	Py_RETURN_NONE;
}

static PyObject *
popup_show(PyObject *self, PyObject *args)
{
	PyObject *str, *time = NULL;
	const Py_UNICODE *text;
	Py_ssize_t dtime = 0;
	if (!PyArg_UnpackTuple(args, "show", 1, 2, &str, &time)) {
		return NULL;
	}
	if(!PyUnicode_Check(str)){
		PyErr_SetString(PyExc_TypeError, "expecting an unicode string");
		return NULL;
	}if(time == NULL){
		dtime = default_time;
	}
	else if(!PyInt_Check(time)){
		PyErr_SetString(PyExc_TypeError, "expecting an integer");
		return NULL;
	}
	else{
		dtime = PyInt_AsSsize_t(time);
	}
	
	if(dtime <= 0){
		PyErr_SetString(PyExc_ValueError, "time must be bigger than 0");
		return NULL;
	}
	text = PyUnicode_AS_UNICODE(str);
	int pcch;
	pcch = lstrlen(text);
	LPWSTR lpszString1 = new WCHAR[pcch];
	lpszString1 = (LPWSTR)text;
	EnterCriticalSection( &cs );
	if(showTimer.Enabled() || fadeTimer.Enabled() || idleTimer.Enabled() || waitTimer.Enabled()){
		pqueue item;
		item.text = lpszString1;
		item.time = (unsigned int)dtime;
		popup_queue.push_back(item);
	} else {
		show(lpszString1, dtime);
	}
	LeaveCriticalSection( &cs );
	Py_RETURN_NONE;
}

static PyObject *
popup_wait(PyObject *self, PyObject *args)
{
	while(showTimer.Enabled() || fadeTimer.Enabled() || !popup_queue.empty())
		Sleep(1);
	Py_RETURN_NONE;
}

static PyObject *
popup_clear(PyObject *self, PyObject *args)
{
	EnterCriticalSection( &cs );
	if(showTimer.Enabled())
		showTimer.Stop();
	if(fadeTimer.Enabled())
		fadeTimer.Stop();
	if(!popup_queue.empty())
		popup_queue.clear();
	ShowWindow(m_hWnd, SW_HIDE);
	LeaveCriticalSection( &cs );
	Py_RETURN_NONE;
}

static PyMethodDef popup_methods[] = {
	{"setOption", popup_setOption, METH_VARARGS, "setOption(name, value)\nUse one of the OPTION_* constants for the name."},
	{"setFont", popup_setFont, METH_VARARGS, "setFont(name, size)\nSet font to the specified name(unicode!) and size."},
	{"setPosition", popup_setPosition, METH_VARARGS, "setPosition(name, x, y)\nUse one of the POSITION_* constants. x and y are only required for POSITION_CUSTOM."},
	{"show", popup_show, METH_VARARGS, "show(text, [time])\nShow a popup (for time milliseconds). Text must be in unicode."},
	{"wait", popup_wait, METH_VARARGS, "wait()\nHalts until popup queue is empty and all popups have been shown."},
	{"clear", popup_clear, METH_VARARGS, "clear()\nRemoves all popups from queue and screen."},
	{NULL, NULL}
};

PyMODINIT_FUNC
initpopup(void)
{
	HANDLE hThread;
    unsigned threadID;
	PyObject *m;
	m = Py_InitModule("popup", popup_methods);
	if(m == NULL)
		return;
	default_opacity = 100;
	startup_done = false;
	hThread = (HANDLE)_beginthreadex( NULL, 0, &boot, NULL, 0, &threadID );
	
	PyModule_AddIntConstant (m, "POSITION_CENTER", POSITION_CENTER);
	PyModule_AddIntConstant (m, "POSITION_TOP_LEFT", POSITION_TOP_LEFT);
	PyModule_AddIntConstant (m, "POSITION_TOP_RIGHT", POSITION_TOP_RIGHT);
	PyModule_AddIntConstant (m, "POSITION_BOTTOM_LEFT", POSITION_BOTTOM_LEFT);
	PyModule_AddIntConstant (m, "POSITION_BOTTOM_RIGHT", POSITION_BOTTOM_RIGHT);
	PyModule_AddIntConstant (m, "POSITION_CUSTOM", POSITION_CUSTOM);
	PyModule_AddIntConstant (m, "OPTION_OPACITY", OPTION_OPACITY);
	PyModule_AddIntConstant (m, "OPTION_USE_WORKAREA", OPTION_USE_WORKAREA);
	PyModule_AddIntConstant (m, "OPTION_TEXT_COLOR", OPTION_TEXT_COLOR);
	PyModule_AddIntConstant (m, "OPTION_BACKGROUND_COLOR", OPTION_BACKGROUND_COLOR);
	PyModule_AddIntConstant (m, "OPTION_DEFAULT_TIME", OPTION_DEFAULT_TIME);
	PyModule_AddIntConstant (m, "OPTION_MARGIN", OPTION_MARGIN);
	PyModule_AddIntConstant (m, "OPTION_FOLLOW_ACTIVE_SCREEN", OPTION_FOLLOW_ACTIVE_SCREEN);
	PyModule_AddIntConstant (m, "OPTION_WAIT_WHEN_USER_IDLE", OPTION_WAIT_WHEN_USER_IDLE);
	PyModule_AddIntConstant (m, "OPTION_DO_NOT_DISTURB", OPTION_DO_NOT_DISTURB);
	
	current_position = POSITION_CENTER;
	default_time = 2000;
	do_not_disturb = true;
	follow_active_screen = true;
	margin = 2;
	use_workarea = true;
	wait_while_idle = true;
	isVistaOrHigher = IsWindowsVistaOrHigher();
	
	while(!startup_done)
		Sleep(1);
}