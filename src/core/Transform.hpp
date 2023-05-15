#pragma once

#include "Macros.h"

class Transform {
public:
	glm::vec3 position = glm::vec3(0.0f);
	glm::quat rotation = glm::quat(1, 0, 0, 0);
	glm::vec3 scale = glm::vec3(1.0f);

	Transform(){}
	~Transform() {}

	Transform(const glm::vec3& _position, const glm::quat& _rotation, const glm::vec3& _scale)
		:position(_position),rotation(_rotation),scale(_scale)
	{
		isDirty = true;
	}

	Transform(const glm::mat4 mat) : localToWorldMatrix(mat){
		isDirty = true;
	}

	glm::mat4 GetLocalToWorldMatrix() {
		if (isDirty) {
			localToWorldMatrix = 
				glm::translate(glm::mat4(1.0f), position) * 
				glm::mat4(rotation) * 
				glm::scale(glm::mat4(1.0f), scale);
			isDirty = false;
		}
		return localToWorldMatrix;
	}

	void Translate(const glm::vec3& _position) {
		position += _position;
		isDirty = true;
	}

	void Rotate(float _angle, const glm::vec3& _axis) {
		rotation = rotation * glm::angleAxis(_angle, _axis);
		isDirty = true;
	}
	
	void Rotate(const glm::quat& _q) {
		rotation = rotation * _q;
		isDirty = true;
	}

	void Rotate(const glm::vec3& _eulerAngles) {
		rotation = rotation * glm::quat(_eulerAngles);
		isDirty = true;
	}

	void Scale(const glm::vec3& _scale) {
		SetScale(_scale);
	}

	void SetScale(const glm::vec3& _scale) {
		scale = _scale;
		isDirty = true;
	}

	void SetRotation(const glm::quat& _q) {
		rotation = _q;
		isDirty = true;
	}

	void SetPositon(const glm::vec3& _position) {
		position = _position;
		isDirty = true;
	}

private:
	bool isDirty = true;
	glm::mat4 localToWorldMatrix = glm::mat4(1.0f);
};