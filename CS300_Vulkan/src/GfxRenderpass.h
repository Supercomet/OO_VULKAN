#pragma once
#include <vector>
#include <memory>
#include <cassert>
#include <iostream>

class GfxRenderpass
{
public:
virtual void Init() = 0;
virtual void Draw() = 0;
virtual void Shutdown() = 0;
~GfxRenderpass() {
    std::cout << "ive been destroyed\n";
}
};

struct RenderPassSingletonWrapper
{
    static std::unique_ptr<RenderPassSingletonWrapper> m_renderpass;
    static RenderPassSingletonWrapper* Get();
    void Add(std::unique_ptr<GfxRenderpass>&& renderPass);
    std::vector<std::unique_ptr<GfxRenderpass>> m_AllRenderPasses;
};


#define DECLARE_RENDERPASS_SINGLETON(pass)\
static pass* Get()\
{\
    assert(m_pass);\
    return m_pass;\
}\
inline static pass* m_pass{nullptr};


// function declares and creates a renderpass automatically at runtime
#define DECLARE_RENDERPASS(pass)\
namespace DeclareRenderPass_ns\
{\
    struct DeclareRenderPass_##pass\
    {\
            DeclareRenderPass_##pass()\
            {\
                std::cout<< __FILE__ << std::endl;\
                auto ptr = std::make_unique<pass>();\
                assert(pass::m_pass == nullptr);\
                pass::m_pass = ptr.get();\
                RenderPassSingletonWrapper::Get()->Add(std::move(ptr));\
                ptr.reset(nullptr);\
            }\
    }g_DeclareRenderPass_##pass;\
}// end DeclareRenderPass_ns
