#include "Precompile.h"
#include <Game/Game01/Object/UI/Image.h>
#include <Game/Game01/Component/UI/ComponentImage.h>

bool Image::Init()
{
    SetScaleAxisXYZ(float3(50, 50, 0));
    auto img = AddComponent<ComponentImage>();

    return Super::Super::Init();
}
