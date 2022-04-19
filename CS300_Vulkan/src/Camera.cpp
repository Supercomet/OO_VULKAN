#include "Camera.h"
#include <iostream>
#include "glm/gtx/euler_angles.hpp"

void Camera::updateViewMatrix()
{
	glm::mat4 transM;
	
	rotation.x = std::max(rotation.x, -89.0f);
	rotation.x = std::min(rotation.x,  89.0f);

	glm::vec3 from{ 0.0f,0.0f,distance };
	glm::vec3 worldUp(0.0f, -1.0f, 0.0f);

	
	glm::mat4 rotM = glm::mat4{ 1.0f };
	//rotM = glm::rotate(rotM, glm::radians(rotation.x * (flipY ? -1.0f : 1.0f)), glm::vec3(1.0f, 0.0f, 0.0f));
	//rotM = glm::rotate(rotM, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	//rotM = glm::rotate(rotM, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	rotM = glm::eulerAngleYXZ(glm::radians(rotation.y),glm::radians(rotation.x), glm::radians(rotation.z));
	glm::vec3 temp = glm::mat3(rotM) * from;
	from = temp;

	position = target + from;

	
	worldUp = glm::eulerAngleZ(glm::radians(rotation.z)) * glm::vec4(worldUp,0.0f);

	forward = position- target;


	right = glm::cross(forward, worldUp);
	up = glm::cross(forward, right);

	forward = glm::normalize(forward);
	right = glm::normalize(right);
	up = glm::normalize(up);

	matrices.view = glm::mat4{
	 right.x,  up.x,  forward.x,  0.0f,
	 right.y,  up.y,  forward.y,  0.0f,
	 right.z,  up.z,  forward.z,  0.0f,
	 -glm::dot(position,right),   -glm::dot(position,up),   -glm::dot(position,forward),  1.0f
	};

	updated = true;
}

bool Camera::Moving()
{
	return keys.left || keys.right || keys.up || keys.down;
}

float Camera::GetNearClip() 
{
	return znear;
}

float Camera::GetFarClip() 
{
	return zfar;
}

void Camera::SetPerspective(float fov, float aspect, float znear, float zfar)
{
	this->fov = fov;
	this->znear = znear;
	this->zfar = zfar;
	matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
	if (flipY) {
		matrices.perspective[1][1] *= -1.0f;
	}
}

void Camera::UpdateAspectRatio(float aspect)
{
	matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
	if (flipY) {
		matrices.perspective[1][1] *= -1.0f;
	}
}

void Camera::SetPosition(glm::vec3 position)
{
	this->position = position;
	updateViewMatrix();
}

void Camera::SetRotation(glm::vec3 rotation)
{
	this->rotation = rotation;
	updateViewMatrix();
}

void Camera::Rotate(glm::vec3 delta)
{
	this->rotation += delta;
	updateViewMatrix();
}

void Camera::SetTranslation(glm::vec3 translation)
{
	if(type == Camera::CameraType::lookat)
	{
		this->target = translation;
	}
	else
	{
		this->position = translation;
	}
	updateViewMatrix();
}

void Camera::Translate(glm::vec3 delta)
{
	if(type == Camera::CameraType::lookat)
	{
		this->target += delta;
	}
	else
	{
		this->position += delta;
	}
	updateViewMatrix();
}

void Camera::SetRotationSpeed(float rotationSpeed)
{
	this->rotationSpeed = rotationSpeed;
}

void Camera::SetMovementSpeed(float movementSpeed)
{
	this->movementSpeed = movementSpeed;
}

void Camera::ChangeDistance(float delta)
{
	distance = std::max(1.0f, delta + distance);
	updateViewMatrix();
}

void Camera::Update(float deltaTime)
{
	updated = false;
	if (type == CameraType::firstperson)
	{
		if (Moving())
		{
			glm::vec3 camFront;
			camFront.x = -cos(glm::radians(rotation.x)) * sin(glm::radians(rotation.y));
			camFront.y = sin(glm::radians(rotation.x));
			camFront.z = cos(glm::radians(rotation.x)) * cos(glm::radians(rotation.y));
			camFront = glm::normalize(camFront);

			float moveSpeed = deltaTime * movementSpeed;

			if (keys.up)
				position += camFront * moveSpeed;
			if (keys.down)
				position -= camFront * moveSpeed;
			if (keys.left)
				position -= glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;
			if (keys.right)
				position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed;

			updateViewMatrix();
		}
	}
}
glm::vec3 Camera::GetFront()
{
	return forward;
}
glm::vec3 Camera::GetRight()
{
	return right;
}
glm::vec3 Camera::GetUp()
{
	return up;
}


bool Camera::UpdatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime)

{
	bool retVal = false;

	if (type == CameraType::firstperson)
	{
		// Use the common console thumbstick layout		
		// Left = view, right = move

		const float deadZone = 0.0015f;
		const float range = 1.0f - deadZone;

		glm::vec3 camFront = GetFront();

		float moveSpeed = deltaTime * movementSpeed * 2.0f;
		float rotSpeed = deltaTime * rotationSpeed * 50.0f;

		// Move
		if (fabsf(axisLeft.y) > deadZone)
		{
			float pos = (fabsf(axisLeft.y) - deadZone) / range;
			position -= camFront * pos * ((axisLeft.y < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
			retVal = true;
		}
		if (fabsf(axisLeft.x) > deadZone)
		{
			float pos = (fabsf(axisLeft.x) - deadZone) / range;
			position += glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * pos * ((axisLeft.x < 0.0f) ? -1.0f : 1.0f) * moveSpeed;
			retVal = true;
		}

		// Rotate
		if (fabsf(axisRight.x) > deadZone)
		{
			float pos = (fabsf(axisRight.x) - deadZone) / range;
			rotation.y += pos * ((axisRight.x < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
			retVal = true;
		}
		if (fabsf(axisRight.y) > deadZone)
		{
			float pos = (fabsf(axisRight.y) - deadZone) / range;
			rotation.x -= pos * ((axisRight.y < 0.0f) ? -1.0f : 1.0f) * rotSpeed;
			retVal = true;
		}
	}
	else
	{
		// todo: move code from example base class for look-at
	}

	if (retVal)
	{
		updateViewMatrix();
	}

	return retVal;
}
