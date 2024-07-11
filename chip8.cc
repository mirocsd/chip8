// for debugging I/O
#include <iostream>

// for signal handling
#include <signal.h>

// for the display
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>

typedef struct {
  SDL_Window *win;
  SDL_Texture *tex;
  SDL_Renderer *rend;
} sdl_t;
void handle_signal(int signal) {
  if (signal == SIGINT || signal == SIGTERM || signal == SIGTSTP) {
    std::cout << "Exiting..." << std::endl;
    SDL_Quit();
    exit(0);
  }
}

// Intializing SDL
void init_sdl(sdl_t *sdl) {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
      printf("error initializing SDL: %s\n", SDL_GetError());
  }
  

  // Defining window width and height
  int SCREEN_WIDTH = 640;
  int SCREEN_HEIGHT = 320;
  sdl->win = SDL_CreateWindow("CHIP8", 
                                     SDL_WINDOWPOS_CENTERED, 
                                     SDL_WINDOWPOS_CENTERED, 
                                     SCREEN_WIDTH, SCREEN_HEIGHT, 0);

  // Defining window resolution
  int RES_WIDTH = 64;
  int RES_HEIGHT = 32;


  // Creating renderer
  sdl->rend = SDL_CreateRenderer(sdl->win, -1, SDL_RENDERER_ACCELERATED);


  // Defining foreground/background colors

  const uint8_t FG_R = 0xFF;
  const uint8_t FG_G = 0xFF;
  const uint8_t FG_B = 0xFF;
  const uint8_t FG_A = 0xFF;
  const uint8_t BG_R = 0x00;
  const uint8_t BG_G = 0x00;
  const uint8_t BG_B = 0x00;
  const uint8_t BG_A = 0x00;


  SDL_SetRenderDrawColor(sdl->rend, BG_R, BG_G, BG_B, BG_A);
  // Clear the screen
  SDL_RenderClear(sdl->rend);
}

void reset_screen(sdl_t *sdl) {
  // Defining foreground/background colors
  const uint8_t FG_R = 0xFF;
  const uint8_t FG_G = 0xFF;
  const uint8_t FG_B = 0xFF;
  const uint8_t FG_A = 0xFF;
  const uint8_t BG_R = 0x00;
  const uint8_t BG_G = 0x00;
  const uint8_t BG_B = 0x00;
  const uint8_t BG_A = 0x00;


  SDL_SetRenderDrawColor(sdl->rend, BG_R, BG_G, BG_B, BG_A);
  // Clear the screen
  SDL_RenderClear(sdl->rend);
}

void update_screen(sdl_t *sdl) {
  SDL_RenderPresent(sdl->rend);
}
// Register the signal handler for stopping with CTRL+Z/C
void init_sighandle(void) {
  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);
  signal(SIGTSTP, handle_signal);
}

void exit_cleanup(sdl_t *sdl) {
  SDL_Quit();
  SDL_DestroyTexture(sdl->tex);
  SDL_DestroyRenderer(sdl->rend);
  SDL_DestroyWindow(sdl->win);
}

int main() {
  init_sighandle();
  
  sdl_t sdl = {0};
  init_sdl(&sdl);
  
  // Main loop
  while(1) {

    // Delays for 60Hz, should compensate for time spent executing CHIP8 instructions later on
    SDL_Delay(1000/60);

    update_screen(&sdl);
  }
  
  exit_cleanup(&sdl);
}
