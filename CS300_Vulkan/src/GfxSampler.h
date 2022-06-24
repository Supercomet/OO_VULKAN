#pragma once

#include <vulkan/vulkan.h>

class GfxSamplerManager
{
public:

    void Init();
    void Shutdown();

    static const VkSampler GetDefaultSampler() { return textureSampler; }
    static const VkSampler GetSampler_Deferred() { return deferredSampler; }
    // TODO: Add more sampler objects as needed...

private:

    static VkSampler textureSampler;
    static VkSampler deferredSampler;
    // TODO: Add more sampler objects as needed...
};
