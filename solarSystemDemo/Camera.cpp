#include "Camera.h"

Camera::Camera()
{
	// initialise camera member variables
	mPosition = glm::vec3(0.0f, 0.0f, 1.0f);
	mLookAt = glm::vec3(0.0f, 0.0f, 0.0f);
	mUp = glm::vec3(0.0f, 1.0f, 0.0f);
	
	mYaw = 0.0f;
	mPitch = 0.0f;
	mFOV = 45.0f;

	mViewMatrix = glm::lookAt(mPosition, mLookAt, mUp);
	mProjectionMatrix = glm::perspective(mFOV, 1.0f, 0.1f, 100.0f);
}

Camera::~Camera()
{}

void Camera::update(float moveForward, float moveRight)
{
	// rotate the respective unit vectors about the y-axis
	glm::vec3 rotatedForwardVec = glm::rotateY(glm::vec3(0.0f, 0.0f, -1.0f), mYaw);
	glm::vec3 rotatedRightVec = glm::rotateY(glm::vec3(1.0f, 0.0f, 0.0f), mYaw);
	// rotate the rotated forward vector about the rotated right vector by the pitch
	rotatedForwardVec = glm::vec3(glm::rotate(mPitch, rotatedRightVec)*glm::vec4(rotatedForwardVec, 0.0f));

	// update camera position, look-at position and up vectors
	mPosition += rotatedForwardVec * moveForward + rotatedRightVec * moveRight;
	mLookAt = mPosition + rotatedForwardVec;
	mUp = glm::cross(rotatedRightVec, rotatedForwardVec);

	// compute the new view matrix
	mViewMatrix = glm::lookAt(mPosition, mLookAt, mUp);
}

void Camera::updateRotation(float yaw, float pitch)
{
	mYaw += yaw;
	mPitch += pitch;
}

void Camera::updateFOV(float zoom)
{
	mFOV += zoom;

	// compute the new projection matrix
	mProjectionMatrix = glm::perspective(mFOV, mAspectRatio, mNear, mFar);
}

void Camera::setViewMatrix(glm::vec3 position, glm::vec3 lookAt, glm::vec3 up)
{
	mPosition = position;
	mLookAt = lookAt;
	mUp = up;

	mViewMatrix = glm::lookAt(mPosition, mLookAt, mUp);
}

void Camera::setProjection(float fov, float aspectRatio, float near, float far)
{
	mFOV = fov;
	mAspectRatio = aspectRatio;
	mNear = near;
	mFar = far;
	mProjectionMatrix = glm::perspective(mFOV, mAspectRatio, mNear, mFar);
}

glm::mat4 Camera::getViewMatrix()
{
	return mViewMatrix;
}

glm::mat4 Camera::getProjectionMatrix()
{
	return mProjectionMatrix;
}

float Camera::getYaw()
{
	return 	mYaw;
}

float Camera::getPitch()
{
	return 0.1f;
}

std::string Camera::getInfo()
{
	return "position: " + glm::to_string(mPosition) + "\n" +
		   "lookAt: " + glm::to_string(mLookAt) + "\n" +
		   "up: " + glm::to_string(mUp);
}