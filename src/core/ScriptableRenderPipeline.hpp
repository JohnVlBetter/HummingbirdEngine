#pragma once

#include "Macros.h"
#include "Camera.hpp"
#include "ForwardRenderer.hpp"

class ScriptableRenderPipeline
{
protected:
public:
	std::string name;
	std::shared_ptr<ForwardRenderer> renderer;

	ScriptableRenderPipeline() {
		name = "ScriptableRenderPipeline";
		renderer = std::make_shared<ForwardRenderer>();
	}

	virtual ~ScriptableRenderPipeline() {
	}

	virtual void Render(std::vector<std::shared_ptr<Camera>> cameras) = 0;
};