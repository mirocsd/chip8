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
  int scale;

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
    scale = 10;
  }
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
  chip8_obj() { 
    state = RUNNING;
    PC = start_address;
    stack_ptr = stack;
    memset(ram, 0, sizeof(ram));
    memset(display, false, sizeof(display));
    memset(V, 0, sizeof(V));
    I = 0;
    delay_timer = 0;
    sound_timer = 0;
    memset(keypad, false, sizeof(keypad)); 
  }

  void initialize(char *rom_name) {
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
    stack_ptr = stack;

    // Load the font into memory
    std::memcpy(ram + 0x50, font, sizeof(font));

    this->rom_name = rom_name;
    // Open ROM
    FILE *rom = fopen(rom_name, "rb");
    if (!rom) { 
      SDL_Log("Could not open rom file %s\n", rom_name); 
      return;
    }

    const size_t rom_size = std::filesystem::file_size(rom_name);
    const size_t max_size = sizeof ram - start_address;

    // Check ROM size
    if (rom_size > max_size) { SDL_Log("Rom file is too large"); }

    // Load ROM
    size_t read_size = fread(ram + start_address, 1, rom_size, rom);
    if (read_size != rom_size) {
      SDL_Log("Read size is not the same as ROM file size\n");
      fclose(rom);
      return;
    }
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
    std::cout << "Current PC: " << PC << std::endl;
    std::cout << "ram[PC] << 8 -- " << std::hex << (ram[PC] << 8) << " ram[PC+1] -- " << std::hex << ram[PC+1] << std::endl;
    inst.opcode = (ram[PC] << 8) | ram[PC+1]; 
    PC += 2;
    std::cout << "PC now: " << PC << std::endl;
    std::cout << "Current opcode: " << std::hex << inst.opcode << std::endl;
    
    inst.nnn = inst.opcode & 0x0FFF;
    inst.kk = inst.opcode & 0x00FF;
    std::cout << "inst.kk before switch: " << inst.kk << std::endl;
    inst.n = inst.opcode & 0x000F;
    inst.x = (inst.opcode >> 8) & 0x000F;
    inst.y = (inst.opcode >> 4) & 0x000F;

    std::cout << "Decoded Instruction: nnn=" << std::hex << inst.nnn 
              << ", kk=" << std::hex << static_cast<int>(inst.kk)
              << ", n=" << std::hex << static_cast<int>(inst.n)
              << ", x=" << std::hex << static_cast<int>(inst.x)
              << ", y=" << std::hex << static_cast<int>(inst.y) << std::endl;

    switch ((inst.opcode >> 12) & 0x000F) {
      case 0x0:
        switch (inst.kk) {
          case 0xE0: // 0x00E0 - Clear the display
            memset(display, false, sizeof display);
            break;
          case 0xEE: // 0x00EE - Return from a subroutine
            // Pop last address from stack, set PC to this (the RA)
            PC = *--stack_ptr;
            break;
        }
      break;

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
        std::cout << "Executing 0x4xkk - Skip next instruction if Vx != kk" << std::endl;
        std::cout << "V[" << std::hex << static_cast<int>(inst.x) << "] = " << std::hex << static_cast<int>(V[inst.x]) << std::endl;
        std::cout << "kk = " << std::hex << static_cast<int>(inst.kk) << std::endl;
        if (V[inst.x] != inst.kk) {
            std::cout << "Skipping next instruction, current PC: " << std::hex << PC << std::endl;
            PC += 2;
            std::cout << "New PC after skipping: " << std::hex << PC << std::endl;
        }
        break;

      case 0x05: // 0x5xy0 - Skip next instruction if Vx = Vy
        if (V[inst.x] == V[inst.y]) { PC += 2; }
        break;

      case 0x06: // Set Vx = kk
        std::cout << "-> Setting Vx\n" << "inst.x = " << static_cast<int>(inst.x) << " inst.kk = " << static_cast<int>(inst.kk) << std::endl;
        V[inst.x] = inst.kk;
        std::cout << "Vx = " << std::dec << V[inst.x] << std::endl;
        break;

      case 0x07: // Set Vx = Vx + kk
        V[inst.x] += inst.kk;
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
      break;

      case 0x0A: // 0xAnnn - Set register I to nnn
        I = inst.nnn;
        std::cout << "Index is now: " << I << std::endl;
        break;

      case 0x0D: // 0xDxyn - Display n-byte sprite starting at memory location I at (Vx, Vy), set VF = collision
      {
        uint8_t x = V[inst.x];
        uint8_t y = V[inst.y];
        uint8_t height = inst.n;
        V[0xF] = 0;
        for (uint8_t row = 0; row < height; ++row) {
          uint8_t sprite_byte = ram[I + row];
          for (uint8_t col = 0; col < 8; ++col) {
            uint8_t sprite_pixel = sprite_byte & (0x80 >> col);
            uint32_t display_index = (x + col + ((y + row) * config.SCREEN_WIDTH)) % (config.SCREEN_WIDTH * config.SCREEN_HEIGHT);
            bool *display_pixel = &display[display_index];
            if (sprite_pixel) {
                if (*display_pixel) {
                    V[0xF] = 1;
                }
                *display_pixel ^= 1;
            }
          }
        }
      }
      break;


      default:
        std::cout << "Not yet implemented" << std::endl;
        break;

    }
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
    std::cout << "Initializing...." << std::endl;
    win = SDL_CreateWindow("CHIP8", SDL_WINDOWPOS_CENTERED,
                                 SDL_WINDOWPOS_CENTERED,
                                 config.SCREEN_WIDTH * config.scale,
                                 config.SCREEN_HEIGHT * config.scale, 0);
    
    rend = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
  }

  void reset_screen(config_obj config) {
    SDL_SetRenderDrawColor(rend, config.BG_R, config.BG_G, config.BG_B,
                           config.BG_A);
    // Clear the screen
    SDL_RenderClear(rend);
  }

  void update_screen(const chip8_obj chip8, const config_obj config) {
    SDL_Rect rect = {.x = 0, .y = 0, .w = config.scale, .h = config.scale };


    for (uint32_t i = 0; i < sizeof chip8.display; i++) {
      // Loop thru all pixels
      // x values are i % width, y values are i / width
      rect.x = (i % config.SCREEN_WIDTH) * config.scale;
      rect.y = (i / config.SCREEN_WIDTH) * config.scale;

      if (chip8.display[i]) {
        SDL_SetRenderDrawColor(rend, config.FG_R, config.FG_B, config.FG_G, config.FG_A);
        SDL_RenderFillRect(rend, &rect);
      } else {
        SDL_SetRenderDrawColor(rend, config.BG_R, config.BG_B, config.BG_G, config.BG_A);
        SDL_RenderFillRect(rend, &rect);
      }
    }
    SDL_RenderPresent(rend); 

  }

  // Perform exit cleanup
  void exit_cleanup() {
    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(win);
    SDL_Quit();
  }

  ~sdl_obj() { exit_cleanup(); }
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
  std::cout << "Rom name: " << rom_name << std::endl;
  chip8.initialize(rom_name);
  std::cout << "Starting address of chip8: " << chip8.PC << std::endl;
  std::cout << "About to reset screen" << std::endl;
  sdl.reset_screen(config);
  std::cout << "Screen reset" << std::endl;
  // Main loop
  while (chip8.state != QUIT) {
    chip8.handle_input();
    std::cout << "About to run..." << std::endl;
    chip8.run_instruction(config);
    std::cout << "Instruction ran" << std::endl;


    // Delays for 60Hz, should compensate for time spent executing CHIP8
    // instructions as well
    SDL_Delay(1000 / 60);

    sdl.update_screen(chip8, config);
  }

  sdl.exit_cleanup();
}
