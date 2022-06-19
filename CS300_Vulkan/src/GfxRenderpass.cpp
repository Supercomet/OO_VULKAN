#include "GfxRenderpass.h"

std::unique_ptr<RenderPassSingletonWrapper> RenderPassSingletonWrapper::m_renderpass{ nullptr };

RenderPassSingletonWrapper* RenderPassSingletonWrapper::Get()
{
	if (m_renderpass.get() == nullptr)
	{
		std::cout << "created new singleton\n";
		m_renderpass = std::make_unique <RenderPassSingletonWrapper>();
	}
	return m_renderpass.get();
}

void RenderPassSingletonWrapper::Add(std::unique_ptr<GfxRenderpass>&& renderPass)
{
	std::cout << "addr" << &m_AllRenderPasses << std::endl;
	m_AllRenderPasses.emplace_back(std::move(renderPass));
}
