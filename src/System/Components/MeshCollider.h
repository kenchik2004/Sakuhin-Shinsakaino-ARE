#pragma once
#include "System/Components/Collider.h"


USING_PTR(ModelRenderer);
USING_PTR(MeshCollider);
class MeshCollider :
	public Collider
{
public:
	USING_SUPER(MeshCollider);
	int Init() override;
	void PrePhysics() override;
	void DebugDraw() override;
	void Update() override;
	void AttachToModel();
	void Exit() override;

private:
	ModelRendererWP model;
	physx::PxTriangleMeshGeometry mesh{};
	bool attached = false;
	MV1_REF_POLYGONLIST* ref_poly_ = nullptr;       //!< ポリゴンデータ
};

