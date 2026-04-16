// 静态网格：二进制 .staticmesh 加载，VBO/IBO 上传，全屏四边形与地面
#pragma once
#include <string>
#include <unordered_map>
#include "Utils.h"
#include "MyVulkan.h"
#include "Material.h"
struct VulkanSubMesh : public SubMesh {
	BufferObject* mIBO;
	VulkanSubMesh() {
		mIBO = nullptr;
	}
};
class StaticMeshComponent {
public:
	StaticMeshVertexData* mVertexData;
	int mVertexCount;
	int mVertexDataSize;
	BufferObject* mVBO;
	std::unordered_map<std::string, VulkanSubMesh*> mSubMeshes;
	void SetPosition(int index, float x, float y, float z, float w = 1.0f);
	void SetTexcoord(int index, float x, float y, float z = 1.0f, float w = 1.0f);
	void SetNormal(int index, float x, float y, float z, float w = 0.0f);
	void LoadFromFile(const char* inFilePath);
	void LoadFromFile2(const char* inFilePath);
	StaticMeshComponent();
	virtual void Update(VkCommandBuffer inCommandBuffer);
	virtual void Draw(VkCommandBuffer inCommandBuffer);
};
class FullScreenQuadMeshComponent : public StaticMeshComponent {
public:
	void Init();
};
class GroundMeshComponent : public StaticMeshComponent {
public:
	void Init(float size = 10.0f);
};