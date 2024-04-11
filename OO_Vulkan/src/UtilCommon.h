/************************************************************************************//*!
\file           UtilCommon.h
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Mar 05, 2024
\brief              Common utility

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#pragma once

#define OO_ASSERT(BoolCondition) do { if (!(BoolCondition)) { __debugbreak(); } } while (0)

#ifdef _MSC_VER
#ifndef OO_OPTIMIZE_OFF
	#define OO_OPTIMIZE_OFF __pragma(optimize( "", off ))
#endif // !OO_OPTIMIZE_OFF
#ifndef OO_OPTIMIZE_ON
	#define OO_OPTIMIZE_ON __pragma(optimize( "", on ))
#endif // !OO_OPTIMIZE_ON

#else
#ifndef OO_OPTIMIZE_OFF
	#define OO_OPTIMIZE_OFF _Pragma(STRINGIFY(optimize( "", off )))
#endif // !OO_OPTIMIZE_OFF
#ifndef OO_OPTIMIZE_ON
	#define OO_OPTIMIZE_ON _Pragma(STRINGIFY(optimize( "", on )))
#endif // !OO_OPTIMIZE_ON
#endif
