/************************************************************************************//*!
\file           GraphicsBatch.cpp
\project        Ouroboros
\author         Jamie Kong, j.kong, 390004720 | code contribution (100%)
\par            email: j.kong\@digipen.edu
\date           Oct 02, 2022
\brief              Defines GraphicsBatch, a generator for command lists for objects that require to be rendered.

Copyright (C) 2022 DigiPen Institute of Technology.
Reproduction or disclosure of this file or its contents
without the prior written consent of DigiPen Institute of
Technology is prohibited.
*//*************************************************************************************/
#include "GraphicsBatch.h"

#include "GraphicsWorld.h"
#include "VulkanRenderer.h"
#include "MathCommon.h"
#include "OctTree.h"
#include "gpuCommon.h"
#include <cassert>
#include "Profiling.h"
#include "DebugDraw.h"

//#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING
//#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
//#include <locale>
//#include <codecvt>

#include <sstream>
#include <numeric>

DrawData ObjectInsToDrawData(const ObjectInstance& obj)
{
	DrawData dd;
	dd.bindlessGlobalTextureIndex_Albedo = obj.bindlessGlobalTextureIndex_Albedo;
	dd.bindlessGlobalTextureIndex_Normal = obj.bindlessGlobalTextureIndex_Normal;
	dd.bindlessGlobalTextureIndex_Roughness = obj.bindlessGlobalTextureIndex_Roughness;
	dd.bindlessGlobalTextureIndex_Metallic = obj.bindlessGlobalTextureIndex_Metallic;
	dd.bindlessGlobalTextureIndex_Emissive = obj.bindlessGlobalTextureIndex_Emissive;
	dd.emissiveColour = obj.emissiveColour;
	dd.localToWorld = obj.localToWorld;
	dd.prevLocalToWorld = obj.prevLocalToWorld;
	dd.entityID = obj.entityID; // Unique ID for this entity instance
	dd.flags = obj.flags;
	dd.instanceData = obj.instanceData;
	dd.ptrToBoneBuffer = &obj.bones;
	dd.modelID = obj.modelID;
	dd.submeshID = obj.submesh;
	return dd;
}

oGFX::AABB getBoxFun(ObjectInstance& oi)
{
	auto& vr = *VulkanRenderer::get();
	auto& models = vr.g_globalModels;
	auto& submeshes = vr.g_globalSubmesh;

	oGFX::AABB box;
	auto& mdl = models[oi.modelID];
	uint32_t submeshID = mdl.m_subMeshes[oi.submesh];

	oGFX::Sphere bs = submeshes[submeshID].boundingSphere;

	float sx = glm::length(glm::vec3(oi.localToWorld[0][0], oi.localToWorld[1][0], oi.localToWorld[2][0]));
	float sy = glm::length(glm::vec3(oi.localToWorld[0][1], oi.localToWorld[1][1], oi.localToWorld[2][1]));
	float sz = glm::length(glm::vec3(oi.localToWorld[0][2], oi.localToWorld[1][2], oi.localToWorld[2][2]));

	box.center = vec3(oi.localToWorld * vec4(bs.center, 1.0));
	float maxSize = std::max(sx, std::max(sy, sz));
	maxSize *= bs.radius;
	box.halfExt = vec3{ maxSize };
	return box;
};
OO_OPTIMIZE_OFF
void CullDrawData(const oGFX::Frustum& f, std::vector<DrawData>& outData, const std::vector<ObjectInstance*>& contained, const std::vector<ObjectInstance*>& intersecting, bool debug = false)
{
	PROFILE_SCOPED();
	auto& vr = *VulkanRenderer::get();

	outData.clear();
	outData.reserve(contained.size() + intersecting.size());	

	for (size_t i = 0; i < contained.size(); i++)
	{
		ObjectInstance& src = *contained[i];

		if (src.isRenderable() == false) continue;

		if(debug)
			oGFX::DebugDraw::AddAABB(getBoxFun(src), oGFX::Colors::GREEN);
		DrawData dd = ObjectInsToDrawData(src);
		gfxModel& mdl = vr.g_globalModels[src.modelID];
		//for (size_t s = 0; s < mdl.m_subMeshes.size(); s++)
		//{
		//	// add draw call for each submesh
		//	if (src.submesh[s] == true)
		//	{
		//		dd.submeshID = mdl.m_subMeshes[s];
				dd.submeshID = mdl.m_subMeshes[src.submesh];
				outData.push_back(dd);
		//	}
		//}

	}
	size_t intersectAccepted{};
	for (size_t i = 0; i < intersecting.size(); i++)
	{
		ObjectInstance& src = *intersecting[i];

		if (src.isRenderable() == false) continue;
		if (debug)
			oGFX::DebugDraw::AddAABB(getBoxFun(src), oGFX::Colors::RED);
		if (oGFX::coll::AABBInFrustum(f, getBoxFun(src), debug) != oGFX::coll::OUTSIDE) {
			DrawData dd = ObjectInsToDrawData(src);
			gfxModel& mdl = vr.g_globalModels[src.modelID];
			//for (size_t s = 0; s < mdl.m_subMeshes.size(); s++)
			//{
			//	// add draw call for each submesh
			//	if (src.submesh[s] == true)
			//	{
			//		dd.submeshID = mdl.m_subMeshes[s];
					dd.submeshID = mdl.m_subMeshes[src.submesh];
					outData.push_back(dd);
			//	}
			//}
			if (debug)
				oGFX::DebugDraw::AddAABB(getBoxFun(src), oGFX::Colors::YELLOW);
			intersectAccepted++;
		}
	}
	//if(debug)
	//	printf("Accepted Entities-%3llu/%3llu Intersect-%3llu/%3llu\n", outData.size(), contained.size() + intersecting.size(), intersectAccepted, intersecting.size());
}
OO_OPTIMIZE_ON
void SortDrawDataByMesh(std::vector<DrawData>& drawData)
{
	std::sort(drawData.begin(), drawData.end(),
		[](const DrawData& L, const DrawData& R) {return L.submeshID < R.submeshID; });
}

void AppendBatch(std::vector<oGFX::IndirectCommand>& dest, oGFX::IndirectCommand cmd, uint32_t cnt)
{
	if (cnt > 0)
	{
		cmd.instanceCount = cnt;
		dest.emplace_back(cmd);
	}
}

void GenerateCommands(const std::vector<DrawData>& entities, std::vector<oGFX::IndirectCommand>& commands, ObjectInstanceFlags filter)
{
	auto& vr = *VulkanRenderer::get();

	int32_t currModelID{ -1 };
	int32_t cnt{ 0 };
	oGFX::IndirectCommand indirectCmd{};

	for (size_t y = 0; y < entities.size(); y++)
	{
		auto& ent = entities[y];
		auto& subMesh = vr.g_globalSubmesh[ent.submeshID];

		if (ent.submeshID != currModelID) // check if we are using the same model
		{
			currModelID = ent.submeshID;

			AppendBatch(commands, indirectCmd, indirectCmd.instanceCount);

			// append to the batches
			// the number represents the index into the InstanceData array see VulkanRenderer::UploadInstanceData();
			indirectCmd.firstInstance += indirectCmd.instanceCount;

			// reset indirect command				
			indirectCmd.instanceCount = 0;
			indirectCmd.firstIndex = subMesh.baseIndices;
			indirectCmd.indexCount = subMesh.indicesCount;
			indirectCmd.vertexOffset = subMesh.baseVertex;

			auto& s = subMesh.boundingSphere;
			indirectCmd.sphere = glm::vec4(s.center, s.radius);

		}

		// increment based on filter
		//if ((ent.flags&filter) == filter)
		{
			indirectCmd.instanceCount++;
		}
	}

	// append last batch if any
	AppendBatch(commands, indirectCmd, indirectCmd.instanceCount);
}

void GraphicsBatch::Init(GraphicsWorld* gw, VulkanRenderer* renderer, size_t maxObjects)
{
	assert(gw != nullptr);
	assert(renderer != nullptr);

	m_world = gw;
	m_renderer = renderer;

	for (auto& batch : m_batches)
	{
		batch.reserve(maxObjects);
	}

	m_casterData.resize(MAX_LIGHTS);
	for (auto& cd :m_casterData)
	{
		const size_t cubeFaces = 6;
		for (size_t face = 0; face < cubeFaces; face++)
		{
			cd.m_commands[face].clear();
			cd.m_culledObjects[face].clear();
		}
	}

	s_scratchBuffer.reserve(maxObjects);

}

void GraphicsBatch::GenerateBatches()
{
	PROFILE_SCOPED("Generate graphics batch");

	// clear old batches
	for (auto& batch : m_batches)
	{
		batch.clear();
	}

	ProcessGeometry();
	
	ProcessUI();

	ProcessLights();

	ProcessParticleEmitters();

}

void GraphicsBatch::ProcessLights()
{
	PROFILE_SCOPED();
	VulkanRenderer& vr = *VulkanRenderer::get();

	for (auto& light : m_world->m_OmniLightCopy)
	{
		if (LightType::POINT != GetLightType(light)) 
		{
			glm::vec3 dir = glm::normalize(glm::vec3(light.view[0][0]));
			light.view[0] = glm::lookAt(glm::vec3(light.position), glm::vec3(light.position) + dir, glm::vec3{ 0.0f, 1.0f, 0.0f });
			
			// move to view1 for transform later
			light.view[1][0] = glm::vec4(dir, 0);
		}
		else 
		{
			constexpr glm::vec3 up{ 0.0f,1.0f,0.0f };
			constexpr glm::vec3 right{ 1.0f,0.0f,0.0f };
			constexpr glm::vec3 forward{ 0.0f,0.0f,-1.0f };

			// standardize to cubemap faces px, nx, py, ny, pz, nz
			light.view[0] = glm::lookAt(glm::vec3(light.position), glm::vec3(light.position) + right, glm::vec3{ 0.0f, 1.0f, 0.0f });
			light.view[1] = glm::lookAt(glm::vec3(light.position), glm::vec3(light.position) + -right, glm::vec3{ 0.0f, 1.0f, 0.0f });
			light.view[2] = glm::lookAt(glm::vec3(light.position), glm::vec3(light.position) + up, glm::vec3{ 0.0f, 0.0f, 1.0f });
			light.view[3] = glm::lookAt(glm::vec3(light.position), glm::vec3(light.position) + -up, glm::vec3{ 0.0f, 0.0f,-1.0f });
			light.view[4] = glm::lookAt(glm::vec3(light.position), glm::vec3(light.position) + -forward, glm::vec3{ 0.0f,-1.0f, 0.0f });
			light.view[5] = glm::lookAt(glm::vec3(light.position), glm::vec3(light.position) + forward, glm::vec3{ 0.0f,-1.0f, 0.0f });
		}

		auto inversed_perspectiveRH_ZO = [](float fovRad, float aspect, float n, float f)->glm::mat4 {
			glm::mat4 result(0.0f);
			assert(abs(aspect - std::numeric_limits<float>::epsilon()) > 0.0f);
			float const tanHalfFovy = tan(fovRad / 2.0f);

			float h = 1.0f / std::tan(fovRad * 0.5f);
			float w = h / aspect;
			float a = -n / (f - n);
			float b = (n * f) / (f - n);
			result[0][0] = w;
			result[1][1] = h;
			result[2][2] = a;
			result[2][3] = -1.0f;
			result[3][2] = b;

			return result;
			};
		light.projection = inversed_perspectiveRH_ZO(glm::radians(90.0f), 1.0f, 0.1f, light.radius.x);
	}

		
	m_numShadowCastGrids = 0;

	m_culledLights.clear();
	auto& lights = m_world->m_OmniLightCopy;
	m_culledLights.reserve(lights.size());
	//oGFX::DebugDraw::AddArrow(currWorld->cameras[0].m_position, currWorld->cameras[0].m_position + currWorld->cameras[0].GetUp(),oGFX::Colors::GREEN);
	//oGFX::DebugDraw::AddArrow(currWorld->cameras[0].m_position, currWorld->cameras[0].m_position + currWorld->cameras[0].GetRight(),oGFX::Colors::RED);
	//oGFX::DebugDraw::AddArrow(currWorld->cameras[0].m_position, currWorld->cameras[0].m_position + currWorld->cameras[0].GetFront(),oGFX::Colors::BLUE);
	oGFX::Frustum frust = m_world->cameras[0].GetFrustum();
	//{
	//	oGFX::DebugDraw::DrawCameraFrustrumDebugArrows(frust);
	//}

	int viewIter{};
	int sss{};
	for (auto& e : lights)
	{
		oGFX::Sphere s;
		s.center = e.position;
		s.radius = e.radius.x;

		auto renderLight = GetLightEnabled(e);
		if (oGFX::coll::SphereInFrustum(frust, s))		
		{ 			
			//SetLightEnabled(e, existing && true);
			renderLight = renderLight && true;
		}
		else
		{
			sss++;
			//SetLightEnabled(e, false);
			renderLight = false;
		}

		if (renderLight == false)
		{
			continue;
		}
		LocalLightInstance si;		
		SetLightEnabled(si, true);
		si.info = e.info;
		si.position = e.position;
		si.color = e.color;
		si.radius = e.radius;
		si.projection = e.projection;
		if (GetLightType(si) == LightType::POINT) 
		{
			//		// loop through all faces
			for (size_t i = 0; i < 6; i++)
			{
				// setup views
				si.view[i] = e.view[i];
			}
		}
		else // area light
		{
			si.view[0] = e.view[0];

			mat4& rects = si.view[1];
			vec3 dir = e.view[1][0];
			glm::mat4 xform(1.0);
			// Calculate the translation matrix
			glm::mat4 translationMatrix = glm::translate(xform, glm::vec3(si.position));

			// Calculate the rotation matrix
			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f); // Define the up vector for orientation
			glm::vec3 forward = dir;
			glm::vec3 right = glm::normalize(glm::cross(up, forward)); // Calculate the right vector
			glm::vec3 newUp = glm::normalize(glm::cross(forward, right)); // Calculate the corrected up vector
			glm::mat4 rotationMatrix = glm::mat4(glm::vec4(right, 0.0f),
				glm::vec4(newUp, 0.0f),
				glm::vec4(forward, 0.0f),
				glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

			xform = translationMatrix * rotationMatrix;
			//glm::mat4 rot = glm::lookAt(glm::vec3(0.0), glm::vec3(dir),glm::vec3(0,1,0));
			rects[0] = xform * vec4(-0.5f, -0.5f, 0.0f, 1.0f); // Bottom left
			rects[1] = xform * vec4(-0.5f,  0.5f, 0.0f, 1.0f);  // Top left 
			rects[2] = xform * vec4( 0.5f, -0.5f, 0.0f, 1.0f);  // Bottom right 
			rects[3] = xform * vec4( 0.5f,  0.5f, 0.0f, 1.0f);	  // Top right

			float dbgsz = 0.1f;
			oGFX::AABB mybox = oGFX::AABB(glm::vec3(rects[0]) - dbgsz * dir, glm::vec3(rects[3]) + dbgsz * dir);
			oGFX::DebugDraw::AddAABB(mybox, si.color);
		}
	
		m_culledLights.emplace_back(si);
	}

	// process shadows
	int32_t gridIdx = 0;
	//front camera culling
	auto& camera = m_world->cameras[0];
	std::vector<LocalLightInstance*> shadowLights;
	for (size_t i = 0; i < m_culledLights.size(); i++)
	{
		if (GetCastsShadows(m_culledLights[i])) {
			shadowLights.emplace_back(&m_culledLights[i]);
		}
		// disable the data for lighting pass to ignore as a shadow light
		SetCastsShadows(m_culledLights[i], false); 
	}
	// sort lights by distance
	std::sort(shadowLights.begin(), shadowLights.end(), [camPos = camera.m_position](const LocalLightInstance* l, const LocalLightInstance* r) {
		glm::vec3 dirL = glm::vec3(l->position) - camPos;
		float distL = glm::dot(dirL,dirL);
		glm::vec3 dirR = glm::vec3(r->position) - camPos;
		float distR = glm::dot(dirR,dirR);
		
		return distL < distR;
	});

	m_shadowCasters.clear();
	int32_t numLights{};

	std::vector<ObjectInstance*> containedEnt;
	std::vector<ObjectInstance*> intersectEnt;
	for (LocalLightInstance* ePtr : shadowLights)
	{
		LocalLightInstance& e = *ePtr;
		// enable the data for lighting pass to use as a shadow light
		SetCastsShadows(e,true);
		{
			e.info.y = gridIdx;
			if (GetLightType(e) == LightType::POINT) // type one is omnilight
			{
				// loop through all faces
				for (size_t i = 0; i < POINT_LIGHT_FACE_COUNT; i++)
				{
					++m_numShadowCastGrids;
					++gridIdx;
				}
				CastersData& caster = m_casterData[numLights];
				glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, e.radius.x);

				for (size_t face = 0; face < POINT_LIGHT_FACE_COUNT; face++)
				{
					glm::mat4 vp = lightProj * e.view[face];
					oGFX::Frustum f = oGFX::Frustum::CreateFromViewProj(vp);

					bool draw = false;

					containedEnt.clear();
					intersectEnt.clear();
					m_world->m_OctTree->GetEntitiesInFrustum(f, containedEnt, intersectEnt);


					CullDrawData(f, caster.m_culledObjects[face], containedEnt, intersectEnt, draw);
					SortDrawDataByMesh(caster.m_culledObjects[face]);
					GenerateCommands(caster.m_culledObjects[face], caster.m_commands[face]
						, ObjectInstanceFlags::SHADOW_CASTER | ObjectInstanceFlags::RENDER_ENABLED);				
				}
				numLights++;
			}
			else // else area light
			{

				++m_numShadowCastGrids;
				++gridIdx;

				CastersData& caster = m_casterData[numLights];
				glm::mat4 lightProj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, e.radius.x);

				// TODO: maybe need 2 sides for 2 sided.. later..
				for (size_t face = 0; face < AREA_LIGHT_FACE_COUNT; face++)
				{
					glm::mat4 vp = lightProj * e.view[face]; // get the only view 
					oGFX::Frustum f = oGFX::Frustum::CreateFromViewProj(vp);

					bool draw = false;

					containedEnt.clear();
					intersectEnt.clear();
					m_world->m_OctTree->GetEntitiesInFrustum(f, containedEnt, intersectEnt);


					CullDrawData(f, caster.m_culledObjects[face], containedEnt, intersectEnt, draw);
					SortDrawDataByMesh(caster.m_culledObjects[face]);
					GenerateCommands(caster.m_culledObjects[face], caster.m_commands[face]
						, ObjectInstanceFlags::SHADOW_CASTER | ObjectInstanceFlags::RENDER_ENABLED);				
				}
				numLights++;
			}						
		}
		m_shadowCasters.emplace_back(e);
		if (numLights >= MAX_LIGHTS) break;
	}


}

void GraphicsBatch::ProcessGeometry()
{
	using Batch = GraphicsBatch::DrawBatch;
	using Flags = ObjectInstanceFlags;
	
	oGFX::Frustum f;
	std::vector<ObjectInstance*> containedEnt;
	std::vector<ObjectInstance*> intersectEnt;

	f = m_world->cameras[0].GetFrustum();
	containedEnt.clear();
	intersectEnt.clear();
	//m_world->m_OctTree->GetAllEntities(containedEnt);
	m_world->m_OctTree->GetEntitiesInFrustum(f, containedEnt, intersectEnt);
	CullDrawData(f, m_culledCameraObjects, containedEnt, intersectEnt);
	SortDrawDataByMesh(m_culledCameraObjects);
	GenerateCommands(m_culledCameraObjects, m_batches[Batch::ALL_OBJECTS], Flags::RENDER_ENABLED);

	//printf("Total Entities[%3llu/%3llu] Con[%3llu] Int[%3llu]\n", m_culledCameraObjects.size(), m_world->m_OctTree->size(), containedEnt.size(), intersectEnt.size());
}

void GraphicsBatch::ProcessUI()
{
	using Flags = UIInstanceFlags;
	auto& allUI = m_world->m_UIcopy;

	PROFILE_SCOPED();
	m_uiVertices.clear();
	
	std::vector<UIInstance*> instances;
	instances.reserve(128);

	std::queue<Task> tasks;

	for (auto& ui: allUI)
	{
		if (static_cast<bool>(ui.flags & Flags::RENDER_ENABLED) == false)
		{
			continue;
		}

		if (static_cast<bool>(ui.flags & Flags::SCREEN_SPACE) == true)
		{
			// skip non depth
			continue;
		}

		if (static_cast<bool>(ui.flags & Flags::TEXT_INSTANCE))
		{
			instances.emplace_back(&ui);
			auto lam = [this, &ui = ui](void*) {GenerateTextGeometry(ui); };
			lam(nullptr);
			// tasks.emplace(lam);
			
		}
		else
		{
			GenerateSpriteGeometry(ui);
		}
	}

	size_t i = 0;
	for (; i < instances.size()/4; i++)
	{
		 auto lam = [this, ui = instances.data()+i*4](void*) {
			 for (size_t x = 0; x < 4; x++)
			 {
				 UIInstance* uiPtr= *(ui + x);
				 GenerateTextGeometry(*uiPtr); 
			 }			 
		 };
		 tasks.emplace(lam);
	}
	i *= 4;
	auto lam = [this, ui = instances.data()+i, i = i, instancesSz = instances.size()](void*) {
		for (size_t x = i; x < instancesSz; x++)
		{
			GenerateTextGeometry(**(ui+x)); 
		}		 
	};
	//tasks.emplace(lam);

	//VulkanRenderer::get()->g_taskManager.AddTaskListAndWait(tasks);

	// dirty way of doing screenspace
	m_SSVertOffset = m_uiVertices.size();
	for (auto& ui: allUI)
	{
		if (static_cast<bool>(ui.flags & Flags::RENDER_ENABLED) == false)
		{
			continue;
		}

		if (static_cast<bool>(ui.flags & Flags::SCREEN_SPACE) == false)
		{
			// skip depth
			continue;
		}

		if (static_cast<bool>(ui.flags & Flags::TEXT_INSTANCE))
		{
			GenerateTextGeometry(ui);
		}
		else
		{
			GenerateSpriteGeometry(ui);
		}
	}

}

void GraphicsBatch::ProcessParticleEmitters()
{
	auto& allEmitters = m_world->m_EmitterCopy;
	m_particleList.clear();
	m_particleCommands.clear();
	/// Create parciles batch
	uint32_t emitterCnt = 0;
	auto* vr = VulkanRenderer::get();
	for (auto& emitter : allEmitters)
	{
		// note to support multiple textures permesh we have to do this per submesh 
		//setup instance data	
		// TODO: this is really bad fix this
		// This is per entity. Should be per material.
		uint32_t albedo = emitter.bindlessGlobalTextureIndex_Albedo;
		uint32_t normal = emitter.bindlessGlobalTextureIndex_Normal;
		uint32_t roughness = emitter.bindlessGlobalTextureIndex_Roughness;
		uint32_t metallic = emitter.bindlessGlobalTextureIndex_Metallic;
		constexpr uint32_t invalidIndex = 0xFFFFFFFF;
		if (albedo == invalidIndex)
			albedo = vr->whiteTextureID;
		if (normal == invalidIndex)
			normal = vr->blackTextureID;
		if (roughness == invalidIndex)
			roughness = vr->whiteTextureID;
		if (metallic == invalidIndex)
			metallic = vr->blackTextureID;

		// Important: Make sure this index packing matches the unpacking in the shader
		const uint32_t albedo_normal = albedo << 16 | (normal & 0xFFFF);
		const uint32_t roughness_metallic = roughness << 16 | (metallic & 0xFFFF);
		for (auto& pd : emitter.particles)
		{
			pd.instanceData.z = albedo_normal;
			pd.instanceData.w = roughness_metallic;
		}

		// copy list
		m_particleList.insert(m_particleList.end(), emitter.particles.begin(), emitter.particles.end());

		auto& model = m_renderer->g_globalModels[emitter.modelID];
		// set up the commands and number of particles
		oGFX::IndirectCommand cmd{};

		cmd.instanceCount = static_cast<uint32_t>(emitter.particles.size());
		// this is the number invoked by the graphics pipeline as the instance id (location = 15) etc..
		// the number represents the index into the InstanceData array see VulkanRenderer::UploadInstanceData();
		cmd.firstInstance = emitterCnt;
		for (size_t i = 0; i < emitter.submesh.size(); i++)
		{
			// create a draw call for each submesh using the same instance data
			if (emitter.submesh[i] == true)
			{
				const auto& subMesh = m_renderer->g_globalSubmesh[model.m_subMeshes[i]];
				cmd.firstIndex = subMesh.baseIndices;
				cmd.indexCount = subMesh.indicesCount;
				cmd.vertexOffset = subMesh.baseVertex;
				m_particleCommands.push_back(cmd);
			}
		}
		//increment instance data
		emitterCnt += cmd.instanceCount;

		// clear this so next draw we dont care
		emitter.particles.clear();
	}
}

const std::vector<oGFX::IndirectCommand>& GraphicsBatch::GetBatch(int32_t batchIdx)
{
	assert(batchIdx > -1 && batchIdx < GraphicsBatch::MAX_NUM);

	return m_batches[batchIdx];
}

const std::vector<oGFX::IndirectCommand>& GraphicsBatch::GetParticlesBatch()
{
	return m_particleCommands;
}

const std::vector<ParticleData>& GraphicsBatch::GetParticlesData()
{
	return m_particleList;
}

const std::vector<oGFX::UIVertex>& GraphicsBatch::GetUIVertices()
{
	return m_uiVertices;
}

const std::vector<LocalLightInstance>& GraphicsBatch::GetLocalLights()
{
	return m_culledLights;
}

const std::vector<LocalLightInstance>& GraphicsBatch::GetShadowCasters()
{
	return m_shadowCasters;
}

size_t GraphicsBatch::GetScreenSpaceUIOffset() const
{
	return m_SSVertOffset;
}

void GraphicsBatch::GenerateSpriteGeometry(const UIInstance& ui) 
{

	const auto& mdl_xform = ui.localToWorld;

	// hardcode for now
	constexpr size_t quadVertexCount = 4;
	oGFX::AABB2D texCoords{ glm::vec2{0.0f}, glm::vec2{1.0f} };
	glm::vec2 textureCoords[quadVertexCount] = {
		{ texCoords.min.x, texCoords.max.y },
		{ texCoords.min.x, texCoords.min.y },
		{ texCoords.max.x, texCoords.min.y },
		{ texCoords.max.x, texCoords.max.y },
	};

	
	constexpr glm::vec4 verts[quadVertexCount] = {
		{ 0.5f,-0.5f, 0.0f, 1.0f},
		{ 0.5f, 0.5f, 0.0f, 1.0f},
		{-0.5f, 0.5f, 0.0f, 1.0f},
		{-0.5f,-0.5f, 0.0f, 1.0f},
	};

	auto invalidIndex = 0xFFFFFFFF;
	auto& vr = *VulkanRenderer::get();
	auto albedo = ui.bindlessGlobalTextureIndex_Albedo;
	if (albedo == invalidIndex || vr.g_Textures[albedo].isValid == false)
		albedo = vr.whiteTextureID; // TODO: Dont hardcode this bindless texture index

	for (size_t i = 0; i < quadVertexCount; i++)
	{
		oGFX::UIVertex vert;
		vert.pos = mdl_xform * verts[i];
		vert.pos.w = 1.0; // positive is sprite
		vert.col = ui.colour;
		vert.tex = glm::vec4(textureCoords[i], albedo, ui.entityID);
		m_uiVertices.push_back(vert);
	}

}


void GraphicsBatch::GenerateTextGeometry(const UIInstance& ui)
{
	PROFILE_SCOPED();
	using FontFormatting = oGFX::FontFormatting;
	using FontAlignment = oGFX::FontAlignment;


	auto* fontAtlas = ui.fontAsset;
	if (!fontAtlas)
	{
		fontAtlas = VulkanRenderer::get()->GetDefaultFont();
	}

	const auto& text = ui.textData;
	const auto& mdl_xform = ui.localToWorld;

	//float fontScale = format.fontSize / fontAtlas->m_pixelSize;
	float fontScale = ui.format.fontSize;


	float boxPixelSizeX = fabsf(ui.format.box.max.x - ui.format.box.min.x);
	float halfBoxX = boxPixelSizeX / 2.0f;
	float boxPixelSizeY = fabsf(ui.format.box.max.y - ui.format.box.min.y);
	float halfBoxY = boxPixelSizeY / 2.0f;

	//static std::wstring_convert<std::codecvt_utf8_utf16<std::wstring::value_type>> converter;
	//std::wstring unicode = converter.from_bytes(text);

	//std::stringstream ss(unicode);
	std::stringstream ss(text);
	std::vector<std::string> tokens;
	std::string temp;
	// Firstly lets tokenize the entire string
	while (std::getline(ss, temp, ' '))
	{
		if (temp.empty())
		{
			// if we have 2 spaces in a row, the user meant to concatenate spaces
			tokens.emplace_back(std::string{ ' ' });
		}
		else
		{
			size_t offset = 0;
			std::stringstream line(temp);
			std::string cleaned;
			// now we have to clean any user entered new line tokens
			while (std::getline(line, cleaned, '\n'))
			{
				if (line.eof() == true)
				{
					// if we have no more text, we can assume that the entire string is completed
					tokens.emplace_back(std::move(cleaned));
					tokens.emplace_back(std::string{ ' ' });
				}
				else
				{
					// add the cleaned text and push in a new line character that the user entered
					tokens.emplace_back(std::move(cleaned));
					tokens.emplace_back(std::string{ '\n' });
				}
			}
		}
	}

	if (tokens.size() && tokens.back().front() == ' ')
	{
		tokens.pop_back();
	}

	int numLines = 1;
	std::vector<float> xStartingOffsets;

	float sizeTaken = 0.0f;
	for (auto token = tokens.begin(); token != tokens.end(); ++token)
	{
		if (token->compare("\n") == 0)
		{
			// if we have a manually entered newline token after cleaning,
			// it means user wants a new line, calculate one with current values and reset

			// handle having spaces at the end of a sentence from the previous iterator			
			if ((&tokens.front() - 1) < (&*token - 1) && std::prev(token)->compare(" ") == 0)
			{
				const auto& gly = fontAtlas->m_characterInfos[L' '];
				float value = (gly.Advance.x) * fontScale;
				sizeTaken -= value;
			}

			++numLines;
			if (ui.format.alignment & (FontAlignment::Centre | FontAlignment::Top_Centre | FontAlignment::Bottom_Centre))
			{
				xStartingOffsets.push_back(sizeTaken / 2.0f);
			}
			else
			{
				xStartingOffsets.push_back(halfBoxX - sizeTaken);
			}
			sizeTaken = 0.0f;
			continue; // go next
		}

		// grab the with of the token
		float textSize = std::accumulate(token->begin(), token->end(), 0.0f, [&](float x, const std::wstring::value_type c)->float
			{
				const auto& gly = fontAtlas->m_characterInfos[c];
				float value = (gly.Advance.x) * fontScale;
				return x + value;
			}
		);

		// now process the token
		if (textSize > boxPixelSizeX)
		{
			//text is much bigger than box, no choice we will just fit it accordingly
			if (sizeTaken == 0.0f)
			{
				// we have a fresh line, just start a new line here
				if (ui.format.alignment & (FontAlignment::Centre | FontAlignment::Top_Centre | FontAlignment::Bottom_Centre))
				{
					xStartingOffsets.push_back(textSize / 2.0f);
				}
				else
				{
					xStartingOffsets.push_back(halfBoxX - textSize);
				}

				if (tokens.size() != 1)
				{
					token->push_back('\n');
					++numLines;
				}
			}
			else
			{
				// we have a line in progress, we must :
				// 1 : clean up the old one and 
				// 2 : start a fresh new line
				if (ui.format.alignment & (FontAlignment::Centre | FontAlignment::Top_Centre | FontAlignment::Bottom_Centre))
				{
					xStartingOffsets.push_back(sizeTaken / 2.0f);
					xStartingOffsets.push_back(textSize / 2.0f);
				}
				else
				{
					xStartingOffsets.push_back(halfBoxX - sizeTaken);
					xStartingOffsets.push_back(halfBoxX - textSize);
				}
				*token = '\n' + *token + '\n';
				numLines += 2;
				sizeTaken = 0.0f;
			}
		}
		else
		{
			// Line is still in progress
			if (textSize + sizeTaken > boxPixelSizeX)
			{
				// We are expected to overflow, so we need to start a new line and continue from there
				if (ui.format.alignment & (FontAlignment::Centre | FontAlignment::Top_Centre | FontAlignment::Bottom_Centre))
				{
					xStartingOffsets.push_back(sizeTaken / 2.0f);
				}
				else
				{
					xStartingOffsets.push_back(halfBoxX - sizeTaken);
				}

				*token = '\n' + *token;
				++numLines;
				// we store the length of the current string as the next starting point
				sizeTaken = textSize;
			}
			else
			{
				// just keep going ...
				sizeTaken += textSize;
			}
		}
	}


	// we set the remaining starting offset
	xStartingOffsets.push_back(sizeTaken);
	if (ui.format.alignment & (FontAlignment::Centre_Right| FontAlignment::Top_Right| FontAlignment::Bottom_Right))
	{
		xStartingOffsets.back() = halfBoxX - xStartingOffsets.back();
	}
	else
	{
		xStartingOffsets.back() /= 2.0f;
	}

	// process starting offsets to get to the right cursor positions
	for (auto& x : xStartingOffsets)
	{
		// old code
		//auto position = ui.position;
		if (ui.format.alignment & (FontAlignment::Centre | FontAlignment::Top_Centre | FontAlignment::Bottom_Centre))
		{
			x = x;
		}
		else
		{
			x = -x;
		}
	}

	float startY{};
	float startX{};
	int xStartIndex = 0;

	// Select formatting along X axis
	if (ui.format.alignment & (FontAlignment::Bottom_Left | FontAlignment::Centre_Left| FontAlignment::Top_Left))
	{
		startX = halfBoxX;
	}
	else
	{
		startX = xStartingOffsets[xStartIndex];
	}

	// Select formatting along Y axis
	if (ui.format.alignment & (FontAlignment::Top_Centre | FontAlignment::Top_Left | FontAlignment::Top_Right))
	{
		// downwards growth is handled for us...
		startY = /*ui.position.y*/ + halfBoxY - fontAtlas->m_characterInfos['L'].Size.y * fontScale;
	}
	else if (ui.format.alignment & (FontAlignment::Bottom_Centre | FontAlignment::Bottom_Left | FontAlignment::Bottom_Right))
	{
		// whereas.. needs to take into account vertical line space to handle upwards growth
		startY = /*ui.position.y*/ - halfBoxY + (std::max(0, numLines - 1) * fontAtlas->m_characterInfos['L'].Size.y * fontScale * ui.format.verticalLineSpace);
	}
	else
	{
		// centre alignment takes into account everything
		const float fullFontSize = fontAtlas->m_characterInfos['L'].Size.y * fontScale;
		const float halfFontSize = fontAtlas->m_characterInfos['L'].Size.y * fontScale / 2.0f;
		const float halfLines = std::max(0.0f,float(numLines-1) / 2);
		startY = /*ui.position.y*/ -halfFontSize + halfLines * fullFontSize * ui.format.verticalLineSpace;
	}

	std::vector<oGFX::UIVertex> vertexBuffer;
	
	// arbitrary size
	vertexBuffer.reserve(tokens.size() * 4 * 10);
	glm::vec2 cursorPos{ startX, startY };
	for (const auto& token : tokens)
	{
		// go through all our strings and fill the font buffer
		for (const auto& c : token)
		{
			//get our glyph of this char
			const oGFX::Font::Glyph& glyph = fontAtlas->m_characterInfos[c];

			if (c == '\n')
			{
				if (ui.format.alignment & (FontAlignment::Centre_Left| FontAlignment::Top_Left| FontAlignment::Bottom_Left))
				{
					// provide left alignment which is default
					cursorPos.x = startX;
				}
				else
				{
					// provide custom alignment
					cursorPos.x = xStartingOffsets[++xStartIndex];
				}

				// start new line
				cursorPos.y -= glyph.Size.y * /*ui.scale.y * */ ui.format.verticalLineSpace * fontScale;
				continue;
			}


			// calculating glyph positions..
			float xpos = cursorPos.x - glyph.Bearing.x * /*ui.scale.x * */ fontScale;
			float ypos = cursorPos.y - (glyph.Size.y - glyph.Bearing.y) * /*ui.scale.y * */ fontScale;
			ypos = cursorPos.y + (glyph.Bearing.y) * /*ui.scale.y * */ fontScale;

			float w = glyph.Size.x * /*ui.scale.x * */ fontScale;
			float h = glyph.Size.y * /*ui.scale.y * */ fontScale;

			//note position plus scale is already done here
			std::array<glm::vec4, 4> verts = {
				glm::vec4{xpos,    ypos,     0.0f, 1.0f},
				glm::vec4{xpos,    ypos + h, 0.0f, 1.0f},
				glm::vec4{xpos- w ,ypos + h, 0.0f, 1.0f},
				glm::vec4{xpos- w ,ypos,     0.0f, 1.0f},
			};
			cursorPos.x -= (glyph.Advance.x) * /*ui.scale.x * */ fontScale;  // bitshift by 6 to get value in pixels (2^6 = 64)

																		 // Reference : constexpr glm::vec2 textureCoords[] = { { 0.0f, 1.0f }, { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f } };
			std::array<glm::vec2, 4> textureCoords = {
				glm::vec2{ glyph.textureCoordinates.x, glyph.textureCoordinates.y},
				glm::vec2{ glyph.textureCoordinates.x, glyph.textureCoordinates.w},
				glm::vec2{ glyph.textureCoordinates.z, glyph.textureCoordinates.w},
				glm::vec2{ glyph.textureCoordinates.z, glyph.textureCoordinates.y},
			};

			constexpr size_t quadVertexCount = 4;
			for (size_t i = 0; i < quadVertexCount; i++)
			{
				oGFX::UIVertex vert;
				vert.pos = mdl_xform * verts[i];
				vert.pos.w = -1.0; // neagtive is font
				vert.col = ui.colour;
				vert.tex = glm::vec4(textureCoords[i], fontAtlas->m_atlasID, ui.entityID);
				vertexBuffer.push_back(vert);
			}

		}
	}
	
	// transfer to global buffer
	std::scoped_lock lock{ m_uiVertMutex };
	auto sz = m_uiVertices.size();
	m_uiVertices.resize(m_uiVertices.size() + vertexBuffer.size());
	for (size_t i = 0; i < vertexBuffer.size(); i++)
	{
		m_uiVertices[sz + i] = vertexBuffer[i];
	}
}
