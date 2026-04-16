#include "Node.h"
#include "Camera.h"

Node::Node() {
	mStaticMeshComponent = nullptr;
	mMat4UBOData = new Mat4UniformBufferData(1024);
}

// 写入 M/V/P 与法线矩阵 UBO，绑定材质并绘制
void Node::Draw(VkCommandBuffer inCommandBuffer, glm::mat4& inProjectionMatrix, Camera& inCamera) {
	mMat4UBOData->SetMat4(0, glm::value_ptr(mModelMatrix));
	mMat4UBOData->SetMat4(1, glm::value_ptr(inCamera.mViewMatrix));
	mMat4UBOData->SetMat4(2, glm::value_ptr(inProjectionMatrix));
	glm::mat4 itModelMatrix = glm::inverseTranspose(mModelMatrix);
	mMat4UBOData->SetMat4(3, glm::value_ptr(itModelMatrix));

	if (mMat4UBOData->mUBO == nullptr) {
		mMat4UBOData->UpdateGPUData();
		mMaterial->SetUBO(0, mMat4UBOData->mUBO);
	}
	else {
		mMat4UBOData->UpdateGPUData();
	}
	mStaticMeshComponent->Update(inCommandBuffer);
	mMaterial->mVec4s->SetVec4(0, glm::value_ptr(inCamera.mPosition));
	mMaterial->Active(inCommandBuffer,mStaticMeshComponent->mVertexDataSize);
	SetDynamicState(mMaterial->mPipelineStateObject, inCommandBuffer);
	mStaticMeshComponent->Draw(inCommandBuffer);
}