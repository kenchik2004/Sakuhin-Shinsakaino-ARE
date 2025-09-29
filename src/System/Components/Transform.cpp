#include "precompile.h"
#include "Transform.h"
#include "algorithm"

void WrapAngles0_360(Vector3* euler) {
	while (euler->x < 0)
		euler->x += 360;
	while (euler->x > 360)
		euler->x -= 360;
	while (euler->y < 0)
		euler->y += 360;
	while (euler->y > 360)
		euler->y -= 360;
	while (euler->z < 0)
		euler->z += 360;
	while (euler->z > 360)
		euler->z -= 360;
}

void Transform::Construct()
{
	status.status_bit.on(CompStat::STATUS::SINGLE);
	SetPriority(0);
}

void Transform::PreDraw()
{
	// 親がいる場合、親によって位置、回転、スケールが変化するので、
	// 親Transformの情報を考慮して子Transformの位置、回転、スケールを計算する
	if (auto p = parent.lock()) {
		// 親がいる場合でも、グローバルの位置情報が更新された場合、
		// ユーザーはグローバルの変更を前提に処理を行うだろう。
		// この場合はローカルの位置情報を更新する必要がある

		Vector3 local_position_ = (p->rotation.rotate(position - p->position));// 親Transformからの相対位置を計算
		Quaternion local_rotation_ = p->rotation.getConjugate() * rotation;	// 親Transformからの相対回転を計算
		Vector3 local_scale_ = scale / p->scale;	// 親Transformからの相対スケールを計算
		local_position = position == position_prev ? local_position : local_position_;
		local_rotation = rotation == rotation_prev ? local_rotation : local_rotation_;
		local_scale = scale == scale_prev || (scale.isZero() && !local_scale.isZero()) ? local_scale : local_scale_;

		// そうでない場合、グローバル位置情報は基本的に親によって決まるので、
		// 親Transformの情報とローカルの位置情報を考慮してグローバルの情報を書き換える
		Vector3 position_ = p->position + p->scale.multiply(p->rotation.rotate(local_position));// 親Transformがあれば、親の位置と回転を考慮して子Transformの位置を計算
		Quaternion rotation_ = p->rotation * local_rotation;	// 親Transformがあれば、親の回転を考慮して子Transformの回転を計算
		Vector3 scale_ = p->scale.multiply(local_scale);	// 親Transformがあれば、親のスケールを考慮して子Transformのスケールを計算
		position = position_;
		rotation = rotation_;
		scale = scale_;

	}
	else {
		// 親Transformがない場合は、グローバルの位置情報はローカルの位置情報と同じなので、
		// 変更された方を、そうでない方に反映する

		local_position = position == position_prev ? local_position : position;
		local_rotation = rotation == rotation_prev ? local_rotation : rotation;
		local_scale = scale == scale_prev ? local_scale : scale;

		position = local_position;	// 親Transformがない場合は、グローバル位置はローカル位置と同じ
		rotation = local_rotation;	// 親Transformがない場合は、グローバル回転はローカル回転と同じ
		scale = local_scale;	// 親Transformがない場合は、グローバルスケールはローカルスケールと同じ

	}
	position_prev = position;	// 前回の位置を更新
	rotation_prev = rotation;	// 前回の回転を更新
	scale_prev = scale;	// 前回のスケールを更新

}

void Transform::DebugDraw()
{
	DrawLine3D(cast(position), cast(position + AxisX()), Color::BLUE);
	DrawCone3D(cast(position + AxisX()), cast(position + AxisX() * 0.7f), 0.05f, 8, Color::BLUE, Color::BLUE, true);
	DrawLine3D(cast(position), cast(position + AxisY()), Color::GREEN);
	DrawCone3D(cast(position + AxisY()), cast(position + AxisY() * 0.7f), 0.05f, 8, Color::GREEN, Color::GREEN, true);
	DrawLine3D(cast(position), cast(position + AxisZ()), Color::RED);
	DrawCone3D(cast(position + AxisZ()), cast(position + AxisZ() * 0.7f), 0.05f, 8, Color::RED, Color::RED, true);

}

TransformWP Transform::GetChild(size_t index) const
{
	if (index >= children.size())
		return nullptr;
	return children[index];
}

void Transform::SetChild(TransformP new_child)
{
	if (!new_child || new_child->owner == owner) return;
	TransformWP p = parent;
	while (p) {
		if (p.lock() == new_child) {
			// 自分自身を親に設定しようとした場合は無視
			return;
		}
		p = p->parent; // 親をたどる
	}
	// 子Transformの親を設定
	new_child->parent = SafeWeakPtr<Transform>(std::static_pointer_cast<Transform>(shared_from_this()));
	children.push_back(new_child);
	new_child->owner->SetPriority(new_child->owner->GetPriority());	// 親の優先度を引き継ぐためにSetPriorityを呼び出す(親がいる場合は勝手に加算されるので、今の優先度をそのまま与える)
	Vector3 scale_ = new_child->scale;	// 親Transformからの相対スケールを計算
	new_child->local_scale = scale_ / scale;	// 親Transformからの相対スケールを計算
	new_child->local_position = (rotation.rotate(new_child->position - position));	// 親Transformからの相対位置を計算
	new_child->local_rotation = rotation.getConjugate() * new_child->rotation;	// 親Transformからの相対回転を計算
	new_child->PreDraw();
}

void Transform::SetParent(TransformP new_parent)
{
	if (new_parent == nullptr) return;
	TransformWP p = new_parent;

	new_parent->SetChild(std::static_pointer_cast<Transform>(shared_from_this()));

}

void Transform::ResetChild(TransformWP reset_child)
{
	if (!reset_child) return;

	// 子Transformの親をnullptrに設定
	auto it = std::find(children.begin(), children.end(), reset_child);
	if (it != children.end()) {
		children.erase(it);
		reset_child->parent.reset();
	}
	// 子Transformのローカル位置、回転、スケールをリセット
	reset_child->local_position = reset_child->position;
	reset_child->local_rotation = reset_child->rotation;
	reset_child->local_scale = reset_child->scale;
}

void Transform::ResetParent()
{
	if (!parent) return; // 親Transformが既に無い場合は何もしない

	// 親Transformの子リストから自分を削除
	parent->ResetChild(std::static_pointer_cast<Transform>(shared_from_this()));
	// 親Transformへの参照をクリア
	parent.reset();

	// 親Transformがいなくなったので、ローカル位置、回転、スケールはグローバルと同じになる
	local_position = position;
	local_rotation = rotation;
	local_scale = scale;
}

void Transform::AddRotation(Vector3 euler_angles)
{
	Quaternion qx(DEG2RAD(euler_angles.x), Vector3(1, 0, 0));
	Quaternion qy(DEG2RAD(euler_angles.y), Vector3(0, 1, 0));
	Quaternion qz(DEG2RAD(euler_angles.z), Vector3(0, 0, 1));
	rotation = rotation * (qx * qy * qz);
	rotation.normalize();

}
void Transform::AddRotation(Quaternion q)
{
	rotation = rotation * q;
	rotation.normalize();
}

void Transform::SetRotation(Vector3 euler_angles)
{
	Quaternion qx(DEG2RAD(euler_angles.x), Vector3(1, 0, 0));
	Quaternion qy(DEG2RAD(euler_angles.y), Vector3(0, 1, 0));
	Quaternion qz(DEG2RAD(euler_angles.z), Vector3(0, 0, 1));
	rotation = qx * qy * qz;
}
void Transform::SetRotation(Quaternion q)
{
	rotation = q;
}

Vector3 Transform::AxisX()
{
	float3 vec = rotation.getBasisVector0();
	return vec;
}

Vector3 Transform::AxisY()
{
	float3 vec = rotation.getBasisVector1();
	return vec;
}

Vector3 Transform::AxisZ()
{
	float3 vec = rotation.getBasisVector2();
	return vec;

}

void Transform::SetAxisX(Vector3 target) {
	float3 right = { 1.0f, 0.0f, 0.0f }; // デフォルトの前方 (+Z)
	float3 targetNorm = target;
	targetNorm.normalize(); // 正規化

	float3 axis = GetFloat3Cross(right, targetNorm); // 回転軸
	float dot = GetFloat3Dot(right, targetNorm); // 内積
	float angle = acosf(dot); // 角度（ラジアン）

	// 回転が不要な場合（すでに一致している）
	if (fabs(dot - 1.0f) < FLT_EPSILON) {
		rotation *= { 1, 0, 0, 0 }; // 単位クォータニオン
	}

	axis.normalize(); // 軸を正規化
	rotation = Quaternion(angle, axis);
}

void Transform::SetAxisY(Vector3 target) {
	float3 up = { 0.0f, 1.0f, 0.0f }; // デフォルトの前方 (+Z)
	float3 targetNorm = target;
	targetNorm.normalize(); // 正規化

	float3 axis = GetFloat3Cross(up, targetNorm); // 回転軸
	float dot = GetFloat3Dot(up, targetNorm); // 内積
	float angle = acosf(dot); // 角度（ラジアン）

	// 回転が不要な場合（すでに一致している）
	if (fabs(dot - 1.0f) < FLT_EPSILON) {
		rotation *= { 1, 0, 0, 0 }; // 単位クォータニオン
	}

	axis.normalize(); // 軸を正規化
	rotation = Quaternion(angle, axis);
}
void Transform::SetAxisZ(Vector3 target) {
	float3 forward = { 0.0f, 0.0f, 1.0f }; // デフォルトの前方 (+Z)
	float3 targetNorm = target;
	targetNorm.normalize(); // 正規化

	float3 axis = GetFloat3Cross(forward, targetNorm); // 回転軸
	float dot = GetFloat3Dot(forward, targetNorm); // 内積
	float angle = acosf(dot); // 角度（ラジアン）

	// 回転が不要な場合（すでに一致している）
	if (fabs(dot - 1.0f) < FLT_EPSILON) {
		rotation *= { 1, 0, 0, 0 }; // 単位クォータニオン
	}

	axis.normalize(); // 軸を正規化
	rotation = Quaternion(angle, axis);
}