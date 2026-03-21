#include "Camera.h"
void Camera::Init(glm::vec3 inTargetPosition,float inDistanceFromTarget,glm::vec3 inViewDirection) {
	// Store normalized initial direction for orbit updates.
	inViewDirection=glm::normalize(inViewDirection);
	mInitialViewDirection = inViewDirection;
	mDistanceFromTarget = inDistanceFromTarget;
	mRotateAngle = 0.0;
	Update(inTargetPosition, inDistanceFromTarget, inViewDirection);
}
void Camera::Update(glm::vec3 inTargetPosition, float inDistanceFromTarget, glm::vec3 inViewDirection) {
	// Basic look-at camera: position is offset along reverse view direction.
	glm::vec3 cameraPosition = inTargetPosition - inViewDirection * inDistanceFromTarget;
	glm::vec3 rightDirection = glm::cross(inViewDirection,glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 upDirection = glm::normalize(glm::cross(rightDirection, inViewDirection));
	mPosition = cameraPosition;
	mViewDirection = inViewDirection;
	mViewMatrix=glm::lookAt(cameraPosition, inTargetPosition, upDirection);
}
void Camera::RoundRotate(float inDeltaTime, glm::vec3 inTargetPosition,float inDistanceFromTarget, float inRotateSpeed) {
	// Orbit around target by rotating initial direction around world Y axis.
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