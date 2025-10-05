#include "precompile.h"
#include "System/Object.h"
#include "System/Scene.h"
#include <algorithm>



GameObject::GameObject()
{
	status.obj_type = ObjStat::NORMAL;
}

GameObject::GameObject(std::string name_)
{
	status.obj_type = ObjStat::NORMAL;
	name = name_;
}

void GameObject::DebugDraw()
{
}

void Object::SyncComponentsPriority()
{
	if (dirty_priority_components.empty()) return;

	// 1) dirty_GetPriority()_components を優先度順に並べ替える
	std::sort(dirty_priority_components.begin(), dirty_priority_components.end(),
		[](ComponentP a, ComponentP b) { return a->GetPriority() < b->GetPriority(); });

	// 2) まとめて components へ挿入
	for (ComponentP& obj : dirty_priority_components)
	{
		if (obj->status.status_bit.is(CompStat::STATUS::REMOVED))
			continue;
		// 2‑1) 古い場所を消す（同じポインタ重複を防ぐ）
		auto cur = std::find(components.begin(), components.end(), obj);
		if (cur != components.end()) components.erase(cur);

		// 2‑2) 優先度に合った場所を二分探索で探す
		auto pos = std::upper_bound(components.begin(), components.end(), obj,
			[](ComponentP a, ComponentP b) { return a->GetPriority() < b->GetPriority(); });

		components.insert(pos, obj);  // ここに差し込む
	}
	dirty_priority_components.clear();
}

bool Object::CheckForSingleComponent(ComponentP comp)
{
	//一匹しかいちゃいけないコンポーネントの場合、既に同じものがいないかチェックする
	if (comp->status.status_bit.is(CompStat::STATUS::SINGLE))
		//コストはかかるが、一つ一つクラス名をチェックする
		for (auto& cmp : components)
			if (cmp->Info()->ClassName() == comp->Info()->ClassName())
			{
				//いた場合、登録しない
				return false;
			}
	return true;
}

void Object::SetPriority(unsigned int prio)
{
	unsigned int my_prio = GetPriority();
	scene->SetObjPriority(prio, shared_from_this());
	if (!transform->children.empty())
	{
		for (auto& child : transform->children) {
			//
			unsigned int child_prio = child->GetPriority();
			bool child_greater = child_prio > my_prio;
			child->SetPriority(child_greater ? (prio + (my_prio - child_prio)) : (prio));
		}
	}
}


UIObject::UIObject() : Object()
{
	status.obj_type = ObjStat::UI;
	tag = UI;
}


int UIObject::Init()
{
	SetPriority(2000);
	return Super::Init();
}

void UIObject::PreUpdate()
{
	draw_pos = transform->position;
	draw_pos.y *= -1;
	Vector3 div = { 0, 0, 0 };
	Vector3 scale = transform->scale;
	switch (anchor_type) {
	case LEFT_TOP:
		div = { 0, 0, 0 };
		break;
	case CENTER_TOP:
		div = { scale.x * -0.5f, 0, 0 };
		break;
	case RIGHT_TOP:
		div = { scale.x * -1, 0, 0 };
		break;
	case LEFT_MIDDLE:
		div = { 0, scale.y * -0.5f, 0 };
		break;
	case CENTER:
		div = { scale.x * -0.5f, scale.y * -0.5f, 0 };
		break;
	case RIGHT_MIDDLE:
		div = { scale.x * -1, scale.y * -0.5f, 0 };
		break;
	case LEFT_BOTTOM:
		div = { 0, scale.y * -1, 0 };
		break;
	case CENTER_BOTTOM:
		div = { scale.x * -0.5f, scale.y * -1, 0 };
		break;
	case RIGHT_BOTTOM:
		div = { scale.x * -1, scale.y * -1, 0 };
		break;
	}
	switch (canvas_anchor_type) {
	case LEFT_TOP:
		div += {0, 0, 0};
		break;
	case CENTER_TOP:
		div += {static_cast<float>(SCREEN_W) * 0.5f, 0, 0};
		break;
	case RIGHT_TOP:
		div += {static_cast<float>(SCREEN_W), 0, 0};
		break;
	case LEFT_MIDDLE:
		div += {0, static_cast<float>(SCREEN_H) * 0.5f, 0};
		break;
	case CENTER:
		div += {static_cast<float>(SCREEN_W) * 0.5f, static_cast<float>(SCREEN_H) * 0.5f, 0};
		break;
	case RIGHT_MIDDLE:
		div += {static_cast<float>(SCREEN_W), static_cast<float>(SCREEN_H) * 0.5f, 0};
		break;
	case LEFT_BOTTOM:
		div += {0, static_cast<float>(SCREEN_H), 0};
		break;
	case CENTER_BOTTOM:
		div += {static_cast<float>(SCREEN_W) * 0.5f, static_cast<float>(SCREEN_H), 0};
		break;
	case RIGHT_BOTTOM:
		div += {static_cast<float>(SCREEN_W), static_cast<float>(SCREEN_H), 0};
		break;
	}

	draw_pos += div;
}

void UIObject::Update()
{
#ifndef NDEBUG
	if (CheckHitKey(KEY_INPUT_LCONTROL)) {
		if (CheckHitKey(KEY_INPUT_NUMPAD7))
			anchor_type = LEFT_TOP;
		if (CheckHitKey(KEY_INPUT_NUMPAD8))
			anchor_type = CENTER_TOP;
		if (CheckHitKey(KEY_INPUT_NUMPAD9))
			anchor_type = RIGHT_TOP;
		if (CheckHitKey(KEY_INPUT_NUMPAD4))
			anchor_type = LEFT_MIDDLE;
		if (CheckHitKey(KEY_INPUT_NUMPAD5))
			anchor_type = CENTER;
		if (CheckHitKey(KEY_INPUT_NUMPAD6))
			anchor_type = RIGHT_MIDDLE;
		if (CheckHitKey(KEY_INPUT_NUMPAD1))
			anchor_type = LEFT_BOTTOM;
		if (CheckHitKey(KEY_INPUT_NUMPAD2))
			anchor_type = CENTER_BOTTOM;
		if (CheckHitKey(KEY_INPUT_NUMPAD3))
			anchor_type = RIGHT_BOTTOM;
	}
	else {
		if (CheckHitKey(KEY_INPUT_NUMPAD7))
			canvas_anchor_type = LEFT_TOP;
		if (CheckHitKey(KEY_INPUT_NUMPAD8))
			canvas_anchor_type = CENTER_TOP;
		if (CheckHitKey(KEY_INPUT_NUMPAD9))
			canvas_anchor_type = RIGHT_TOP;
		if (CheckHitKey(KEY_INPUT_NUMPAD4))
			canvas_anchor_type = LEFT_MIDDLE;
		if (CheckHitKey(KEY_INPUT_NUMPAD5))
			canvas_anchor_type = CENTER;
		if (CheckHitKey(KEY_INPUT_NUMPAD6))
			canvas_anchor_type = RIGHT_MIDDLE;
		if (CheckHitKey(KEY_INPUT_NUMPAD1))
			canvas_anchor_type = LEFT_BOTTOM;
		if (CheckHitKey(KEY_INPUT_NUMPAD2))
			canvas_anchor_type = CENTER_BOTTOM;
		if (CheckHitKey(KEY_INPUT_NUMPAD3))
			canvas_anchor_type = RIGHT_BOTTOM;
	}
#endif
}


void UIObject::PreDraw()
{
	Super::PreDraw();
	auto scale = transform->local_scale;
	transform->local_scale = { scale.x, scale.y, 0 };
	auto pos = transform->local_position;
	transform->local_position = { pos.x, pos.y, 0 };

}

void UIObject::LateDraw()
{
	Vector3 scale = transform->scale;
	if (use_back_color) {
		SetDrawBlendMode(DX_BLENDMODE_ALPHA, 128);
		DrawBoxAA(draw_pos.x, draw_pos.y, draw_pos.x + scale.x, draw_pos.y + scale.y, back_ground_color, true);
		SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
	}
}
