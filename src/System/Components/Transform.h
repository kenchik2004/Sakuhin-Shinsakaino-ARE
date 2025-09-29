#pragma once
#include "System/Component.h"
USING_PTR(Transform);
class Transform :
	public Component
{
private:
	friend class Object;
	TransformWPVec children; //!< 子TransformへのWeakPointerのリスト
	Vector3 position_prev = Vector3(0, 0, 0);; //!< 前回の位置
	Quaternion rotation_prev = Quaternion(physx::PxIdentity);; //!< 前回の回転
	Vector3 scale_prev = Vector3(1, 1, 1); //!< 前回のスケール

public:
	USING_SUPER(Transform);

	Vector3 position = Vector3(0, 0, 0);
	Vector3 local_position = Vector3(0, 0, 0); //!< 親Transformからの相対位置

	Quaternion rotation = Quaternion(physx::PxIdentity);
	Quaternion local_rotation = Quaternion(physx::PxIdentity); //!< 親Transformからの相対回転
	Vector3 scale = Vector3(1, 1, 1);
	Vector3 local_scale = Vector3(1, 1, 1); //!< 親Transformからの相対スケール
	TransformWP parent; //!< 親TransformへのWeakPointer
	void Construct() override;
	void PreDraw() override;
	void DebugDraw() override;
	TransformWP GetChild(size_t index) const;
	void SetChild(TransformP new_child);
	void SetParent(TransformP new_parent);

	void ResetChild(TransformWP reset_child);
	void ResetParent();


	inline void MovePosition(const Vector3& dir) { position += dir * Time::DeltaTime(); }
	inline void SetPosition(const Vector3& pos) { position = pos; }
	void AddRotation(Vector3 euler_angles);
	void AddRotation(Quaternion q);
	void SetRotation(Vector3 euler_angles);
	void SetRotation(Quaternion q);
	Vector3 AxisX();
	Vector3 AxisY();
	Vector3 AxisZ();
	void SetAxisX(Vector3 target);
	void SetAxisY(Vector3 target);
	void SetAxisZ(Vector3 target);
};

