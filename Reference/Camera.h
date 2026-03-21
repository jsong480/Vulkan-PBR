#pragma once
#include "Utils.h"
class Camera {
public:
	float mDistanceFromTarget;
	float mRotateAngle;
	glm::vec3 mInitialViewDirection,mViewDirection,mPosition;
	glm::mat4 mViewMatrix;
	void Init(glm::vec3 inTargetPosition, float inDistanceFromTarget, glm::vec3 inViewDirection);
	void Update(glm::vec3 inTargetPosition,float inDistanceFromTarget,glm::vec3 inViewDirection);
	void RoundRotate(float inDeltaTime, glm::vec3 inTargetPosition, float inDistanceFromTarget, float inRotateSpeed);
};