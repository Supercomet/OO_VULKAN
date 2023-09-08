/************************************************************************************//*!
\file           DebugRenderpass.h
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief              Defines a debg drawing pass

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#pragma once

#include "GfxRenderpass.h"

// This render pass support the drawing for debug objects, typically for editor or debugging purpose and not shipped in the final product.
// What kinds of things should this renderer do at this level? (non-exhaustive)
// - Drawing of points, lines, triangles, alpha tested or not.
// - Drawing of wireframe and solid meshes, alpha tested or not.
// - Drawing of debug text. (although you can just piggyback ImGui)
// - Support of timed and persistent debug drawing. (Draw for 5 seconds, draw permanently, etc)
class DebugDrawRenderpass : public GfxRenderpass
{
public:

	void Init() override;
	void Draw() override;
	void Shutdown() override;

	bool SetupDependencies() override;

private:

	void CreateDebugRenderpass();
	void CreatePipeline();
	void InitDebugBuffers();
};

