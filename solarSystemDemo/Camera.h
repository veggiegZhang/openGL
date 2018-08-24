#ifndef __CAMERA_H
#define __CAMERA_H

#include<iostream>
#include"glm/gtx/string_cast.hpp"

#include <glm/glm.hpp>	// include GLM (ideally should only use the GLM headers that are actually used)
#include <glm/gtx/transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
using namespace glm;	// to avoid having to use glm::

class Camera {
public:
	Camera();
	~Camera();

	void update(float moveForward, float moveRight);
	void updateRotation(float yaw, float pitch);
	void updateFOV(float zoom);
	void setViewMatrix(glm::vec3 position, glm::vec3 lookAt, glm::vec3 up);
	void setProjection(float fov, float aspectRatio, float near, float far);
	glm::mat4 getViewMatrix();
	glm::mat4 getProjectionMatrix();
	float getYaw();
	float getPitch();
	std::string getInfo();

private:
	float mYaw;
	float mPitch;
	float mFOV;//field of view
	float mAspectRatio;
	float mNear;
	float mFar;
	glm::vec3 mPosition;
	glm::vec3 mLookAt;
	glm::vec3 mUp;
	glm::mat4 mViewMatrix;
	glm::mat4 mProjectionMatrix;
};

#endif