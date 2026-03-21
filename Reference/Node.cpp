#include "Node.h"
#include "Camera.h"
Node::Node() {
	mStaticMeshComponent = nullptr;
	// Reserve a large per-node matrix UBO block.
	// Current usage:
	// [0]=Model, [1]=View, [2]=Projection, [3]=InverseTranspose(Model)
	mMat4UBOData = new Mat4UniformBufferData(1024);
}

void Node::Draw(VkCommandBuffer inCommandBuffer, glm::mat4& inProjectionMatrix, Camera& inCamera) {
	// Update transform matrices for this draw.
	mMat4UBOData->SetMat4(0, glm::value_ptr(mModelMatrix));
	mMat4UBOData->SetMat4(1, glm::value_ptr(inCamera.mViewMatrix));
	mMat4UBOData->SetMat4(2, glm::value_ptr(inProjectionMatrix));
	glm::mat4 itModelMatrix = glm::inverseTranspose(mModelMatrix);
	mMat4UBOData->SetMat4(3, glm::value_ptr(itModelMatrix));

	// First draw lazily binds matrix UBO to material binding 0.
	if (mMat4UBOData->mUBO == nullptr) {
		mMat4UBOData->UpdateGPUData();
		mMaterial->SetUBO(0, mMat4UBOData->mUBO);
	}
	else {
		mMat4UBOData->UpdateGPUData();
	}
	mStaticMeshComponent->Update(inCommandBuffer);
	// Camera world position is consumed in fragment shaders (binding 3 vec4[0]).
	mMaterial->mVec4s->SetVec4(0, glm::value_ptr(inCamera.mPosition));
	// Bind PSO/descriptors and issue draw.
	mMaterial->Active(inCommandBuffer,mStaticMeshComponent->mVertexDataSize);
	SetDynamicState(mMaterial->mPipelineStateObject, inCommandBuffer);
	mStaticMeshComponent->Draw(inCommandBuffer);
}