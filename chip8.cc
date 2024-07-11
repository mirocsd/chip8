// for debugging I/O
#include <iostream>

// for signal handling
#include <signal.h>

// for the display
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>

void handle_signal(int signal) {
  if (signal == SIGINT || signal == SIGTERM || signal == SIGTSTP) {
    std::cout << "Exiting..." << std::endl;
    SDL_Quit();
    exit(0);
  }
}


int *memory = new int[4096];
int main() {

  // Register the signal handler for stopping with CTRL+Z/C
  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);
  signal(SIGTSTP, handle_signal);

  // intializing SDL
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
      printf("error initializing SDL: %s\n", SDL_GetError());
  }
  // window width and height, to scale 64x32 pixels to
  int SCREEN_WIDTH = 640;
  int SCREEN_HEIGHT = 320;
  // actual pixel resolution
  int RES_WIDTH = 64;
  int RES_HEIGHT = 32;

  SDL_Window* win = SDL_CreateWindow("CHIP8", 
                                     SDL_WINDOWPOS_CENTERED, 
                                     SDL_WINDOWPOS_CENTERED, 
                                     SCREEN_WIDTH, SCREEN_HEIGHT, 0);

  Uint32 render_flags = SDL_RENDERER_ACCELERATED;

  SDL_Renderer* rend = SDL_CreateRenderer(win, -1, render_flags);

  SDL_Surface* surface;

  surface = IMG_Load("./output-onlinepngtools.png");

  SDL_Texture* tex = SDL_CreateTextureFromSurface(rend, surface);

  SDL_FreeSurface(surface);


  SDL_Rect dest;


  SDL_QueryTexture(tex, NULL, NULL, &dest.w, &dest.h);
  
  dest.w /= 1;
  dest.h /= 1;

  dest.x = (64 - dest.w) / 2;
  dest.y = (32 - dest.h) / 2;

  int close = 0;

  int speed = 30;


  while (!close) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      switch (event.type) {

      case SDL_QUIT:
        close = 1;
        break;
      
      case SDL_KEYDOWN:
        switch (event.key.keysym.scancode) {
        case SDL_SCANCODE_W:
        case SDL_SCANCODE_UP:
          dest.y -= speed / 30;
          break;
        case SDL_SCANCODE_A:
        case SDL_SCANCODE_LEFT:
          dest.x -= speed / 30;
          break;
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_DOWN:
          dest.y += speed / 30;
          break;
        case SDL_SCANCODE_D:
        case SDL_SCANCODE_RIGHT:
          dest.x += speed / 30;
          break;
        default:
          break;
        }
      }
    }


    if (dest.x + dest.w > 64)
      dest.x = 1000 - dest.w;

    if (dest.x < 0)
      dest.x = 0;

    if (dest.y + dest.h > 32)
      dest.y = 32 - dest.h;

    if (dest.y < 0)
      dest.y = 0;

    SDL_RenderClear(rend);
    SDL_RenderCopy(rend, tex, NULL, &dest);

    SDL_RenderPresent(rend);

    SDL_Delay(1000/60);
  }

  SDL_DestroyTexture(tex);
  SDL_DestroyRenderer(rend);
  SDL_DestroyWindow(win);
  SDL_Quit();





  memory[1] = 3;
  std::cout << memory[1] << std::endl;
  return 0;
}
