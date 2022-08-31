#include "CameraController.h"

#include "Input.h"

void CameraController::Update(float dt)
{
    if (m_Camera == nullptr)
        return;

    // Poll input states
    const bool left  = Input::GetKeyHeld(KEY_A);
    const bool right = Input::GetKeyHeld(KEY_D);
    const bool down  = Input::GetKeyHeld(KEY_S);
    const bool up    = Input::GetKeyHeld(KEY_W);
    const glm::vec2 mousedelta = Input::GetMouseDelta();
    const float wheelDelta = Input::GetMouseWheel();

    const glm::vec3 amountToRotate = glm::vec3{ -mousedelta.y * m_Camera->rotationSpeed, mousedelta.x * m_Camera->rotationSpeed, 0.0f };
    glm::vec3 amountToTranslate{ 0.0f, 0.0f, 0.0f };

    // Camera Movement
    if (m_Camera->m_CameraMovementType == Camera::CameraMovementType::firstperson)
    {
        const float moveSpeed = dt * m_Camera->movementSpeed;
        if (up)
            amountToTranslate -= m_Camera->GetFront() * moveSpeed;
        if (down)
            amountToTranslate += m_Camera->GetFront() * moveSpeed;
        if (left)
            amountToTranslate -= m_Camera->GetRight() * moveSpeed;
        if (right)
            amountToTranslate += m_Camera->GetRight() * moveSpeed;
    }
    
    // Simulate some very simple fucked up camera shake
    if (m_CameraShakeDuration > 0.0f)
    {
        m_CameraShakeDuration -= dt;
        amountToTranslate -= m_LastFrameCameraShakeOffset;

        glm::vec3 shakeOffset{ 0.0f,0.0f,0.0f };
        float shakeAmount = 0.01f;
        shakeOffset = glm::sphericalRand(1.0f) * shakeAmount;

        if (m_CameraShakeDuration < 0.0001f)
        {
            m_CameraShakeDuration = 0.0f;
            m_LastFrameCameraShakeOffset = glm::vec3{ 0.0f,0.0f,0.0f };
        }
        else
        {
            m_LastFrameCameraShakeOffset = shakeOffset;
            amountToTranslate += shakeOffset;
        }
    }

    m_Camera->Translate(amountToTranslate);

    // Camera Rotation
    if (Input::GetMouseHeld(MOUSE_RIGHT))
    {
        m_Camera->Rotate(amountToRotate);
    }

    if (m_Camera->m_CameraMovementType == Camera::CameraMovementType::lookat)
    {
        // Move closer or further in the direction of the lookat target
        constexpr float targetDistanceSpeed = 0.001f;
        m_Camera->ChangeTargetDistance(-wheelDelta * targetDistanceSpeed);
    }

    // Update View & Projection Matrix
    m_Camera->UpdateProjectionMatrix();
}

void CameraController::ShakeCamera()
{
    if (m_Camera == nullptr)
        return;
    
    m_CameraShakeDuration = 0.5f;

    if (m_CameraShakeDuration < 0.0001f)
    {
        m_LastFrameCameraShakeOffset = glm::vec3{ 0.0f,0.0f,0.0f };
    }
}