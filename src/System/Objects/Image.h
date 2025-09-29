#pragma once
#include <Game/Game01/Object/UI/UIObject.h>
#include <Game/Game01/Component/UI/ComponentImage.h>

class Image : public UIObject
{
public:
    BP_OBJECT_DECL(Image, u8"画像オブジェクト");
    bool Init() override;
};
