#pragma once

#include "MathCommon.h"
#include "MeshModel.h"
#include "Vulkanrenderer.h"

bool BoolQueryUser(const char* str);
void UpdateBV(ModelData* model, VulkanRenderer::EntityDetails& entity, int i = 0);

enum class AppWindowSizeTypes : int
{
    HQ_600P_4_3,
    HD_720P_16_9,
    HD_900P_16_10
};

static glm::ivec2 gs_AppWindowSizes[] =
{
    glm::ivec2{ 800, 600 },
    glm::ivec2{ 1280, 720 },
    glm::ivec2{ 1440, 900 },
};
