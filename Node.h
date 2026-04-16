// 场景节点：网格 + 材质 + per-object mat4 UBO
#pragma once
#include "Mesh.h"
#include "MyVulkan.h"
class Camera;
class Node {
public:
	StaticMeshComponent* mStaticMeshComponent;
	glm::mat4 mModelMatrix;
	Material* mMaterial;
	Mat4UniformBufferData*mMat4UBOData;
	Node();
	void Draw(VkCommandBuffer inCommandBuffer, glm::mat4& inProjectionMatrix, Camera&inCamera);
};