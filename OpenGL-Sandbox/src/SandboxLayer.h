#pragma once

#include <GLCore.h>
#include <GLCoreUtils.h>

class SandboxLayer : public GLCore::Layer
{
public:
	SandboxLayer(uint32_t width, uint32_t height);
	virtual ~SandboxLayer();

	virtual void OnAttach() override;
	virtual void OnDetach() override;
	virtual void OnEvent(GLCore::Event& event) override;
	virtual void OnUpdate(GLCore::Timestep ts) override;
	virtual void OnImGuiRender() override;
private:
	GLCore::Utils::Shader* m_Shader;
};