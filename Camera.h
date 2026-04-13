#pragma once
#include "Utils.h"
class Camera {
public:
	float mDistanceFromTarget;
	float mRotateAngle;
	glm::vec3 mInitialViewDirection,mViewDirection,mPosition;
	glm::mat4 mViewMatrix;
	float mYaw, mPitch;
	void Init(glm::vec3 inTargetPosition, float inDistanceFromTarget, glm::vec3 inViewDirection);
	void InitFPS(glm::vec3 pos, float yaw, float pitch);
	void RotateFPS(float dx, float dy, float sensitivity);
	void MoveFPS(float dt, bool fwd, bool back, bool left, bool right, bool up, bool down, float speed);
	void Update(glm::vec3 inTargetPosition,float inDistanceFromTarget,glm::vec3 inViewDirection);
	void RoundRotate(float inDeltaTime, glm::vec3 inTargetPosition, float inDistanceFromTarget, float inRotateSpeed);
};
