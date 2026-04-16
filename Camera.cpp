#include "Camera.h"
#include <cmath>

void Camera::Init(glm::vec3 inTargetPosition,float inDistanceFromTarget,glm::vec3 inViewDirection) {
	inViewDirection=glm::normalize(inViewDirection);
	mInitialViewDirection = inViewDirection;
	mDistanceFromTarget = inDistanceFromTarget;
	mRotateAngle = 0.0;
	mYaw = 0.0f;
	mPitch = 0.0f;
	Update(inTargetPosition, inDistanceFromTarget, inViewDirection);
}
void Camera::Update(glm::vec3 inTargetPosition, float inDistanceFromTarget, glm::vec3 inViewDirection) {
	glm::vec3 cameraPosition = inTargetPosition - inViewDirection * inDistanceFromTarget;
	glm::vec3 rightDirection = glm::cross(inViewDirection,glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 upDirection = glm::normalize(glm::cross(rightDirection, inViewDirection));
	mPosition = cameraPosition;
	mViewDirection = inViewDirection;
	mViewMatrix=glm::lookAt(cameraPosition, inTargetPosition, upDirection);
}
void Camera::RoundRotate(float inDeltaTime, glm::vec3 inTargetPosition,float inDistanceFromTarget, float inRotateSpeed) {
	float newAngle = mRotateAngle + inRotateSpeed * inDeltaTime;
	mRotateAngle = newAngle;
	glm::mat4 rotateMatrix;
	rotateMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(newAngle), glm::vec3(0.0f, 1.0f, 0.0f)); 
	glm::vec3 inserseInitialViewDirection = -mInitialViewDirection;
	glm::vec3 inverseViewDirection = glm::vec3(rotateMatrix * glm::vec4(inserseInitialViewDirection,0.0f));
	inverseViewDirection=glm::normalize(inverseViewDirection);

	glm::vec3 viewDirection = -inverseViewDirection;
	Update(inTargetPosition, inDistanceFromTarget, viewDirection);
}
void Camera::InitFPS(glm::vec3 pos, float yaw, float pitch) {
	mPosition = pos;
	mYaw = yaw;
	mPitch = pitch;
	RotateFPS(0, 0, 0);
}
void Camera::RotateFPS(float dx, float dy, float sensitivity) {
	mYaw -= dx * sensitivity;
	mPitch -= dy * sensitivity;
	if (mPitch > 89.0f) mPitch = 89.0f;
	if (mPitch < -89.0f) mPitch = -89.0f;
	float yawRad = glm::radians(mYaw);
	float pitchRad = glm::radians(mPitch);
	mViewDirection.x = cosf(pitchRad) * sinf(yawRad);
	mViewDirection.y = sinf(pitchRad);
	mViewDirection.z = cosf(pitchRad) * cosf(yawRad);
	mViewDirection = glm::normalize(mViewDirection);
	glm::vec3 right = glm::normalize(glm::cross(mViewDirection, glm::vec3(0, 1, 0)));
	glm::vec3 up = glm::normalize(glm::cross(right, mViewDirection));
	mViewMatrix = glm::lookAt(mPosition, mPosition + mViewDirection, up);
}
void Camera::MoveFPS(float dt, bool fwd, bool back, bool left, bool right, bool up, bool down, float speed) {
	glm::vec3 forward = glm::normalize(glm::vec3(mViewDirection.x, 0, mViewDirection.z));
	glm::vec3 rightDir = glm::normalize(glm::cross(mViewDirection, glm::vec3(0, 1, 0)));
	glm::vec3 move(0);
	if (fwd) move += forward;
	if (back) move -= forward;
	if (right) move += rightDir;
	if (left) move -= rightDir;
	if (up) move += glm::vec3(0, 1, 0);
	if (down) move -= glm::vec3(0, 1, 0);
	if (glm::length(move) > 0.001f) {
		move = glm::normalize(move);
		mPosition += move * speed * dt;
	}
	glm::vec3 rightVec = glm::normalize(glm::cross(mViewDirection, glm::vec3(0, 1, 0)));
	glm::vec3 upVec = glm::normalize(glm::cross(rightVec, mViewDirection));
	mViewMatrix = glm::lookAt(mPosition, mPosition + mViewDirection, upVec);
}
