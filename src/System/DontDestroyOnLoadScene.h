#pragma once
#include "Scene.h"
class DontDestroyOnLoadScene :
	public Scene
{
public:
	void DontDestroyOnLoad(ObjectP object, SceneP from_where);
};

