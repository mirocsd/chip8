// for debugging I/O
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <iostream>

// for signal handling
#include <signal.h>

// for the display
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_timer.h>

struct config_obj {
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

  config_obj() {
    // Screen width and height
    SCREEN_WIDTH = 64;
    SCREEN_HEIGHT = 32;
    // Foreground/background colors
    FG_R = 0xFF;
    FG_G = 0xFF;
    FG_B = 0xFF;
    FG_A = 0xFF;
    BG_R = 0x00;
    BG_G = 0x00;
    BG_B = 0x00;
    BG_A = 0x00;
    scale = 20;
  }
};

class sdl_obj {
 public:
  SDL_Window *win;
  SDL_Texture *tex;
  SDL_Renderer *rend;

  // ctor just runs the initialization function
  sdl_obj() {
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
      std::cout << "Error initializing SDL: " << SDL_GetError() << std::endl;
    }
  }

  // "Initialize" SDL (create window and renderer)
  void initialize(config_obj config) {
    this->win = SDL_CreateWindow("CHIP8", SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED,
                                 config.SCREEN_WIDTH * config.scale,
                                 config.SCREEN_HEIGHT * config.scale, 0);

    this->rend = SDL_CreateRenderer(this->win, -1, SDL_RENDERER_ACCELERATED);
  }

  void reset_screen(config_obj config) {
    SDL_SetRenderDrawColor(rend, config.BG_R, config.BG_G, config.BG_B,
                           config.BG_A);
    // Clear the screen
    SDL_RenderClear(rend);
  }

  void update_screen() { SDL_RenderPresent(rend); }

  // Perform exit cleanup
  void exit_cleanup() {
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();
  }

  ~sdl_obj() { exit_cleanup(); }
};

enum emulator_state_t {
  QUIT,
  RUNNING,
  PAUSED,
};

struct instruction {
  uint16_t opcode;
  uint16_t nnn; // constant - last 12 bits - *nnn
  uint8_t kk;   // constant - last 8 bits  - **kk 
  uint8_t n;    // constant - last 4 bits  - ***n
  uint8_t x;    // lower 4 bits of high byte of instruction - *x** - register identifier
  uint8_t y;    // upper 4 bits of low byte of instruction  - **y* - register identifier
};
class chip8_obj {
 public:
  emulator_state_t state;
  uint8_t ram[4096];     // CHIP8 Memory
  bool display[64 * 32]; // Display array
  uint16_t stack[12];    // Subroutine stacl
  uint8_t V[16];         // Registers V0-VF
  uint16_t I;            // Index register
  uint16_t PC;
  uint8_t delay_timer;
  uint8_t sound_timer;   // Beeps when nonzero
  bool keypad[16];       // Hex keypad
  char *rom_name;        // Current ROM
  instruction inst;      // Current instruction

  const uint32_t start_address = 0x200;
  chip8_obj() { state = RUNNING; }

  void initialize(char rom_name[]) {
    const uint32_t start_address = 0x200;
    const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };
    PC = start_address;

    // Load the font into memory
    std::memcpy(ram, font, sizeof(font));

    this->rom_name = rom_name;
    // Open ROM
    FILE *rom = fopen(rom_name, "rb");
    if (!rom) { SDL_Log("Could not open rom file %s\n", rom_name); }

    const size_t rom_size = std::filesystem::file_size(rom_name);
    const size_t max_size = sizeof ram - start_address;

    // Check ROM size
    if (rom_size > max_size) { SDL_Log("Rom file is too large"); }

    // Load ROM
    fread(ram + start_address, rom_size, 1, rom);
    fclose(rom);
  }

  void handle_input() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
      switch (event.type) {
      case SDL_QUIT:
        state = QUIT;
        return;

      case SDL_KEYDOWN:
        switch (event.key.keysym.sym) {
        case SDLK_ESCAPE:
          state = QUIT;
          return;

        case SDLK_SPACE:
          if (state == RUNNING) {
            state = PAUSED;
            std::cout << "CHIP8 PAUSED" << std::endl;
          } else {
            state = RUNNING;
          }

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

  void run_instruction() {
    


  }
};

void handle_extern_signal(int signal) {
  if (signal == SIGINT || signal == SIGTERM || signal == SIGTSTP) {
    std::cout << "\nExiting..." << std::endl;
    SDL_Quit();
    exit(0);
  }
}

// Register the signal handler for stopping with CTRL+Z/C
void init_sighandle(void) {
  signal(SIGINT, handle_extern_signal);
  signal(SIGTERM, handle_extern_signal);
  signal(SIGTSTP, handle_extern_signal);
}

int main(int argc, char **argv) { 
  init_sighandle();

  // Init configuration
  config_obj config;

  // Init SDL
  sdl_obj sdl;
  sdl.initialize(config);

  chip8_obj chip8;
  if (argc < 2) {
    std::cout << "Usage: " << argv[0] << " <path to rom file>" << std::endl;
    sdl.exit_cleanup();
    exit(0);
  }

  char *rom_name = argv[1];
  chip8.initialize(rom_name);

  sdl.reset_screen(config);
  // Main loop
  while (chip8.state != QUIT) {
    chip8.handle_input();

    



    // Delays for 60Hz, should compensate for time spent executing CHIP8
    // instructions as well
    SDL_Delay(1000 / 60);

    sdl.update_screen();
  }

  sdl.exit_cleanup();
}
