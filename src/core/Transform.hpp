#pragma once

#include "Macros.h"

class Transform {
public:
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(0.0f);
	glm::vec3 scale = glm::vec3(1.0f);
	//glm::quat scale = glm::quat();

	Transform(){}
	~Transform() {}

	Transform(const glm::vec3& _position, const glm::vec3& _rotation, const glm::vec3& _scale)
		:position(_position),rotation(_rotation),scale(_scale)
	{
		isDirty = true;
	}

	glm::mat4x4 GetLocalToWorldMatrix() {
		if (isDirty) {
			localToWorldMatrix = glm::mat4x4(1.0f);
			localToWorldMatrix = glm::translate(localToWorldMatrix, position);
			//localToWorldMatrix = glm::rotate(,);
			localToWorldMatrix = glm::scale(localToWorldMatrix, scale);
			isDirty = false;
		}
		return localToWorldMatrix;
	}

	void Translate(const glm::vec3& delta) {
		position += delta;
		isDirty = true;
	}

private:
	bool isDirty = true;
	glm::mat4x4 localToWorldMatrix = glm::mat4x4(1.0f);
};