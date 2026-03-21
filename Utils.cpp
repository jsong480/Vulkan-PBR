#include "Utils.h"

unsigned char* LoadImageFromFile(const char* path, int& width, int& height, int& channel, int force_channel) {
	unsigned char* result = stbi_load(path, &width, &height, &channel, force_channel);
	if (result == nullptr) {
		return nullptr;
	}
	return result;
}