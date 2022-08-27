#pragma once

#include "Camera.h"

class CameraController
{
public:

    void SetCamera(Camera* camera) { m_Camera = camera; }

private:

    Camera* m_Camera{ nullptr };
};
