#include "input.h"

SDL_GameController *gameController;

void input_init() {
    SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER);
    gameController = SDL_GameControllerOpen(0);
}

void input_poll() {
    SDL_GameControllerUpdate();
}

int input_button_down(SDL_GameControllerButton button) {
    return SDL_GameControllerGetButton(gameController, button);
}

void input_free() {
    SDL_GameControllerClose(gameController);
    SDL_QuitSubSystem(SDL_INIT_GAMECONTROLLER);
}
