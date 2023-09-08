/************************************************************************************//*!
\file           ForwardUIPass.h
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief              Declares a gbuffer pass

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#pragma once

#include "GfxRenderpass.h"

struct ForwardUIPass : public GfxRenderpass
{
	//DECLARE_RENDERPASS_SINGLETON(ForwardUIPass)

	void Init() override;
	void Draw() override;
	void Shutdown() override;

	bool SetupDependencies() override;

	void CreatePSO() override;
	
private:
	void SetupRenderpass();
	void SetupFramebuffer();
	void CreatePipeline();

};
