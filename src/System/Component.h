#pragma once

struct CompStat {

	friend class Component;
	friend class Object;
	enum class STATUS :u32 {
		CONSTRUCTED = 0,
		INITIALIZED = 1,
		ACTIVE = 1 << 1,
		DRAW = 1 << 2,
		REMOVED = 1 << 3,
		SINGLE = 1 << 4,
	};
	SBit<STATUS> status_bit;
private:
	unsigned int priority = 0;
	std::string class_name;
};
USING_PTR(Object);
class Component :public std::enable_shared_from_this<Component>
{
	friend class Object;
private:
public:

	virtual ~Component() {}

	USING_SUPER(Component);
	CompStat status;

	ObjectWP owner;

	inline void ConstructBase() {
		//こっちは必ずやる
		status.status_bit.on(CompStat::STATUS::CONSTRUCTED);
		status.status_bit.on(CompStat::STATUS::INITIALIZED);
		status.status_bit.on(CompStat::STATUS::ACTIVE);
		status.status_bit.on(CompStat::STATUS::DRAW);
		status.class_name = info.ClassName();
		//こっちはユーザーがカスタマイズできる
		Construct();
	}
	inline virtual void Construct() {};
	void SetPriority(unsigned int prio);	
	inline unsigned int GetPriority() { return status.priority; }
	void RemoveThisComponent();
	//-----------------------------
	// Initブロック(初期化処理)
	//-----------------------------
	inline virtual int Init() { return 0; }
	//-----------------------------

	//-----------------------------
	// Updateブロック(更新前後処理)
	//-----------------------------
	inline virtual void PreUpdate() {}
	inline virtual void Update() {}
	inline virtual void LateUpdate() {}
	inline virtual void PostUpdate() {}
	//-----------------------------

	//-----------------------------
	// Physicsブロック(物理前後処理)
	//-----------------------------
	inline virtual void PrePhysics() {}
	inline virtual void Physics() {}
	inline virtual void PostPhysics() {}
	//-----------------------------

	//-----------------------------
	// Drawブロック(描画前後処理)
	//-----------------------------
	inline virtual void PreDraw() {}
	inline virtual void Draw() {}
	inline virtual void LateDraw() {}
	inline virtual void DebugDraw() {}
	inline virtual void LateDebugDraw() {}

	//ここの処理は描画に次フレームまで反映されない
	inline virtual void PostDraw() {}
	//-----------------------------

	inline virtual void Exit() {}
};

