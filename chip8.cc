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
  uint16_t stack[12];    // Subroutine stack
  uint16_t *stack_ptr;
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
    *stack_ptr = stack[0];

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

  void run_instruction(config_obj config) {
    inst.opcode = (ram[PC] << 8) | ram[PC+1]; 
    PC += 2;
    
    inst.nnn = inst.opcode & 0x0FFF;
    inst.kk = inst.opcode & 0x0FF;
    inst.n = inst.opcode & 0x0F;
    inst.x = (inst.opcode >> 8) & 0x0F;
    inst.y = (inst.opcode >> 4) & 0x0F;
    switch ((inst.opcode >> 12) & 0x0F) {
      case 0x0:
        switch (inst.kk) {
          case 0xE0: // 0x00E0 - Clear the display
            memset(display, false, sizeof display);
          case 0xEE: // 0x00EE - Return from a subroutine
            // Pop last address from stack, set PC to this (the RA)
            PC = *--stack_ptr;
        }

      case 0x01:
        PC = inst.nnn;
        break;

      case 0x02: // 0x2nnn - Call subroutine at nnn
        *stack_ptr++ = PC; // Store the current (return) address on top of stack
        PC = inst.nnn; // PC will be at nnn for start of subroutine
        break;
      
      case 0x03: // 0x3xkk - Skip next instruction if Vx = kk
        if (V[inst.x] == inst.kk) { PC += 2; }
        break;

      case 0x04: // 0x4xkk - Skip next instruction if Vx != kk
        if (V[inst.x] != inst.kk) { PC += 2; }
        break;

      case 0x05: // 0x5xy0 - Skip next instruction if Vx = Vy
        if (V[inst.x] == V[inst.y]) { PC += 2; }
        break;

      case 0x06: // Set Vx = kk
        V[inst.x] = inst.kk;
        break;

      case 0x07: // Set Vx = Vx + kk
        V[inst.x] = inst.x + inst.kk;
        break;

      case 0x08:
        switch (inst.n) {
          case 0x00: // 0x8xy0 - Set Vx = Vy
            V[inst.x] = V[inst.y];
            break;
          
          case 0x01: // 0x8xy1 - Set Vx = Vx OR Vy
            V[inst.x] = V[inst.x] | V[inst.y];
            break;

          case 0x02: // 0x8xy2 - Set Vx = Vx AND Vy
            V[inst.x] = V[inst.x] & V[inst.y];
            break;


        }

      case 0x0A: // 0xAnnn - Set register I to nnn
        I = inst.nnn;
        break;

      case 0x0D: // 0xDxyn - Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
      {
        uint8_t x_coord = V[inst.x] % config.SCREEN_WIDTH; 
        uint8_t y_coord = V[inst.y] % config.SCREEN_HEIGHT;
        V[0xF] = 0; // Set flag to 0
        
        // For all n rows of the sprite
        for (uint8_t i = 0; i < inst.n; i++) {
          const uint8_t sprite_data = ram[I + i]; // Get next byte of sprite data
          
          for (int8_t j = 7; j <= 0; j--) {
            if ((sprite_data & (1 << j)) && display[y_coord * config.SCREEN_WIDTH + x_coord]) {
              V[0xF] = 1; // Set the carry flag since sprite pixel and display pixel both on
            }

            // jumbled xor stuffs
            display[y_coord * config.SCREEN_WIDTH + x_coord] ^= (sprite_data & (1 << j));

            // Stop drawing if on right edge of screen
            if (++x_coord >= config.SCREEN_WIDTH) break;

          }

          // Stop drawing if sprite hits bottom edge of screen
            if (++y_coord >= config.SCREEN_HEIGHT) break;
        }
        break;
      }


      default:
        std::cout << "Not yet implemented" << std::endl;
        break;

    }
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

    chip8.run_instruction(config);



    // Delays for 60Hz, should compensate for time spent executing CHIP8
    // instructions as well
    SDL_Delay(1000 / 60);

    sdl.update_screen();
  }

  sdl.exit_cleanup();
}
