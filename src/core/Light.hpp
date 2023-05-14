#pragma once

#include "Macros.h"

class Light {
public:
	glm::vec3 color = glm::vec3(1.0f,1.0f,0.0f);
	float intensity = 10.0f;
	glm::vec3 rotation = glm::vec3(75.0f, 40.0f, 0.0f);
};

class DirectionalLight : public Light {

};