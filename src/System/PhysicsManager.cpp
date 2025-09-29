#include "precompile.h"
#include "..\PhysX\PhysX-5.5.1\include\cooking\PxCooking.h"
#include "PhysicsManager.h"
#include "System/Object.h"
#include <System/Components/Collider.h>

using namespace physx;
// PhysXの初期化に必要なコールバック
// メモリ確保用のコールバックでメモリの確保と解放を行う
PxDefaultAllocator PhysicsManager::m_defaultAllocator;
// エラー時用のコールバックでエラー内容が入ってる
PxDefaultErrorCallback PhysicsManager::m_defaultErrorCallback;
// 上位レベルのSDK(PxPhysicsなど)をインスタンス化する際に必要
PxFoundation* PhysicsManager::m_pFoundation = nullptr;
// 実際に物理演算を行う
PxPhysics* PhysicsManager::m_pPhysics = nullptr;
// シミュレーションをどう処理するかの設定でマルチスレッドの設定もできる
PxDefaultCpuDispatcher* PhysicsManager::m_pDispatcher = nullptr;
// PVDと通信する際に必要
PxPvd* PhysicsManager::m_pPvd = nullptr;
// シーンの配列
std::vector<physx::PxScene*> PhysicsManager::scenes(0);

// マテリアルのインスタンス
physx::PxMaterial* Material::Default = nullptr;
physx::PxMaterial* Material::Metal = nullptr;
physx::PxMaterial* Material::Rubber = nullptr;
physx::PxMaterial* Material::Wood = nullptr;
physx::PxMaterial* Material::Plastic = nullptr;
physx::PxMaterial* Material::Glass = nullptr;
physx::PxMaterial* Material::Concrete = nullptr;
physx::PxMaterial* Material::Asphalt = nullptr;
physx::PxMaterial* Material::Wool = nullptr;
physx::PxMaterial* Material::Paper = nullptr;

// コールバッククラスのインスタンス
HitCallBack hit_callback;


PxFilterFlags filtershader(PxFilterObjectAttributes attributes0,
	PxFilterData filterData0,
	PxFilterObjectAttributes attributes1,
	PxFilterData filterData1,
	PxPairFlags& pairFlags,
	const void* constantBlock,
	PxU32 constantBlockSize) {
	if (!(filterData0.word0 & filterData1.word1) || !(filterData1.word0 & filterData0.word1))
		if (filterData0.word1 != 0 && filterData1.word1 != 0)
			return PxFilterFlag::eSUPPRESS;

	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1)) {

		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;


	}
	else {
		pairFlags = PxPairFlag::eCONTACT_DEFAULT;
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_PERSISTS;
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_LOST;
	}


	return PxFilterFlag::eDEFAULT;
}


void PhysicsManager::Init()
{
	if (!(m_pFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, m_defaultAllocator, m_defaultErrorCallback))) {
#ifndef PACKAGE_BUILD
		throw Exception("PhysXの初期化に失敗しました。", DEFAULT_EXCEPTION_PARAM);
#endif
	}
	// PVDと接続する設定
	if (m_pPvd = physx::PxCreatePvd(*m_pFoundation)) {
		// PVD側のデフォルトポートは5425
		physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);

		int try_num = 0;
		//3回ほど試す
		while (try_num < 3) {
			//PVDとの接続を試みる
			bool success = m_pPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);
			if (success)
				break;
			//駄目なら少し待ってから再度試みる
			Sleep(10);
			try_num++;
		}
	}
	// Physicsのインスタンス化
	if (!(m_pPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *m_pFoundation, physx::PxTolerancesScale(), true, m_pPvd))) {
#ifndef PACKAGE_BUILD
		throw(Exception("PhysXのインスタンス生成に失敗しました。", DEFAULT_EXCEPTION_PARAM));
#endif
	}
	// 拡張機能用
	if (!PxInitExtensions(*m_pPhysics, m_pPvd)) {
		return (void)false;
	}
	// 処理に使うスレッドを指定する
	m_pDispatcher = physx::PxDefaultCpuDispatcherCreate(16);
	physx::PxSceneDesc scene_desc(m_pPhysics->getTolerancesScale());
	scene_desc.simulationEventCallback = &hit_callback;

	Material::Default = m_pPhysics->createMaterial(0.6f, 0.6f, 0.0f);
	Material::Metal = m_pPhysics->createMaterial(0.6f, 0.5f, 0.2f);
	Material::Rubber = m_pPhysics->createMaterial(1.1f, 0.8f, 0.7f);
	Material::Wood = m_pPhysics->createMaterial(0.6f, 0.5f, 0.2f);
	Material::Plastic = m_pPhysics->createMaterial(0.4f, 0.2f, 0.3f);
	Material::Glass = m_pPhysics->createMaterial(0.6f, 0.5f, 0.2f);
	Material::Concrete = m_pPhysics->createMaterial(1.0f, 0.8f, 0.2f);
	Material::Asphalt = m_pPhysics->createMaterial(0.8f, 0.6f, 0.3f);
	Material::Wool = m_pPhysics->createMaterial(0.7f, 0.5f, 0.05f);
	Material::Paper = m_pPhysics->createMaterial(0.5f, 0.4f, 0.1f);

}

void PhysicsManager::Exit()
{
	//拡張機能を使わなくする
	PxCloseExtensions();
	for (auto& ite : scenes) {
		ite->release();
	}

	Material::Default->release();
	Material::Metal->release();
	Material::Rubber->release();
	Material::Wood->release();
	Material::Plastic->release();
	Material::Glass->release();
	Material::Concrete->release();
	Material::Asphalt->release();
	Material::Wool->release();
	Material::Paper->release();
	m_pDispatcher->release();
	m_pPhysics->release();
	if (m_pPvd) {
		m_pPvd->disconnect();
		physx::PxPvdTransport* transport = m_pPvd->getTransport();
		m_pPvd->release();
		transport->release();
	}
	m_pFoundation->release();
}

physx::PxScene* PhysicsManager::AddScene()
{
	// シミュレーションする空間の単位でActorの追加などをここで行う
	// 空間の設定
	physx::PxSceneDesc scene_desc(m_pPhysics->getTolerancesScale());
	scene_desc.gravity = physx::PxVec3(0, -9.81f, 0);

	scene_desc.filterShader = filtershader;
	scene_desc.cpuDispatcher = m_pDispatcher;
	scene_desc.simulationEventCallback = &hit_callback;
	scene_desc.flags |= PxSceneFlag::eENABLE_CCD;
	scene_desc.flags |= PxSceneFlag::eENABLE_PCM;
	scene_desc.flags |= PxSceneFlag::eENABLE_STABILIZATION;



	physx::PxScene* scene = nullptr;

	// 空間のインスタンス化
	scene = (m_pPhysics->createScene(scene_desc));
	// PVDの表示設定

	physx::PxPvdSceneClient* pvd_client;
	if (pvd_client = scene->getScenePvdClient()) {
		pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvd_client->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);
	}

	scenes.push_back(scene);

	return scene;
}

void PhysicsManager::ReleaseScene(physx::PxScene* scene_)
{
	for (auto ite = scenes.begin(); ite != scenes.end();) {
		if (*ite == scene_) {
			(*ite)->release();
			ite = scenes.erase(ite);
			break;
		}
		ite++;
	}
}

void HitCallBack::onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs)
{
	for (physx::PxU32 i = 0; i < nbPairs; i++)
	{
		PxContactPair contact = pairs[i];
		if (contact.events & PxPairFlag::eNOTIFY_TOUCH_FOUND) // 衝突が発生したとき
		{
			auto wp_a = static_cast<SafeWeakPtr<Collider>*>((pairHeader.pairs->shapes[0]->userData));
			auto wp_b = static_cast<SafeWeakPtr<Collider>*>((pairHeader.pairs->shapes[1]->userData));
			//SafeWeakPtrへのポインタにすることで、userDataがnullptrになっていたり、shared_ptrが解放されていてもアクセスすることがない
			if (wp_a && wp_b) {
				//ヒット時に、オブジェクトを消しちゃうおバカちゃんたちがいるかもしれないので
				//あらかじめlockしてコールバック内で解放されないようにしておく
				auto sp_a = wp_a->lock();
				auto sp_b = wp_b->lock();
				if (sp_a && sp_b) {
					HitInfo hit_info0;
					hit_info0.collision = sp_a;
					hit_info0.hit_collision = sp_b;
					sp_a->owner->OnCollisionEnter(hit_info0); // オブジェクトAのヒット発生関数を呼ぶ
					HitInfo hit_info1;
					hit_info1.collision = sp_b;
					hit_info1.hit_collision = sp_a;
					sp_b->owner->OnCollisionEnter(hit_info1); // オブジェクトBのヒット発生関数を呼ぶ
				}

			}
		}

		if (contact.events & PxPairFlag::eNOTIFY_TOUCH_PERSISTS) // 衝突が続けて発生したとき
		{

			auto wp_a = static_cast<SafeWeakPtr<Collider>*>((pairHeader.pairs->shapes[0]->userData));
			auto wp_b = static_cast<SafeWeakPtr<Collider>*>((pairHeader.pairs->shapes[1]->userData));
			if (wp_a && wp_b) {
				auto sp_a = wp_a->lock();
				auto sp_b = wp_b->lock();
				if (sp_a && sp_b) {
					HitInfo hit_info0;
					hit_info0.collision = sp_a;
					hit_info0.hit_collision = sp_b;
					sp_a->owner->OnCollisionStay(hit_info0); // オブジェクトAのヒット継続関数を呼ぶ
					HitInfo hit_info1;
					hit_info1.collision = sp_b;
					hit_info1.hit_collision = sp_a;
					sp_b->owner->OnCollisionStay(hit_info1); // オブジェクトBのヒット継続関数を呼ぶ
				}
			}
		}

		if (contact.events & PxPairFlag::eNOTIFY_TOUCH_LOST) // 衝突が終了したとき
		{
			auto wp_a = static_cast<SafeWeakPtr<Collider>*>((pairHeader.pairs->shapes[0]->userData));
			auto wp_b = static_cast<SafeWeakPtr<Collider>*>((pairHeader.pairs->shapes[1]->userData));
			if (wp_a && wp_b) {
				auto sp_a = wp_a->lock();
				auto sp_b = wp_b->lock();
				if (sp_a && sp_b) {
					HitInfo hit_info0;
					hit_info0.collision = sp_a;
					hit_info0.hit_collision = sp_b;
					sp_a->owner->OnCollisionExit(hit_info0); // オブジェクトAのヒット終了関数を呼ぶ
					HitInfo hit_info1;
					hit_info1.collision = sp_b;
					hit_info1.hit_collision = sp_a;
					sp_b->owner->OnCollisionExit(hit_info1); // オブジェクトBのヒット終了関数を呼ぶ
				}
			}
		}
	}

}

void HitCallBack::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count)
{
	for (physx::PxU32 i = 0; i < count; i++)
	{
		PxTriggerPair contact = pairs[i];
		if (contact.status & PxPairFlag::eNOTIFY_TOUCH_FOUND) // 衝突が発生したとき
		{
			auto wp_a = static_cast<SafeWeakPtr<Collider>*>((pairs[i].triggerShape->userData));
			auto wp_b = static_cast<SafeWeakPtr<Collider>*>((pairs[i].otherShape->userData));
			//SafeWeakPtrへのポインタにすることで、userDataがnullptrになっていたり、shared_ptrが解放されていてもアクセスすることがない
			if (wp_a && wp_b) {

				auto sp_a = wp_a->lock();
				auto sp_b = wp_b->lock();
				if (sp_a && sp_b) {
					HitInfo hit_info0;
					hit_info0.collision = sp_a;
					hit_info0.hit_collision = sp_b;
					sp_a->owner->OnTriggerEnter(hit_info0); // オブジェクトAのトリガー発生関数を呼ぶ
					HitInfo hit_info1;
					hit_info1.collision = sp_b;
					hit_info1.hit_collision = sp_a;
					sp_b->owner->OnTriggerEnter(hit_info1); // オブジェクトBのトリガー発生関数を呼ぶ
				}

			}
		}

		if (contact.status & PxPairFlag::eNOTIFY_TOUCH_PERSISTS) // 衝突が続けて発生したとき
		{

			auto wp_a = static_cast<SafeWeakPtr<Collider>*>((pairs[i].triggerShape->userData));
			auto wp_b = static_cast<SafeWeakPtr<Collider>*>((pairs[i].otherShape->userData));
			if (wp_a && wp_b) {
				auto sp_a = wp_a->lock();
				auto sp_b = wp_b->lock();
				if (sp_a && sp_b) {
					HitInfo hit_info0;
					hit_info0.collision = sp_a;
					hit_info0.hit_collision = sp_b;
					sp_a->owner->OnTriggerStay(hit_info0); // オブジェクトAのトリガー継続関数を呼ぶ
					HitInfo hit_info1;
					hit_info1.collision = sp_b;
					hit_info1.hit_collision = sp_a;
					sp_b->owner->OnTriggerStay(hit_info1); // オブジェクトBのトリガー継続関数を呼ぶ
				}
			}
		}

		if (contact.status & PxPairFlag::eNOTIFY_TOUCH_LOST) // 衝突が終了したとき
		{
			auto wp_a = static_cast<SafeWeakPtr<Collider>*>((pairs[i].triggerShape->userData));
			auto wp_b = static_cast<SafeWeakPtr<Collider>*>((pairs[i].otherShape->userData));
			if (wp_a && wp_b) {
				auto sp_a = wp_a->lock();
				auto sp_b = wp_b->lock();
				if (sp_a && sp_b) {
					HitInfo hit_info0;
					hit_info0.collision = sp_a;
					hit_info0.hit_collision = sp_b;
					sp_a->owner->OnTriggerExit(hit_info0); // オブジェクトAのトリガー終了関数を呼ぶ
					HitInfo hit_info1;
					hit_info1.collision = sp_b;
					hit_info1.hit_collision = sp_a;
					sp_b->owner->OnTriggerExit(hit_info1); // オブジェクトBのトリガー終了関数を呼ぶ
				}
			}
		}
	}
}
