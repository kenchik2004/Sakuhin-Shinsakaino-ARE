#pragma once
#include "Collider.h"

USING_PTR(CapsuleCollider);
class CapsuleCollider :
    public Collider
{
public:
    USING_SUPER(CapsuleCollider);
    int Init()override;
    void PrePhysics()override;
    void DebugDraw() override;
    float height = 1.0f;
    float radius = 0.5f;
};

