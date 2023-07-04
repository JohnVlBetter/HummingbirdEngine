#pragma once

#include "Macros.h"
#include "Camera.hpp"
#include "ForwardRenderer.hpp"

class ScriptableRenderPipeline
{
protected:
public:
	std::string name;

	ScriptableRenderPipeline() {
		name = "ScriptableRenderPipeline";
	}

	virtual ~ScriptableRenderPipeline() {
	}

	virtual void Render(std::vector<std::shared_ptr<Camera>> cameras) = 0;
};