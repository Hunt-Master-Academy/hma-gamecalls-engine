// This file centralizes the implementations for single-file third-party libraries.
// Defining the _IMPLEMENTATION macro in only one compilation unit prevents
// "multiple definition" linker errors.

#define MINIAUDIO_IMPLEMENTATION
#include "../libs/miniaudio.h"

#define DR_WAV_IMPLEMENTATION
#include "../libs/dr_wav.h"

// #define DR_MP3_IMPLEMENTATION
// #include "../libs/dr_mp3.h"
