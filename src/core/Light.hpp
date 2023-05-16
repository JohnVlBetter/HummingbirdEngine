#pragma once

#include "Macros.h"
#include "Transform.hpp"

class Light {
public:
	glm::vec3 color = glm::vec3(1.0f,1.0f,0.0f);
	float intensity = 10.0f;
	std::shared_ptr<Transform> transform;

	Light() {
		transform = std::make_shared<Transform>();
	}
};

class DirectionalLight : public Light {
public:
	DirectionalLight() {
		transform->Rotate(glm::vec3(75.0f, 40.0f, 0.0f));
	}
};