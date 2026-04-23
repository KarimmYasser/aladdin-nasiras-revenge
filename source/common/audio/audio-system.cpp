/**
 * @file audio-system.cpp
 * @brief Compiles the miniaudio implementation exactly once.
 *
 * miniaudio is a single-header library.  It needs MINIAUDIO_IMPLEMENTATION
 * defined in exactly ONE translation unit so that all the function bodies are
 * compiled.  This file does that, and nothing else.
 *
 * Every other file that needs miniaudio includes "audio-system.hpp" which only
 * includes the *declarations* from <miniaudio.h>.
 */

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
