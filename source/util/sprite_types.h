#ifndef RME_SPRITE_TYPES_H
#define RME_SPRITE_TYPES_H

#include <cstdint>

struct SpriteLight {
	uint8_t intensity = 0;
	uint8_t color = 0;
};

struct SpriteUV {
	float u0;
	float v0;
	float u1;
	float v1;
};

#endif
