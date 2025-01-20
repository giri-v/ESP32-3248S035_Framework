////////////////////////////////////////////////////////////////////
/// @file build_options.h
/// @brief Contains defines that select framework features.
////////////////////////////////////////////////////////////////////

#ifndef BUILD_OPTIONS_H
#define BUILD_OPTIONS_H

#define USE_GRAPHICS
#define USE_OPEN_FONT_RENDERER
#define USE_SD_CARD
#define FONT_FS SD // comment this line to load font from code or #define FONT_FS (SPIFFS|SD)
#define USE_AUDIO
#define USE_JPEG_DECODER
#define USE_PNG_DECODER
#define USE_WEB_SERVER

#endif /// BUILD_OPTIONS_H