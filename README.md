# pyPopUp

A Windows extension for Python (2.x) to display simple message popups.  
This is a personal project and still Work In Progress. Based on extension example project and compiled with VS2008.
Only tested with Python 2.7 on 64bit Windows. You can use the provided popup.pyd on these systems.

### Compilation
copy paste de sourcefiles into the Python extension example project. Remove example.c and add popup.cpp to the project.
Change the "/export:initexample" to "/export:initpopup" in settings (or in example.vcproj).

### Installation
Put the popup.pyd in the "Lib/site-packages" folder for system-wide use. Or put it in the same folder as your script.

### Sample image
![Preview](https://raw.github.com/ArmyOfPirates/miniature-octo-shame/master/pyPopUp/popup.png)

## Features
* Message queue
* Automatic positioning with lots of options (avoid taskbar/follow active screen/etc)
* Font adjustable
* Colors adjustable
* Timing adjustable
* Multi monitor support (untested)
* Fade out
* Unicode!
* Fancy rounded corners

## Functions
* setOption(name, value)  
  Use one of the OPTION_* constants for the name. The type of value depends on the constant.
* setFont(name, size)  
  Set font to the specified name(unicode!) and size.
* setPosition(name, x, y)  
  Use one of the POSITION_* constants. x and y are only required for POSITION_CUSTOM.
* show(text, [time])  
  Show a popup (optional for _time_ milliseconds). Text must be in unicode.
* wait()  
  Halts until popup queue is empty and all popups have been shown.
* clear()  
  Removes all popups from queue and screen.

## Constants
for setPosition:
* POSITION_CENTER, default
* POSITION_TOP_LEFT
* POSITION_TOP_RIGHT
* POSITION_BOTTOM_LEFT
* POSITION_BOTTOM_RIGHT
* POSITION_CUSTOM, needs X and Y values.

for setOption:
* OPTION_OPACITY, accepts 0-100, default 100.
* OPTION_USE_WORKAREA, bool, automatically avoid taskbar if true, default true.
* OPTION_TEXT_COLOR, tuple "(R, G, B)", values 0-255.
* OPTION_BACKGROUND_COLOR, tuple "(R, G, B)", values 0-255.
* OPTION_DEFAULT_TIME, integer, in milliseconds, default 2000.
* OPTION_MARGIN, margin used in TOP/BOTTOM positions, default 2.
* OPTION_FOLLOW_ACTIVE_SCREEN, display popup on monitor with active window, default true.
* OPTION_WAIT_WHEN_USER_IDLE, keep popup on screen while user is idle, default true.
* OPTION_DO_NOT_DISTURB, prevent displaying popups when something runs fullscreen, default true.

## Demo
```python
import popup

popup.setFont(u"Calibri", 40)
popup.setOption(popup.OPTION_TEXT_COLOR, (255,0,255))
popup.setOption(popup.OPTION_BACKGROUND_COLOR, (60,0,60))
popup.show(u"Test script")
print "wait"
popup.wait()
print "spam"
popup.setPosition(popup.POSITION_BOTTOM_RIGHT)
popup.show(u"with positions", 3000)
popup.show(u"and queue!")
print"wait again"
popup.wait()
print "nothing to wait for"
popup.wait()
popup.show(u"with positions", 3000)
popup.show(u"and queue!")
popup.clear()
print "cleared!"
```