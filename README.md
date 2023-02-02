# OpenUFO Motion
OpenUFO Motion is firmware for Arduino Mega(Atmel ATMEGA2560) Revision 3 based boards to run the motion of UFO catchers/crane games.  It handles the entirety of the basic game loop, player input, and credit redemption.  For service mode configuration and visual interface please visit the OpenUFO Controller project.

The motion board state machine logic is intended to be simple to avoid the microcontroller having to handle too many tasks at once.  The current state of the machine along with hardware events is broadcast on the serial port to any listeners.  For example, the Controller will listen to the credit event to update the displayed total credits.

# Building

1. Change all the necessary pin configurations in `config.h`.
2. Load the `openufo.ino` file in the Arduino IDE or another suitable editor with Arduino support.
3. Compile and upload to the board.