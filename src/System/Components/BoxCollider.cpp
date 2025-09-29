#include "precompile.h"
#include <System/Components/RigidBody.h>
#include "BoxCollider.h"
#include <System/Components/ModelRenderer.h>

using namespace physx;

int BoxCollider::Init()
{
	rigidbody = owner->GetComponent<RigidBody>();
	if (!rigidbody) {
		RemoveThisComponent();
		return -1;
	}
	auto p_scene = owner->GetScene()->GetPhysicsScene();

	shape = PhysicsManager::GetPhysicsInstance()->createShape(
		PxBoxGeometry(extension.x * 0.5f, extension.y * 0.5f, extension.z * 0.5f),
		*Material::Default, true);
	shape->userData = new SafeWeakPtr<Collider>(std::static_pointer_cast<Collider>(shared_from_this()));
	shape->setSimulationFilterData(PxFilterData(hit_group, collision_group, 0, 0));

	rigidbody->GetBody()->attachShape(*shape);
	return 0;
}

void BoxCollider::PrePhysics()
{


	PxTransform trns = MakeCollisionTransform();

	shape->setGeometry(PxBoxGeometry(extension.x * 0.5f, extension.y * 0.5f, extension.z * 0.5f));
	shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, !is_trigger);
	shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, is_trigger);
	shape->setLocalPose(trns);



}

void BoxCollider::DebugDraw()
{
	auto body = rigidbody->GetBody();
	physx::PxTransform trns = body->getGlobalPose();
	physx::PxTransform trns2 = shape->getLocalPose();
	mat4x4 mat(trns * trns2);
	Vector3 points[8] = {
		{extension.x * 0.5f,extension.y * 0.5f,extension.z * 0.5f},
		{extension.x * 0.5f,extension.y * 0.5f,extension.z * -0.5f},
		{extension.x * 0.5f,extension.y * -0.5f,extension.z * -0.5f},
		{extension.x * 0.5f,extension.y * -0.5f,extension.z * 0.5f},
		{extension.x * -0.5f,extension.y * 0.5f,extension.z * 0.5f},
		{extension.x * -0.5f,extension.y * 0.5f,extension.z * -0.5f},
		{extension.x * -0.5f,extension.y * -0.5f,extension.z * -0.5f},
		{extension.x * -0.5f,extension.y * -0.5f,extension.z * 0.5f},
	};
	for (int i = 0; i < 8; i++) {
		points[i] = mat.getPosition() + mat.rotate(points[i]);
	}
	for (int i = 0; i < 4; i++) {
		DrawLine3D(cast(points[i]), i != 3 ? cast(points[i + 1]) : cast(points[0]), Color::GREEN);
		DrawLine3D(cast(points[i]), cast(points[i + 4]), Color::GREEN);
		if (i % 2 == 0 && i != 0) {
			DrawLine3D(cast(points[i]), cast(points[i - 2]), Color::GREEN);
		}
	}
	for (int i = 4; i < 8; i++) {
		DrawLine3D(cast(points[i]), i != 7 ? cast(points[i + 1]) : cast(points[4]), Color::GREEN);
		if (i % 2 == 0 && i != 4) {
			DrawLine3D(cast(points[i]), cast(points[i - 2]), Color::GREEN);
		}
	}
}
