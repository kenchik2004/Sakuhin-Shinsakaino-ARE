#include "precompile.h"
#include "DiceScene.h"
#include <cmath>

#include "Game/Objects/RLyeh/Dices/DiceBase.h"
#include "Game/Objects/RLyeh/Dices/DiceD4.h"
#include "Game/Objects/RLyeh/Dices/DiceD6.h"
#include "Game/Objects/RLyeh/Dices/DiceD8.h"
#include "Game/Objects/RLyeh/Dices/DiceD10.h"
#include "Game/Objects/RLyeh/Dices/DiceD12.h"
#include "Game/Objects/RLyeh/Dices/DiceD20.h"
#include "System/Components/ModelRenderer.h"
#include "System/Components/RigidBody.h"
#include "System/Components/MeshCollider.h"
#include"System/Components/Camera.h"
#include <System/IniFileManager.h>
#include "Game/Objects/RLyeh/Cards/CardBase.h"
#include <System/Objects/ShadowMapObject.h>

namespace {
	std::array<std::string, 5> card_name = {
		"CardBase",
		"CardFist",
		"CardShinsoku",
		"CardDoman",
		"CardUnpu"
	};
}

namespace RLyeh {
	bool a = false;
	int DiceScene::Init()
	{
		GetPhysicsScene()->addActor(*physx::PxCreatePlane(
			*PhysicsManager::GetPhysicsInstance(), physx::PxPlane(0, 1, 0, -0.05f),
			*Material::Default)
		);
		GetPhysicsScene()->addActor(*physx::PxCreatePlane(
			*PhysicsManager::GetPhysicsInstance(), physx::PxPlane(0, -1, 0, 10),
			*Material::Default)
		);
		GetPhysicsScene()->addActor(*physx::PxCreatePlane(
			*PhysicsManager::GetPhysicsInstance(), physx::PxPlane(1, 0, 0, 4),
			*Material::Default)
		);
		GetPhysicsScene()->addActor(*physx::PxCreatePlane(
			*PhysicsManager::GetPhysicsInstance(), physx::PxPlane(-1, 0, 0, 4),
			*Material::Default)
		);
		GetPhysicsScene()->addActor(*physx::PxCreatePlane(
			*PhysicsManager::GetPhysicsInstance(), physx::PxPlane(0, 0, 1, 4),
			*Material::Default)
		);
		GetPhysicsScene()->addActor(*physx::PxCreatePlane(
			*PhysicsManager::GetPhysicsInstance(), physx::PxPlane(0, 0, -1, 4),
			*Material::Default)
		);
		SceneManager::Object::Create<ShadowMapObject>(shared_from_this())->SetShadowMapSize(2048);
		auto tray = SceneManager::Object::Create<GameObject>(shared_from_this());
		tray->AddComponent<RigidBody>();
		tray->transform->scale = { 5,5,5 };
		tray->transform->position = { 0,-0.05f,0 };
		tray->transform->AddRotation(Vector3(0, 90, 0));
		tray->SetPriority(0);

		auto model = tray->AddComponent<ModelRenderer>();
		model->SetModel("Tray");
		tray->AddComponent<MeshCollider>();
		camera = SceneManager::Object::Create<GameObject>(shared_from_this());
		auto cam = camera->AddComponent<Camera>();
		cam->camera_near = 0.1f;
		cam->camera_far = 1000;
		camera->transform->position = { 0,8,-5 };
		camera->transform->SetAxisZ(Vector3(0, -8, 5).getNormalized());
		return 0;
	}

	void DiceScene::Load()
	{
		texture[1] = LoadGraph("data/dice/D6.png");
		texture[2] = LoadGraph("data/dice/D8.png");
		texture[3] = LoadGraph("data/dice/D10.png");
		texture[4] = LoadGraph("data/dice/D12.png");
		texture[5] = LoadGraph("data/dice/D20.png");
		tray_texture = LoadGraph("data/dice/Tray.png");
		ModelManager::LoadAsModel("data/dice/D4.mv1", "D4");
		ModelManager::LoadAsModel("data/dice/D6.mv1", "D6");
		ModelManager::LoadAsModel("data/dice/D8.mv1", "D8");
		ModelManager::LoadAsModel("data/dice/D10.mv1", "D10");
		ModelManager::LoadAsModel("data/dice/D12.mv1", "D12");
		ModelManager::LoadAsModel("data/dice/D20.mv1", "D20");
		ModelManager::LoadAsModel("data/dice/DiceTray.mv1", "Tray");
		ModelManager::LoadAsModel(reinterpret_cast<const char*>(u8"data/カード/Card.mv1"), "card");

	}

	void DiceScene::Roll(int first, int second, bool use_dices)
	{
		if (first <= 0 || second <= 0)
			return;
		if (!use_dices) {
			for (int i = 0; i < first; i++) {
				results_out_of_dices.push_back(Random::Int(1, second));
			}
			return;
		}
		if (first > 5) {

			for (int i = first; i > 5; i--) {
				results_out_of_dices.push_back(Random::Int(1, second));
			}
			first = 5;
		}
		switch (second) {
		case 4:
			for (int i = 0; i < first; i++) {
				auto obj = SceneManager::Object::Create<DiceD4>(shared_from_this());
				obj->transform->position = Random::Position({ -2.5f,8,-2.5f }, { 2.5f,8,2.5f });//{ 0.1f * (GetRand(50) - 25),8, 0.1f * (GetRand(50) - 25) };
				DicePair pair(DicePair::D4, obj);
				dices.push_back(std::move(pair));
			}
			break;
		case 6:
			for (int i = 0; i < first; i++) {
				auto obj = SceneManager::Object::Create<DiceD6>(shared_from_this(), texture[1]);
				obj->transform->position = { 0.1f * (GetRand(50) - 25),8, 0.1f * (GetRand(50) - 25) };
				DicePair pair(DicePair::D6, obj);
				dices.push_back(std::move(pair));
			}
			break;
		case 8:
			for (int i = 0; i < first; i++) {
				auto obj = SceneManager::Object::Create<DiceD8>(shared_from_this(), texture[2]);
				obj->transform->position = { 0.1f * (GetRand(50) - 25),8, 0.1f * (GetRand(50) - 25) };
				DicePair pair(DicePair::D8, obj);
				dices.push_back(std::move(pair));
			}
			break;
		case 10:
			for (int i = 0; i < first; i++) {
				auto obj = SceneManager::Object::Create<DiceD10>(shared_from_this(), texture[3]);
				obj->transform->position = { 0.1f * (GetRand(50) - 25),8, 0.1f * (GetRand(50) - 25) };
				MV1SetMaterialDifColor(obj->GetComponent<ModelRenderer>()->GetModelHandle(), 0, Color::RED);
				DicePair pair(DicePair::D10, obj);
				dices.push_back(std::move(pair));
			}
			break;
		case 12:
			for (int i = 0; i < first; i++) {
				auto obj = SceneManager::Object::Create<DiceD12>(shared_from_this(), texture[4]);
				obj->transform->position = { 0.1f * (GetRand(50) - 25),8, 0.1f * (GetRand(50) - 25) };
				DicePair pair(DicePair::D12, obj);
				dices.push_back(std::move(pair));
			}
			break;
		case 20:
			for (int i = 0; i < first; i++) {
				auto obj = SceneManager::Object::Create<DiceD20>(shared_from_this(), texture[5]);
				obj->transform->position = { 0.1f * (GetRand(50) - 25),8, 0.1f * (GetRand(50) - 25) };
				DicePair pair(DicePair::D20, obj);
				dices.push_back(std::move(pair));
			}
			break;
		case 100:
			for (int i = 0; i < first; i++) {
				DicePair pair;
				pair.mode = DicePair::D100;
				for (int i = 0; i < 2; i++) {

					auto obj = SceneManager::Object::Create<DiceD10>(shared_from_this(), texture[3]);
					if (i == 0)
						MV1SetMaterialDifColor(obj->GetComponent<ModelRenderer>()->GetModelHandle(), 0, Color::RED);
					else
						MV1SetMaterialDifColor(obj->GetComponent<ModelRenderer>()->GetModelHandle(), 0, Color::BLUE);

					obj->transform->position = { 0.1f * (GetRand(50) - 25),8, 0.1f * (GetRand(50) - 25) };
					pair.pair[i] = obj;
				}
				dices.push_back(std::move(pair));
			}
			break;
		default:
			for (int i = 0; i < first; i++)
				results_out_of_dices.push_back(GetRand(second - 1) + 1);

		}
		if (!a) {
			a=true;
		for (int i = 0; i < 1; i++)
			CreateGameObjectFromPtr<Object>(CreateInstanceFromName<GameObject>("RLyeh::" + card_name[GetRand(4)]), "Card");
		}

	}

	void DiceScene::Skip()
	{
		constexpr float delta = 1.0f / 50.0f;
		if (dices.empty())	//ダイスがないなら無駄に処理をさせない。
			return;
		for (int i = 0; i < 50 * 40; i++) {
			GetPhysicsScene()->simulate(delta);
			GetPhysicsScene()->fetchResults(true);
		}
		SceneManager::PostPhysics();
	}

	void DiceScene::Clear()
	{
		for (auto ite = dices.begin(); ite != dices.end();) {
			SceneManager::Object::Destory(shared_from_this(), (*ite).pair[0]);
			SceneManager::Object::Destory(shared_from_this(), (*ite).pair[1]);
			ite = dices.erase(ite);
		}
		results_out_of_dices.clear();
	}

	std::vector<int> DiceScene::FetchResults()
	{
		std::vector<int> results;
		//ダイスがあるならいいが、1D3などはダイスが存在しないので、予め計算した結果から持ってくる
		if (!dices.empty()) {

			for (auto& ite : dices) {
				results.push_back(ite.Result());
			}
			// もし単独で計算した結果(軽量化のためのダイス数削減など)
			// があるなら、追加する。
			if (!results_out_of_dices.empty())
				results.insert(results.end(), results_out_of_dices.begin(), results_out_of_dices.end());
		}
		else {
			for (auto& ite : results_out_of_dices)
				results.push_back(std::move(ite));
		}
		return results;
	}

	void DiceScene::Exit()
	{
		Clear();
	}

	void DiceScene::Draw()
	{
		if (Input::GetKey(KeyCode::Up))
			camera->transform->AddRotation(Quaternion(DEG2RAD(Time::DrawDeltaTime() * -10), { 1,0,0 }));
		if (Input::GetKey(KeyCode::Down))
			camera->transform->AddRotation(Quaternion(DEG2RAD(Time::DrawDeltaTime() * 10), { 1,0,0 }));
		//	if (Input::GetKeyDown(KeyCode::Return))
			//	camera->transform->AddRotation(Quaternion(DEG2RAD(180), { 0,1,0 }));

		if (Input::GetKey(KeyCode::Left))
			camera->transform->AddRotation(Quaternion(DEG2RAD(Time::DrawDeltaTime() * -10), { 0,1,0 }));
		if (Input::GetKey(KeyCode::Right))
			camera->transform->AddRotation(Quaternion(DEG2RAD(Time::DrawDeltaTime() * 10), { 0,1,0 }));
		if (Input::GetKey(KeyCode::E))
			camera->transform->position += Time::DrawDeltaTime() * camera->transform->AxisY() * 5;
		if (Input::GetKey(KeyCode::Q))
			camera->transform->position += Time::DrawDeltaTime() * camera->transform->AxisY() * -5;
		if (Input::GetKey(KeyCode::W))
			camera->transform->position += Time::DrawDeltaTime() * camera->transform->AxisZ() * 5;
		if (Input::GetKey(KeyCode::S))
			camera->transform->position += Time::DrawDeltaTime() * -camera->transform->AxisZ() * 5;
		if (Input::GetKey(KeyCode::D))
			camera->transform->position += Time::DrawDeltaTime() * camera->transform->AxisX() * 5;
		if (Input::GetKey(KeyCode::A))
			camera->transform->position += Time::DrawDeltaTime() * camera->transform->AxisX() * -5;


	}

	void DiceScene::OnDrawFinish()
	{
	}

	void DiceScene::UnLoad()
	{
		for (int i = 0; i < 6; i++)
			DeleteGraph(texture[i]);
		DeleteGraph(tray_texture);
	}

}