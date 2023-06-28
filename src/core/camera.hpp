#pragma once

#include "Macros.h"
#include "Transform.hpp"

class Camera
{
private:
	float fov;
	float znear, zfar;

	void updateViewMatrix()
	{
		glm::mat4 rotM = glm::mat4(1.0f);
		rotM = glm::rotate(rotM, glm::radians(transform->eulerAngle.x), glm::vec3(1.0f, 0.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(transform->eulerAngle.y), glm::vec3(0.0f, 1.0f, 0.0f));
		rotM = glm::rotate(rotM, glm::radians(transform->eulerAngle.z), glm::vec3(0.0f, 0.0f, 1.0f));
		glm::mat4 transM = glm::translate(glm::mat4(1.0f), transform->position * glm::vec3(1.0f, 1.0f, -1.0f));

		if (type == CameraType::firstperson) matrices.view = rotM * transM;
		else matrices.view = transM * rotM;

		updated = true;
	};
public:
	enum CameraType { lookat, firstperson };
	CameraType type = CameraType::lookat;

	std::shared_ptr<Transform> transform;

	float rotationSpeed = 1.0f;
	float movementSpeed = 1.0f;

	bool updated = false;

	struct
	{
		glm::mat4 perspective;
		glm::mat4 view;
	} matrices;

	struct
	{
		bool left = false;
		bool right = false;
		bool up = false;
		bool down = false;
	} keys;

	Camera() {
		transform = std::make_shared<Transform>();
	}

	bool moving()
	{
		return keys.left || keys.right || keys.up || keys.down;
	}

	float getNearClip() {
		return znear;
	}

	float getFarClip() {
		return zfar;
	}

	void setPerspective(float fov, float aspect, float znear, float zfar)
	{
		this->fov = fov;
		this->znear = znear;
		this->zfar = zfar;
		matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
	};

	void updateAspectRatio(float aspect)
	{
		matrices.perspective = glm::perspective(glm::radians(fov), aspect, znear, zfar);
	}

	void setPosition(glm::vec3 position)
	{
		this->transform->SetPositon(position);
		updateViewMatrix();
	}

	void setRotation(glm::vec3 rotation)
	{
		this->transform->SetRotation(rotation);
		updateViewMatrix();
	};

	void rotate(glm::vec3 delta)
	{
		this->transform->Rotate(delta);
		updateViewMatrix();
	}

	void setTranslation(glm::vec3 translation)
	{
		this->transform->SetPositon(translation);
		updateViewMatrix();
	};

	void translate(glm::vec3 delta)
	{
		this->transform->Translate(delta);
		updateViewMatrix();
	}

	void update(float deltaTime)
	{
		updated = false;
		if (type == CameraType::firstperson)
		{
			if (moving())
			{
				glm::vec3 camFront;
				camFront.x = -cos(glm::radians(transform->eulerAngle.x)) * sin(glm::radians(transform->eulerAngle.y));
				camFront.y = sin(glm::radians(transform->eulerAngle.x));
				camFront.z = cos(glm::radians(transform->eulerAngle.x)) * cos(glm::radians(transform->eulerAngle.y));
				camFront = glm::normalize(camFront);

				float moveSpeed = deltaTime * movementSpeed;

				if (keys.up)
					this->transform->Translate(camFront * moveSpeed);
				if (keys.down)
					this->transform->Translate(-camFront * moveSpeed);
				if (keys.left)
					this->transform->Translate(-glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed);
				if (keys.right)
					this->transform->Translate(glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * moveSpeed);

				updateViewMatrix();
			}
		}
	};

	// Update camera passing separate axis data (gamepad)
	// Returns true if view or position has been changed
	bool updatePad(glm::vec2 axisLeft, glm::vec2 axisRight, float deltaTime)
	{
		bool retVal = false;

		if (type == CameraType::firstperson)
		{
			// Use the common console thumbstick layout		
			// Left = view, right = move

			const float deadZone = 0.0015f;
			const float range = 1.0f - deadZone;

			glm::vec3 camFront;
			camFront.x = -cos(glm::radians(transform->eulerAngle.x)) * sin(glm::radians(transform->eulerAngle.y));
			camFront.y = sin(glm::radians(transform->eulerAngle.x));
			camFront.z = cos(glm::radians(transform->eulerAngle.x)) * cos(glm::radians(transform->eulerAngle.y));
			camFront = glm::normalize(camFront);

			float moveSpeed = deltaTime * movementSpeed * 2.0f;
			float rotSpeed = deltaTime * rotationSpeed * 50.0f;

			// Move
			if (fabsf(axisLeft.y) > deadZone)
			{
				float pos = (fabsf(axisLeft.y) - deadZone) / range;
				this->transform->Translate(-camFront * pos * ((axisLeft.y < 0.0f) ? -1.0f : 1.0f) * moveSpeed);
				retVal = true;
			}
			if (fabsf(axisLeft.x) > deadZone)
			{
				float pos = (fabsf(axisLeft.x) - deadZone) / range;
				this->transform->Translate(glm::normalize(glm::cross(camFront, glm::vec3(0.0f, 1.0f, 0.0f))) * pos * ((axisLeft.x < 0.0f) ? -1.0f : 1.0f) * moveSpeed);
				retVal = true;
			}

			// Rotate
			if (fabsf(axisRight.x) > deadZone)
			{
				float pos = (fabsf(axisRight.x) - deadZone) / range;
				transform->Rotate(pos * ((axisRight.x < 0.0f) ? -1.0f : 1.0f) * rotSpeed, glm::vec3(0, 1, 0));
				retVal = true;
			}
			if (fabsf(axisRight.y) > deadZone)
			{
				float pos = (fabsf(axisRight.y) - deadZone) / range;
				transform->Rotate(-pos * ((axisRight.y < 0.0f) ? -1.0f : 1.0f) * rotSpeed, glm::vec3(1, 0, 0));
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

};