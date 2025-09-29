#pragma once
#include "Collider.h"

USING_PTR(SphereCollider);
class SphereCollider :
    public Collider
{
public:
    USING_SUPER(SphereCollider);
    int Init() override;
    void PrePhysics() override;
    void DebugDraw() override;

    const float& Radius() { return radius; }
    float radius = 0.5f;
private:


};

