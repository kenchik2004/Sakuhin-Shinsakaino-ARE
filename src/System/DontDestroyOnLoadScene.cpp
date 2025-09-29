#include "precompile.h"
#include "DontDestroyOnLoadScene.h"


void DontDestroyOnLoadScene::DontDestroyOnLoad(ObjectP object, SceneP from_where)
{
	from_where->MoveGameObjectPtrFromThis(object, shared_from_this());
}
