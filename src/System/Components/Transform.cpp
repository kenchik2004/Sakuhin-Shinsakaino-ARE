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
	if (new_child->parent)
		new_child->parent->ResetChild(new_child); // 既に親がいる場合は、親の子リストから削除しておく
	new_child->parent = SafeWeakPtr<Transform>(std::static_pointer_cast<Transform>(shared_from_this()));
	children.push_back(new_child);
	new_child->owner->SetPriority(new_child->owner->GetPriority());	// 親の優先度を引き継ぐためにSetPriorityを呼び出す(親がいる場合は勝手に加算されるので、今の優先度をそのまま与える)
	Vector3 scale_ = new_child->scale;	// 親Transformからの相対スケールを計算
	new_child->local_scale = scale_ / scale;	// 親Transformからの相対スケールを計算
	new_child->local_position = (new_child->local_scale / scale_).multiply(rotation.rotate(new_child->position - position));	// 親Transformからの相対位置を計算
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
	rotation = rotation * (qz * qy * qx);
	rotation.normalize();

}
void Transform::AddRotation(Quaternion q)
{
	if (q.isIdentity())
		return;
	rotation = rotation * q;
	rotation.normalize();
}

void Transform::SetRotation(Vector3 euler_angles)
{
	Quaternion qx(DEG2RAD(euler_angles.x), Vector3(1, 0, 0));
	Quaternion qy(DEG2RAD(euler_angles.y), Vector3(0, 1, 0));
	Quaternion qz(DEG2RAD(euler_angles.z), Vector3(0, 0, 1));
	rotation = qz * qy * qx;
}
void Transform::SetRotation(Quaternion q)
{
	q.normalize();
	rotation = q;
}

void Transform::AddLocalRotation(Quaternion q)
{
	if (q.isIdentity())
		return;
	local_rotation = local_rotation * q;
	local_rotation.normalize();
}

void Transform::AddLocalRotation(Vector3 euler_angles)
{
	Quaternion qx(DEG2RAD(euler_angles.x), Vector3(1, 0, 0));
	Quaternion qy(DEG2RAD(euler_angles.y), Vector3(0, 1, 0));
	Quaternion qz(DEG2RAD(euler_angles.z), Vector3(0, 0, 1));
	local_rotation = local_rotation * (qz * qy * qx);
	local_rotation.normalize();
}

void Transform::SetLocalRotation(Quaternion q)
{
	q.normalize();
	local_rotation = q;
}

void Transform::SetLocalRotation(Vector3 euler_angles)
{
	Quaternion qx(DEG2RAD(euler_angles.x), Vector3(1, 0, 0));
	Quaternion qy(DEG2RAD(euler_angles.y), Vector3(0, 1, 0));
	Quaternion qz(DEG2RAD(euler_angles.z), Vector3(0, 0, 1));
	local_rotation = qz * qy * qx;
}

Vector3 Transform::AxisX()
{
	Vector3 vec = rotation.getBasisVector0();
	return vec;
}

Vector3 Transform::AxisY()
{
	Vector3 vec = rotation.getBasisVector1();
	return vec;
}

Vector3 Transform::AxisZ()
{
	Vector3 vec = rotation.getBasisVector2();
	return vec;

}

void Transform::SetAxisX(Vector3 target, Vector3 up) {
	Vector3 targetNorm = target.getNormalized();	//新しいX軸
	Vector3 upNorm = up.getNormalized();		//基準となるY軸
	Vector3 newZ = targetNorm.cross(upNorm).getNormalized(); //二つをもとに新しいZ軸を計算
	rotation = Quaternion({ targetNorm, upNorm, newZ });	//3軸の方向からクォータニオンを作成
}

void Transform::SetAxisY(Vector3 target, Vector3 right) {
	Vector3 targetNorm = target.getNormalized();	//新しいY軸
	Vector3 rightNorm = right.getNormalized();		//基準となるX軸
	Vector3 newZ = rightNorm.cross(targetNorm).getNormalized(); //二つをもとに新しいZ軸を計算
	rotation = Quaternion({ right, targetNorm, newZ });	//3軸の方向からクォータニオンを作成
}
void Transform::SetAxisZ(Vector3 target, Vector3 up) {
	Vector3 targetNorm = target.getNormalized();	//新しいZ軸
	Vector3 upNorm = up.getNormalized();		//基準となるY軸
	Vector3 newX = upNorm.cross(targetNorm).getNormalized(); //二つをもとに新しいX軸を計算
	rotation = Quaternion({ newX, upNorm, targetNorm });	//3軸の方向からクォータニオンを作成
}