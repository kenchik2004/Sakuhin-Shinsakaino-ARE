#include "Precompile.h"
#include <Game/Game01/Component/UI/ComponentImage.h>
#include <Game/Game01/Object/UI/UIObject.h>

ComponentImage::ComponentImage(std::string_view file_path_)
    : Component()
{
    file_path = file_path_;
}

void ComponentImage::Construct(ObjectPtr owner, std::string_view file_path_)
{
    owner_    = owner;
    file_path = file_path_;
}
void ComponentImage::Construct(ObjectPtr owner)
{
    owner_   = owner;
    ui_owner = std::static_pointer_cast<UIObject>(owner);
}
void ComponentImage::Init()
{
    if(owner_->obj_type != Object::UI_OBJ)
        RemoveThisComponent();
    if(image < 0)
        image = LoadGraph(file_path.c_str());
    null_image = LoadGraph("data/Game/Image/nullptr.png");
    Super::Init();
}

void ComponentImage::Update()
{
}

void ComponentImage::PreDraw()
{
    if(ui_owner->GetDrawTiming() != UIObject::PRE_DRAW)
        return;
    DrawMain();
}

void ComponentImage::Draw()
{
    if(ui_owner->GetDrawTiming() != UIObject::DRAW)
        return;
    DrawMain();
}

void ComponentImage::LateDraw()
{
    if(ui_owner->GetDrawTiming() != UIObject::LATE_DRAW)
        return;
    DrawMain();
}

void ComponentImage::PostDraw()
{
    if(ui_owner->GetDrawTiming() != UIObject::POST_DRAW)
        return;
    DrawMain();
}

void ComponentImage::DrawMain()
{
    if(!enable)
        return;
    auto   owner    = std::static_pointer_cast<UIObject>(owner_);
    float2 draw_pos = owner->GetDrawPos().xy;
    float2 scale    = owner->GetScaleAxisXYZ().xy;

    if(image <= 0) {
        draw_pos += scale * 0.5f;
        DrawRotaGraphF(draw_pos.x, draw_pos.y, 1.0f, 0.0f, null_image, true);
        return;
    }
    alpha = std::clamp<int>(alpha, 0, 255);
    SetDrawBlendMode(DX_BLENDMODE_ALPHA, alpha);
    switch(draw_type) {
    case ComponentImage::DEFAULT:

        draw_pos += scale * 0.5f;
        DrawRotaGraphF(draw_pos.x, draw_pos.y, 1.0f, 0.0f, image, true);

        break;
    case ComponentImage::CLAMP:
        DrawRectGraphF(draw_pos.x, draw_pos.y, 0, 0, scale.x, scale.y, image, true);
        break;
    case ComponentImage::EXTEND:
        DrawExtendGraphF(draw_pos.x, draw_pos.y, draw_pos.x + scale.x, draw_pos.y + scale.y, image, true);
        break;
    default:
        break;
    }
    SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

float2 ComponentImage::GetImageSize()
{
    float2 ret;
    GetGraphSizeF(image, (float*)&ret.x, (float*)&ret.y);
    return ret;
}

void ComponentImage::Exit()
{
    DeleteGraph(null_image);
    if(!file_path.empty() && image > 0)
        DeleteGraph(image);
    Super::Exit();
}
