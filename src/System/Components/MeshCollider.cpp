#include "precompile.h"
#include "System/Components/RigidBody.h"
#include "MeshCollider.h"
#include "System/Components/ModelRenderer.h"


using namespace physx;

int MeshCollider::Init()
{
	rigidbody = owner->GetComponent<RigidBody>();
	if (!rigidbody) {
		RemoveThisComponent();
		return -1;
	}
	rigidbody->ChangeToStatic();
	model = owner->GetComponent<ModelRenderer>();
	if (model && rigidbody)
		AttachToModel();
	return 0;
}

void MeshCollider::PrePhysics()
{
	if (!shape)
		return;

	PxTransform trns = MakeCollisionTransform();
	mesh.scale = owner->transform->scale;
	shape->setGeometry(mesh);
	shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !is_trigger);
	shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, is_trigger);
	shape->setLocalPose(trns);
}

void MeshCollider::DebugDraw()
{
	if (!ref_poly_)
		return;

	auto trns = rigidbody->GetBody()->getGlobalPose();
	for (int i = 0; i < ref_poly_->PolygonNum; i++) {
		Vector3 p0 = cast(ref_poly_->Vertexs[ref_poly_->Polygons[i].VIndex[0]].Position) * owner->transform->scale.x;
		Vector3 p1 = cast(ref_poly_->Vertexs[ref_poly_->Polygons[i].VIndex[1]].Position) * owner->transform->scale.x;
		Vector3 p2 = cast(ref_poly_->Vertexs[ref_poly_->Polygons[i].VIndex[2]].Position) * owner->transform->scale.x;
		p0 = trns.q.rotate(p0);
		p1 = trns.q.rotate(p1);
		p2 = trns.q.rotate(p2);
		p0 += trns.p;
		p1 += trns.p;
		p2 += trns.p;
		// ポリゴンを形成する三頂点を使用してワイヤーフレームを描画する
		DrawLine3D(cast(p0), cast(p1), Color::RED);

		DrawLine3D(cast(p1), cast(p2), Color::RED);

		DrawLine3D(cast(p2), cast(p0), Color::RED);
	}
}

void MeshCollider::Update()
{
	if (model && !attached)
		AttachToModel();
}

void MeshCollider::AttachToModel()
{
	if (!model)
		model = owner->GetComponent<ModelRenderer>();
	if (attached || !model || !rigidbody)
		return;
	if (!model->IsLoaded())
		return;



	try {
		auto triangle_mesh = model->model->GetTriangleMesh();
		ref_poly_ = model->model->GetPolygon();
		mesh.triangleMesh = triangle_mesh;
		shape = PhysicsManager::GetPhysicsInstance()->createShape(mesh, *Material::Default,true);
#ifndef PACKAGE_BUILD
		if (!shape)
			throw(Exception("トライアングルメッシュ作成に失敗しました。メッシュデータが無効です。モデルが有効なものか再確認してください", DEFAULT_EXCEPTION_PARAM));
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

void MeshCollider::Exit()
{
	Super::Exit();
}
