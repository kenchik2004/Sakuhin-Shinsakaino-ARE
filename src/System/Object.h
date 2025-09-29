#pragma once



struct ObjStat {
	friend class Scene;
	friend class Object;
	friend class GameObject;
	friend class UIObject;
public:
	enum OBJ_TYPE {
		NORMAL,
		UI,
	};
	enum struct STATUS :u32 {
		CONSTRUCTED = 0,
		INITIALIZED = 1,
		ACTIVE = 1 << 1,
		DRAW = 1 << 2,
		REMOVED = 1 << 3,
	};

	SBit <STATUS> status_bit;
	const OBJ_TYPE& Type() const { return obj_type; }
private:
	std::string class_name = "Object";
	OBJ_TYPE obj_type = NORMAL;
	unsigned int priority = 10;
	unsigned int draw_priority = 10;
};

USING_PTR(Component);
USING_PTR(Transform);
USING_PTR(Object);
USING_PTR(Scene);
class Object :public std::enable_shared_from_this<Object>
{
	friend class Scene;
	friend class SceneManager;
public:
	virtual ~Object() {}

	USING_SUPER(Object);
	ObjStat status;
	enum TAG {
		Untaged,
		Player,
		Enemy,
		Stage,
		UI,
		GameManager,

		__TagMax,
	};
	TAG tag = Untaged;
	std::string name = "EmptyObject";
	TransformP transform = nullptr;
private:


	inline void Construct(SceneP owner_scene) {
		status.status_bit.on(ObjStat::STATUS::CONSTRUCTED);
		status.status_bit.on(ObjStat::STATUS::INITIALIZED);
		status.status_bit.on(ObjStat::STATUS::ACTIVE);
		status.status_bit.on(ObjStat::STATUS::DRAW);
		status.class_name = info.ClassName();
		transform = AddComponent<Transform>();
		scene = owner_scene;
	};


	void SyncComponentsPriority();
	bool CheckForSingleComponent(ComponentP comp);


	ComponentPVec components;
	ComponentPVec dirty_priority_components;
	SceneP scene;


public:
	void SetComponentPriority(unsigned int prio, ComponentP who) {
		who->status.priority = prio;
		if (std::find(dirty_priority_components.begin(), dirty_priority_components.end(), who) != dirty_priority_components.end())
			dirty_priority_components.push_back(who);
	}

	template <class T, std::enable_if_t<std::is_convertible_v<T*, Component*>, int> = 0, typename... Args>
	SafeSharedPtr<T> AddComponent(Args&& ...args) {
		auto comp = make_safe_shared<T>(std::forward<Args>(args)...);
		try {
			comp->owner = SafeSharedPtr(shared_from_this());
			comp->ConstructBase();
			if (!CheckForSingleComponent(comp))
				return nullptr;

			dirty_priority_components.push_back(comp);
			components.push_back(comp);
			comp->Init();
		}
		catch (Exception& ex) {
			ex.Show();
		}
		return comp;
	}

	//登録前のコンポーネントであれば、ここからも登録できる(CreateInstanceしたものなど)
	template <class T, std::enable_if_t<std::is_convertible_v<T*, Component*>, int> = 0>
	SafeSharedPtr<T> AddComponentFromPtr(SafeSharedPtr<T> component) {
		auto comp = SafeStaticCast<Component>(component);
		if (comp->status.status_bit.is(CompStat::STATUS::CONSTRUCTED))
			return nullptr;
		try {
			comp->owner = SafeSharedPtr(shared_from_this());
			comp->ConstructBase();
			if (!CheckForSingleComponent(comp))
				return nullptr;

			dirty_priority_components.push_back(comp);
			components.push_back(comp);
			comp->Init();
		}
		catch (Exception& ex) {
			ex.Show();
		}
		return component;
	}

	template <class T, std::enable_if_t<std::is_convertible_v<T*, Component*>, int> = 0>
	SafeSharedPtr<T> GetComponent() {
		for (auto& comp : components) {
			if (!comp->status.status_bit.is(CompStat::STATUS::REMOVED))
				if (auto pick_comp = SafeDynamicCast<T>(comp)) {
					return pick_comp;
				}
		}
		return nullptr;
	}
	template <class T, std::enable_if_t<std::is_convertible_v<T*, Component*>, int> = 0>
	std::vector<SafeSharedPtr<T>> GetComponents() {
		std::vector<SafeSharedPtr<T>> vec(0);
		if (!GetComponent<T>())
			return vec;
		for (auto& comp : components) {
			if (!comp->status.status_bit.is(CompStat::STATUS::REMOVED))
				if (auto pick_comp = SafeDynamicCast<T>(comp)) {
					vec.push_back(pick_comp);
				}
		}
		return vec;
	}

	void RemoveComponent(ComponentP remove_comp) {

		//if (remove_comp == transform)
	//		return;

		ComponentWP comp_wp;
		if (auto comp = std::find(dirty_priority_components.begin(), dirty_priority_components.end(), remove_comp); comp != dirty_priority_components.end())
		{
			dirty_priority_components.erase(comp);
		}
		for (auto comp = components.begin(); comp != components.end();) {
			comp_wp = (*comp);
			if (comp_wp.lock() == remove_comp) {
				comp_wp->Exit();
				comp_wp->status.status_bit.on(CompStat::STATUS::REMOVED);
				comp = components.erase(comp);
				remove_comp.reset();
				break;

			}
			comp++;
		}
	}

	inline void SetPriority(unsigned int prio);
	inline unsigned int GetPriority() { return status.priority; }
	inline SceneP GetScene() { return scene; }


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
	// Hit時コールバック
	//-----------------------------
	inline virtual void OnCollisionEnter(const HitInfo& hit_info) {}
	inline virtual void OnCollisionStay(const HitInfo& hit_info) {}
	inline virtual void OnCollisionExit(const HitInfo& hit_info) {}

	//-----------------------------
	// Hit時コールバック(トリガー)
	//-----------------------------
	inline virtual void OnTriggerEnter(const HitInfo& hit_info) {}
	inline virtual void OnTriggerStay(const HitInfo& hit_info) {}
	inline virtual void OnTriggerExit(const HitInfo& hit_info) {}
	//-----------------------------

	//-----------------------------
	// Drawブロック(描画前後処理)
	//-----------------------------
	inline virtual void PreDraw() {}
	inline virtual void Draw() {}
	inline virtual void LateDraw() {}
	//デバッグ用描画(デバッグウィンドウに描画される)
	inline virtual void DebugDraw() {}
	inline virtual void LateDebugDraw() {}

	//ここの処理は描画に次フレームまで反映されない
	inline virtual void PostDraw() {}
	//-----------------------------

	inline virtual void Exit() {}
};

USING_PTR(GameObject);
class GameObject :public Object {
public:
	USING_SUPER(GameObject);
	GameObject();
	GameObject(std::string name_);
	void DebugDraw() override;


};


USING_PTR(UIObject);
class UIObject : public Object
{
public:
	USING_SUPER(UIObject);

	UIObject();
	int          Init() override;
	void          PreUpdate() override;
	void          Update() override;
	void          PreDraw() override final;
	void          LateDraw() override final;
	virtual unsigned int& BackGroundColor() { return back_ground_color; }
	virtual bool& UseBackGround() { return use_back_color; }

	inline const Vector3& GetDrawPos() { return draw_pos; }
	enum ANCHOR_TYPE
	{
		LEFT_TOP,
		CENTER_TOP,
		RIGHT_TOP,
		LEFT_MIDDLE,
		CENTER,
		RIGHT_MIDDLE,
		LEFT_BOTTOM,
		CENTER_BOTTOM,
		RIGHT_BOTTOM
	};
	inline ANCHOR_TYPE& AnchorType() { return anchor_type; }
	inline ANCHOR_TYPE& CanvasAnchorType() { return canvas_anchor_type; }



protected:
	int          draw_priolity = 0;
	bool         use_back_color = false;
	unsigned int back_ground_color = Color::GRAY;
	Vector3       anchor_point;
	Vector3       canvas_anchor_point;
	Vector3       draw_pos;
	ANCHOR_TYPE  anchor_type = CENTER;
	ANCHOR_TYPE  canvas_anchor_type = CENTER;
};
