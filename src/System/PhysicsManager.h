#pragma once

using RayCastInfo = physx::PxRaycastBuffer;
class Ray {
public:
	Vector3 position;
	Vector3 direction;
	float length;
};
class PhysicsManager
{
	static physx::PxDefaultAllocator m_defaultAllocator;
	// エラー時用のコールバックでエラー内容が入ってる
	static physx::PxDefaultErrorCallback m_defaultErrorCallback;
	// 上位レベルのSDK(PxPhysicsなど)をインスタンス化する際に必要
	static physx::PxFoundation* m_pFoundation;
	// 実際に物理演算を行う
	static physx::PxPhysics* m_pPhysics;
	// シミュレーションをどう処理するかの設定でマルチスレッドの設定もできる
	static physx::PxDefaultCpuDispatcher* m_pDispatcher;
	// PVDと通信する際に必要
	static physx::PxPvd* m_pPvd;
	static std::vector<physx::PxScene*> scenes;
public:


	static void Init();

	static void Exit();

	static physx::PxScene* AddScene();
	static void ReleaseScene(physx::PxScene* scene_);
	inline static physx::PxPhysics* GetPhysicsInstance() { return m_pPhysics; }
};

USING_PTR(Collider);
class HitInfo {
public:
	ColliderP collision = nullptr;
	ColliderP hit_collision = nullptr;
};

class HitCallBack : public physx::PxSimulationEventCallback {

	void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) override;
	void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) override;

	// 使わない関数は空実装
	void onConstraintBreak(physx::PxConstraintInfo*, physx::PxU32) override {}
	void onWake(physx::PxActor**, physx::PxU32) override {}
	void onSleep(physx::PxActor**, physx::PxU32) override {}
	void onAdvance(const physx::PxRigidBody* const*, const physx::PxTransform*, physx::PxU32) override {}
};


struct Material {
	static physx::PxMaterial* Default;
	static physx::PxMaterial* Metal;
	static physx::PxMaterial* Rubber;
	static physx::PxMaterial* Wood;
	static physx::PxMaterial* Plastic;
	static physx::PxMaterial* Glass;
	static physx::PxMaterial* Concrete;
	static physx::PxMaterial* Asphalt;
	static physx::PxMaterial* Wool;
	static physx::PxMaterial* Paper;
};
