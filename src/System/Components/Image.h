#pragma once
#include <System/Component.h>
USING_PTR(UIObject);
class ComponentImage : public Component
{
public:
    USING_SUPER(ComponentImage);
    ComponentImage() = default;
    ComponentImage(std::string_view file_path_);
    void Init() override;
    void Update() override;
    void PreDraw() override;
    void Draw() override;
    void LateDraw() override;
    void PostDraw() override;

    void        DrawMain();
    inline void SetImage(std::string_view file_path_)
    {
        image     = LoadGraph(file_path_.data());
        file_path = file_path_;
    }
    inline void             SetImage(int handle) { image = handle; }
    float2                  GetImageSize();
    inline std::string_view FilePath() { return file_path; }
    int                     alpha = 255;

    void Exit() override;
    enum DRAW_TYPE
    {
        DEFAULT,
        CLAMP,
        EXTEND,
    };

    inline DRAW_TYPE& DrawType() { return draw_type; }

private:
    UIObjectPtr ui_owner   = nullptr;
    DRAW_TYPE   draw_type  = DEFAULT;
    std::string file_path  = "";
    int         image      = -1;
    int         null_image = -1;
};
