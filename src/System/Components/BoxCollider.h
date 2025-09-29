#pragma once
#include "Collider.h"
USING_PTR(BoxCollider);
class BoxCollider :
	public Collider
{
public:
	USING_SUPER(BoxCollider);
	int Init() override;
	void PrePhysics() override;
	void DebugDraw() override;
	Vector3 extension = { 1.0f,1.0f,1.0f };
	bool is_trigger = false;
};

