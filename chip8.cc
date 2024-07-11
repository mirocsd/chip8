// for debugging I/O
#include <iostream>

// for signal handling
#include <signal.h>

// for the display
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>

struct sdl_t
{
  SDL_Window *win;
  SDL_Texture *tex;
  SDL_Renderer *rend;
};

struct config_t
{
  uint32_t SCREEN_WIDTH;
  uint32_t SCREEN_HEIGHT;
  uint8_t FG_R;
  uint8_t FG_G;
  uint8_t FG_B;
  uint8_t FG_A;
  uint8_t BG_R;
  uint8_t BG_G;
  uint8_t BG_B;
  uint8_t BG_A;
  uint32_t scale;
};

enum emulator_state_t
{
  QUIT,
  RUNNING,
  PAUSED,
};

struct chip8_t
{
  emulator_state_t state;
  uint8_t ram[4096];
  int display[64*32];
};

void handle_extern_signal(int signal) {
  if (signal == SIGINT || signal == SIGTERM || signal == SIGTSTP) {
    std::cout << "\nExiting..." << std::endl;
    SDL_Quit();
    exit(0);
  }
}

void init_config(config_t &config) {
  // Screen width and height
  config.SCREEN_WIDTH = 64;
  config.SCREEN_HEIGHT = 32;
  // Foreground/background colors
  config.FG_R = 0xFF;
  config.FG_G = 0xFF;
  config.FG_B = 0xFF;
  config.FG_A = 0xFF;
  config.BG_R = 0x00;
  config.BG_G = 0x00;
  config.BG_B = 0x00;
  config.BG_A = 0x00;
  config.scale = 20;
}

// Intializing SDL
void init_sdl(sdl_t *sdl, config_t config) {
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
      printf("error initializing SDL: %s\n", SDL_GetError());
  }
  
  sdl->win = SDL_CreateWindow("CHIP8", 
                                     SDL_WINDOWPOS_CENTERED, 
                                     SDL_WINDOWPOS_CENTERED, 
                                     config.SCREEN_WIDTH * config.scale, 
                                     config.SCREEN_HEIGHT * config.scale, 0);
 
  // Creating renderer
  sdl->rend = SDL_CreateRenderer(sdl->win, -1, SDL_RENDERER_ACCELERATED);

}

void reset_screen(sdl_t *sdl, config_t config) {
  SDL_SetRenderDrawColor(sdl->rend, config.BG_R, config.BG_G,config.BG_B, config.BG_A);
  // Clear the screen
  SDL_RenderClear(sdl->rend);
}

void update_screen(sdl_t *sdl) {
  SDL_RenderPresent(sdl->rend);
}
// Register the signal handler for stopping with CTRL+Z/C
void init_sighandle(void) {
  signal(SIGINT, handle_extern_signal);
  signal(SIGTERM, handle_extern_signal);
  signal(SIGTSTP, handle_extern_signal);
}

// Do final SDL cleanups
void exit_cleanup(sdl_t *sdl) {
  SDL_DestroyTexture(sdl->tex);
  SDL_DestroyRenderer(sdl->rend);
  SDL_DestroyWindow(sdl->win);
  SDL_Quit();
}

void handle_input(chip8_t &chip8) {
  SDL_Event event;

  while (SDL_PollEvent(&event)) {
    switch(event.type) {
      case SDL_QUIT:
        chip8.state = QUIT;
        return;
      
      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
          case SDLK_ESCAPE:
            chip8.state = QUIT;
            return;
          default:
            break;
        }
        break;

      case SDL_KEYUP:
        break;

      default:
        break;
    }
  }
}

int main() {
  init_sighandle();
  
  // Init SDL
  sdl_t sdl = {0};
  
  // Init configuration
  config_t config = {0};
  init_config(config);

  init_sdl(&sdl, config);
  
  chip8_t chip8;
  chip8.state = RUNNING; // Default state is RUNNING
  reset_screen(&sdl, config);
  // Main loop
  while(chip8.state != QUIT) {
    handle_input(chip8);
    // Delays for 60Hz, should compensate for time spent executing CHIP8 instructions as well
    SDL_Delay(1000/60);

    update_screen(&sdl);
  }
  
  exit_cleanup(&sdl);
}
