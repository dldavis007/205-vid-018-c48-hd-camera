/*
 * menu_navigation_stub.h  —  what test_menu_navigation.c uses from the stub:
 * the fixture's menu variable, the Select() spy, the camera/mic recorders,
 * the Timer1 thread and the per-test reset.
 */

#ifndef MENU_NAVIGATION_STUB_H
#define MENU_NAVIGATION_STUB_H

#include <stdbool.h>
#include "MenuFunctions.h"

/* Menu index of each fixture menu; {0,0,0,<this>}. */
#define NAV_ROOT 0
#define NAV_SUB  1
#define NAV_TALL 2

/* Lines per fixture menu (entry 0 is the title). */
#define NAV_ROOT_ENTRIES 4
#define NAV_TALL_ENTRIES 10

extern struct menu_var NavValue;       /* the SUB menu's "VALUE" line */

extern int  nav_action_calls;          /* Select() ran ROOT's "ACTION" line */
extern int  nav_camera_on_calls;
extern bool nav_camera_on;
extern bool nav_mic_on;

void nav_timer_start(void);
void nav_timer_stop(void);
void nav_reset(void);

#endif /* MENU_NAVIGATION_STUB_H */
