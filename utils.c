#include <stdint.h>

uint32_t encodeColor(uint8_t r, uint8_t g, uint8_t b){
	return ((uint32_t)r << 24) + ((uint32_t)g << 16) + ((uint32_t)b << 8);
}
uint8_t decodeColorR(uint32_t color){
	return color >> 24 & 0xFF;
}
uint8_t decodeColorG(uint32_t color){
	return color >> 16 & 0xFF;
}
uint8_t decodeColorB(uint32_t color){
	return color >> 8 & 0xFF;
}