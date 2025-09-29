#include "precompile.h"
#include "System/Components/ConvexMeshCollider.h"
#include "System/Components/RigidBody.h"
#include "System/Components/ModelRenderer.h"
#include "fstream"


using namespace physx;

int ConvexMeshCollider::Init()
{
	rigidbody = owner->GetComponent<RigidBody>();
	if (!rigidbody) {
		RemoveThisComponent();
		return -1;
	}
	model = owner->GetComponent<ModelRenderer>();
	if (model && rigidbody)
		AttachToModel();
	return 0;
}

void ConvexMeshCollider::PrePhysics()
{
	if (!shape)
		return;
	//rigidbody->GetBody()->detachShape(*shape);

	mesh.scale = owner->transform->scale;
	shape->setGeometry(mesh);
	shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !is_trigger);
	shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, is_trigger);

	//rigidbody->GetBody()->attachShape(*shape);
}

void ConvexMeshCollider::DebugDraw()
{
	if (!ref_poly_)
		return;
	auto trns = rigidbody->GetBody()->getGlobalPose();



	for (int i = 0; i < ref_poly_->VertexNum; i++) {
		Vector3 p0 = cast(ref_poly_->Vertexs[i].Position) * owner->transform->scale.x;
		Vector3 p1 = cast(ref_poly_->Vertexs[(i + 1) < ref_poly_->VertexNum ? i + 1 : 0].Position) * owner->transform->scale.x;
		Vector3 p2 = cast(ref_poly_->Vertexs[(i + 2) < ref_poly_->VertexNum ? i + 2 : 1].Position) * owner->transform->scale.x;
		Vector3 p3 = cast(ref_poly_->Vertexs[(i + 3) < ref_poly_->VertexNum ? i + 3 : 2].Position) * owner->transform->scale.x;
		p0 = trns.q.rotate(p0);
		p1 = trns.q.rotate(p1);
		p2 = trns.q.rotate(p2);
		p3 = trns.q.rotate(p3);
		p0 += trns.p;
		p1 += trns.p;
		p2 += trns.p;
		p3 += trns.p;
		// 頂点を使用してワイヤーフレームを描画する
		DrawLine3D(cast(p0), cast(p1), Color::GREEN);
		DrawLine3D(cast(p0), cast(p2), Color::GREEN);
		DrawLine3D(cast(p0), cast(p3), Color::GREEN);
	}
}

void ConvexMeshCollider::Update()
{
	if (model && !attached)
		AttachToModel();
}

void ConvexMeshCollider::AttachToModel()
{
	if (!model)
		model = owner->GetComponent<ModelRenderer>();
	if (attached || !model || !rigidbody)
		return;
	if (!model->IsLoaded())
		return;



	try {
		ref_poly_ = model->model->GetPolygon();
		PxConvexMesh* convex_mesh = model->model->GetConvexMesh();
		mesh.convexMesh = convex_mesh;
		shape = PhysicsManager::GetPhysicsInstance()->createShape(mesh, *Material::Default, true);
#ifndef PACKAGE_BUILD
		if (!shape)
			throw(Exception("凸メッシュ作成に失敗しました。メッシュデータが無効です。三角メッシュを試す、モデルが有効なものか再確認する等を試してみてください", DEFAULT_EXCEPTION_PARAM));
#endif
		shape->userData = new SafeWeakPtr<Collider>(std::static_pointer_cast<Collider>(shared_from_this()));
		rigidbody->GetBody()->attachShape(*shape);
	}
	catch (Exception& ex) {
		ex.Show();
		return;
	}
	attached = true;
}

void ConvexMeshCollider::Exit()
{
	attached = false;
	Super::Exit();
}
