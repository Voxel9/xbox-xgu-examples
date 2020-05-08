#ifndef __INPUT_H__
#define __INPUT_H__

#include <SDL.h>

void input_init();
void input_poll();
void input_free();

int input_button_down(SDL_GameControllerButton button);

#endif
