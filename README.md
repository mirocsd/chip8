This is my version of a CHIP8 interpreter, written in C++.
The CHIP8 is a virtual machine which is like a simple computer; machine instructions can modify registers and memory, perform arithmetic, read information from a keypad, and modify and send information to the display. It's capable of playing old-school games like TETRIS, Pong, Space Invaders, etc.

This project completely nailed my understanding of computing basics and object-oriented programming, expanding on what I'd learned in my intro to microprocessors course and intro to tangible computing courses.

Most online resources I could find regarding creating a CHIP8 interpreter used C and took a functional approach, but I chose to go the object-oriented way, using C++. I noticed that separating main components of the program into objects made implementing features a lot more intuitive (and probably less verbose).


On a QWERTY keyboard, the 4x4 keypad is taken to be the box containing the diagonal 1-V (i.e. 1234/QWER/ASDF/ZXCV). 
Games are easy to find (search 'chip8 ROMs'), and the ROM should be the first (and only) argument. 
With SDL2 installed (sudo apt-get libsdl2-2.0-0 on Ubuntu/Debian / most package managers should have SDL2), compile the program using the given Makefile.
