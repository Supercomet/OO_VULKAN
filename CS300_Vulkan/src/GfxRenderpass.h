#pragma once

#include <vector>
#include <memory>
#include <cassert>
#include <iostream>
#include <type_traits>

enum AttachmentIndex
{
    POSITION = 0,
    NORMAL = 1,
    ALBEDO = 2,
    DEPTH = 3,
    // MATERIAL, // TODO
    MAX_GBUFFER_ATTACHMENTS
};

class GfxRenderpass
{
public:
    virtual void Init() = 0;
    virtual void Draw() = 0;
    virtual void Shutdown() = 0;
    uint8_t m_Index{ 0xFF };
};

class RenderPassDatabase
{
public:
    ~RenderPassDatabase();
    static RenderPassDatabase* Get();
    void RegisterRenderPass(std::unique_ptr<GfxRenderpass>&& renderPass);

    // Call this once to call "Init()" on all registered render passes.
    // Take note the order of initialization is undefined.
    static void InitAllRegisteredPasses();

    static void ShutdownAllRegisteredPasses();

    // TODO: Proper C++ Fix needed.
    template<typename T_PASS>
    static inline T_PASS* GetRenderPass()
    {
        for (auto& pass : Get()->m_AllRenderPasses)
        {
            GfxRenderpass* base = pass.get();
            if constexpr (true) // TODO FIX ME
            {
                // This is bad ! Leaving this here as a fallback in case shit happens
                if (T_PASS* derived = dynamic_cast<T_PASS*>(base))
                {
                    return derived;
                }
            }
        }
        return nullptr;
    }

private:
    static RenderPassDatabase* ms_renderpass; // Ideally unique_ptr, for now workaound to fix a (complier) bug?
    std::vector<std::unique_ptr<GfxRenderpass>> m_AllRenderPasses;
    uint8_t m_RegisteredRenderPasses{ 0 };
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
                auto ptr = std::make_unique<pass>();\
                RenderPassDatabase::Get()->RegisterRenderPass(std::move(ptr));\
            }\
    }g_DeclareRenderPass_##pass;\
}// end DeclareRenderPass_ns
