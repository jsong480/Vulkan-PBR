#pragma once
extern "C" {
	#include "stbi/stb_image.h"
}
#include "glm/glm.hpp"
#include "glm/ext.hpp"
struct Matrix4x4f {
	float mData[16];
	Matrix4x4f() {
		memset(mData, 0, sizeof(float) * 16);
		mData[0] = 1.0f;
		mData[5] = 1.0f;
		mData[10] = 1.0f;
		mData[15] = 1.0f;
	}
};
struct StaticMeshVertexData {
	float mPosition[4];
	float mTexcoord[4];
	float mNormal[4];
};
struct StaticMeshVertexDataEx :public StaticMeshVertexData {
	float mTangent[4];
};
struct SubMesh {
	unsigned int* mIndexes;
	int mIndexCount;
};
float GetFrameTime();
unsigned char* LoadFileContent(const char* path, int& filesize);
unsigned char* LoadImageFromFile(const char* path, int& width, int& height, int& channel, int force_channel);