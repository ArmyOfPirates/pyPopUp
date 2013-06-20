#include "Python.h"
#include <windows.h>
#include <Strsafe.h>
#include <process.h>
#define COMPILE_MULTIMON_STUBS
#include "multimon.h"
#include "timer.h"
#pragma warning(push)
#pragma warning(disable: 4995)
#include <vector>
#pragma warning(pop)

#ifndef HINST_THISCOMPONENT
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)
#endif

#define IDC_POPUP_TEXT 101

#define POSITION_CENTER			0
#define POSITION_TOP_LEFT		1
#define POSITION_TOP_RIGHT		2
#define POSITION_BOTTOM_LEFT	3
#define POSITION_BOTTOM_RIGHT	4
#define POSITION_CUSTOM			5

#define OPTION_OPACITY				6
#define OPTION_USE_WORKAREA			7
#define OPTION_TEXT_COLOR			8
#define OPTION_BACKGROUND_COLOR		9
#define OPTION_DEFAULT_TIME			10
#define OPTION_MARGIN				11
#define OPTION_FOLLOW_ACTIVE_SCREEN	12



struct pqueue{
	const Py_UNICODE *text;
	unsigned int time;
};

/*  GLOBALS  */
HWND m_hWnd;
HWND hwndLabel;
HFONT hfFont;
COLORREF textColor;

CTimer showTimer;
CTimer fadeTimer;

int opacity, default_opacity;
int current_position = POSITION_CENTER;
int pos_x, pos_y;
int default_time = 2000;
bool use_workarea = true, follow_active_screen = true, startup_done = false;
int margin = 2;

std::vector <pqueue> popup_queue;
CRITICAL_SECTION cs;

/* functions */
static void startFade();
static void fadeStep();
static LRESULT WINAPI CustomWndProc(HWND lhWnd, UINT message, WPARAM wParam, LPARAM lParam);
static void position_popup(int width, int height);
static void show(const Py_UNICODE *text, Py_ssize_t time);
static unsigned __stdcall boot(void* pArguments);
static PyObject *
popup_setOption(PyObject *self, PyObject *args);
static PyObject *
popup_setPosition(PyObject *self, PyObject *args);
static PyObject *
popup_setFont(PyObject *self, PyObject *args);
static PyObject *
popup_show(PyObject *self, PyObject *args);
static PyObject *
popup_wait(PyObject *self, PyObject *args);
PyMODINIT_FUNC
initpopup(void);