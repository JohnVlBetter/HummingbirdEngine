#pragma once

#include "ScriptableRenderPipeline.hpp"

class RasteriztionRenderPipeline : public ScriptableRenderPipeline
{
private:
	static bool cameraComparison(std::shared_ptr<Camera> c1, std::shared_ptr<Camera> c2) {
		return c1->depth > c2->depth;
	}
public:
	RasteriztionRenderPipeline() {
		name = "RasteriztionRenderPipeline";
	}

	~RasteriztionRenderPipeline() {
	}

	void SetupPerFrameShaderConstants() {
		
	}

	void SortCameras(std::vector<std::shared_ptr<Camera>> cameras)
	{
		if (cameras.size() > 1) std::sort(cameras.begin(), cameras.end(), cameraComparison);
	}

	void Render(std::vector<std::shared_ptr<Camera>> cameras) {
		SetupPerFrameShaderConstants();

		SortCameras(cameras);

		for (int i = 0; i < cameras.size(); ++i) {
			auto camera = cameras[i];
			std::shared_ptr<CameraData> cameraData = std::make_shared<CameraData>();
			InitializeCameraData(camera, cameraData);
			RenderSingleCamera(cameraData);
		}
	}

	static void InitializeCameraData(std::shared_ptr<Camera> camera, std::shared_ptr<CameraData> cameraData) {
		//test
		cameraData->camera = camera;
		cameraData->renderer = std::make_shared<ForwardRenderer>();
	}

	static std::shared_ptr<RenderingData> InitializeRenderingData(std::shared_ptr<CameraData> cameraData, std::shared_ptr<CullingResults> camera) {
		return std::make_shared<RenderingData>();
	}
	
	static std::shared_ptr<CullingResults> Cull() {
		return std::make_shared<CullingResults>();
	}

	static void RenderSingleCamera(std::shared_ptr<CameraData> cameraData) {
		auto camera = cameraData->camera;
		auto renderer = cameraData->renderer;

		if (renderer == nullptr) {
			LOG_WARN("Trying to render {} with an invalid renderer. Camera rendering will be skipped.", camera->name);
			return;
		}

		renderer->Clear();

		auto cullingResult = Cull();

		auto renderingData = InitializeRenderingData(cameraData, cullingResult);

		renderer->Setup(renderingData);

		renderer->Execute(renderingData);
	}
};