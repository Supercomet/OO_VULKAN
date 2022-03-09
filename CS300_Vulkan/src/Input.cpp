#include "Input.h"

bool Input::keysTriggered[1024];
bool Input::keysHeld[1024];
bool Input::keysRelease[1024];
glm::vec2 Input::mousePos{};
glm::vec2 Input::mouseChange{};

bool Input::mouseButtonTriggered[3];
bool Input::mouseButtonHeld[3];
bool Input::mouseButtonRelease[3]; 

short Input::wheelDelta{0};

void Input::Begin()
{
	mouseChange={};
	for (size_t i = 0; i < 1024; ++i)
	{
		keysTriggered[i] = false;
	}
}

 bool Input::GetKeyTriggered(int32_t key)
{
	return keysTriggered[key];
}

 bool Input::GetKeyHeld(int32_t key)
{
	return keysHeld[key];
}

 bool Input::GetKeyReleased(int32_t key)
{
	return keysRelease[key];
}

 void Input::HandleMouseMove(int32_t x, int32_t y)
 {
	 int32_t dx = (int32_t)mousePos.x - x;
	 int32_t dy = (int32_t)mousePos.y - y;

	 mouseChange = { dx,dy };

	 bool handled = false;

	 //if (settings.overlay) {
	//	 ImGuiIO& io = ImGui::GetIO();
	//	 handled = io.WantCaptureMouse;
	 //}
	 //mouseMoved((float)x, (float)y, handled);

	// if (handled) {
	//	 mousePos = glm::vec2((float)x, (float)y);
	//	 return;
	// }

	 mousePos = glm::vec2((float)x, (float)y);
 }

 glm::vec2 Input::GetMouseDelta()
 {
	 return mouseChange;
 }

 glm::vec2 Input::GetMousePos()
 {
	 return mousePos;
 }

 float Input::GetMouseWheel()
 {
	 return static_cast<float>(wheelDelta);
 }

