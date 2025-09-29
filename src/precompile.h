#pragma once

#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <iostream>
#include <time.h>
#include <math.h>
#include <memory>
#include <vector>
#include <array>
#include <functional>
#include <algorithm>
#include "chrono"

#pragma warning (disable : 4819)
#pragma comment(lib, "winmm.lib")

#pragma warning (disable : 26495)
#define _SILENCE_CXX20_CISO646_REMOVED_WARNING
#include "PxPhysicsAPI.h"
#pragma comment(lib, "PhysX_64.lib")
#pragma comment(lib, "PhysXCommon_64.lib")
#pragma comment(lib, "PhysXCooking_64.lib")
#pragma comment(lib, "PhysXExtensions_static_64.lib")
#pragma comment(lib, "PhysXFoundation_64.lib")
#pragma comment(lib, "PhysXPvdSDK_static_64.lib")
#pragma comment(lib, "PhysXTask_static_64.lib")
#pragma comment(lib, "SceneQuery_static_64.lib")
#pragma comment(lib, "SimulationController_static_64.lib")
#pragma warning (default : 26495)


#include <d3d11.h>
//#include <d3dx9.h>

#include "System/Exception.h"
#include "System/typedef.h"
#include <DxLib.h>
//#include "System/ImGui_.h"
#include "float2.h"
#include "float3.h"
#include "System/Input.h"
#include "System/Time.h"
#include"System/Random.h"
#include "System/PhysicsManager.h"
#include "System/TextureManager.h"
#include "System/ModelManager.h"
#include "System/AudioManager.h"
#include "System/Component.h"
#include "System/Components/Transform.h"
#include "System/Object.h"
#include "System/Scene.h"
#include "System/DontDestroyOnLoadScene.h"
#include "System/SceneManager.h"






