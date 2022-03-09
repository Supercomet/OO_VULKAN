#pragma once
#ifndef KEYCODES_H
#define KEYCODES_H

#include <cstdint>
#include "keycodes.h"

#include "glm/glm.hpp"

struct Input
{

static bool keysTriggered[1024];
static bool keysHeld[1024];
static bool keysRelease[1024];
static glm::vec2 mousePos;
static glm::vec2 mouseChange;

static short wheelDelta;

enum MouseButton
{
	left = 0,
	right = 1,
	middle = 2,
};


static bool mouseButtonTriggered[3];
static bool mouseButtonHeld[3];
static bool mouseButtonRelease[3]; 

static void Begin();

static bool GetKeyTriggered(int32_t key);

static bool GetKeyHeld(int32_t key);

static bool GetKeyReleased(int32_t key);

static void HandleMouseMove(int32_t x, int32_t y);

static glm::vec2 GetMouseDelta();
static glm::vec2 GetMousePos();

static float GetMouseWheel();


}; // namespace Input


#endif
