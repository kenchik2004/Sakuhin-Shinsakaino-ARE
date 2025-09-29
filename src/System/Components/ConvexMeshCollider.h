
#pragma once
#include "System/Components/Collider.h"


USING_PTR(ModelRenderer);
USING_PTR(ConvexMeshCollider);
class ConvexMeshCollider :
	public Collider
{

public:
	USING_SUPER(ConvexMeshCollider);
	int Init() override;
	void PrePhysics() override;
	void DebugDraw() override;
	void Update() override;
	void AttachToModel();
	void Exit() override;

private:
	ModelRendererWP model;
	physx::PxConvexMeshGeometry mesh{};
	bool attached = false;
	MV1_REF_POLYGONLIST* ref_poly_ = nullptr;       //!< ポリゴンデータ
};

