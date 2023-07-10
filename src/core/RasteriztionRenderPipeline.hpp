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

		renderer->Setup();
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
			CameraData* cameraData = new CameraData();
			InitializeCameraData(camera, cameraData);
			RenderSingleCamera(cameraData);
		}
	}

	void InitializeCameraData(std::shared_ptr<Camera> camera, CameraData* cameraData) {
		//test
		cameraData->camera = camera;
		cameraData->renderer = renderer;
	}

	static RenderingData* InitializeRenderingData(CameraData* cameraData, CullingResults* cullingResult) {
		return new RenderingData();
	}
	
	static CullingResults* Cull() {
		return new CullingResults();
	}

	static void RenderSingleCamera(CameraData* cameraData) {
		auto camera = cameraData->camera;
		auto renderer = cameraData->renderer;

		if (renderer == nullptr) {
			LOG_WARN("Trying to render {} with an invalid renderer. Camera rendering will be skipped.", camera->name);
			return;
		}

		renderer->Clear();

		auto cullingResult = Cull();

		auto renderingData = InitializeRenderingData(cameraData, cullingResult);

		renderer->Execute(renderingData);
	}
};