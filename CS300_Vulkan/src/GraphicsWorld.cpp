#include "GraphicsWorld.h"

int32_t GraphicsWorld::CreateObjectInstance()
{
	return m_ObjectInstances.Add(ObjectInstance());
}

ObjectInstance& GraphicsWorld::GetObjectInstance(int32_t id)
{
	return m_ObjectInstances.Get(id);
}
