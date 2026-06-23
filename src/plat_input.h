#ifndef _PLAT_INPUT_H_
#define _PLAT_INPUT_H_

void input_init();
void input_close();

void mouse_poll_host();
void mouse_get_mickeys(int *x, int *y);
int mouse_get_buttons();
void mouse_capture_enable();
void mouse_capture_disable();

void get_mouse_deltas(int *x, int *y, int *b);

void keyboard_poll_host();
extern int key[512];

#include "keyboard_sdl.h"

#endif /*_PLAT_INPUT_H_*/