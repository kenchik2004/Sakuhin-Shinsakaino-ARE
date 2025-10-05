#pragma once

constexpr float PI = 3.1415926535f;		//円周率
constexpr float RADIAN = PI / 180;			//円周率/180(Euler->Radian)
constexpr float EULER = 180 / PI;			//180/円周率(Radian->Euler)
#define DEG2RAD(deg) deg*RADIAN		//オイラー角->ラジアン角の変換
#define RAD2DEG(rad) rad*EULER		//ラジアン角->オイラー角の変換


#define NON_COPYABLE(CLASS)										\
					CLASS(const CLASS& that)=delete;			\
					void operator=(const CLASS& that)=delete;	\


#define NON_CREATABLE(CLASS) CLASS()=delete; 


#define SINGLETON(CLASS)															\
					CLASS(){};														\
					NON_COPYABLE;													\
					static CLASS* instance(){static CLASS class_; return &class_;}	\

//スマートポインタを使用するためのマクロ
#define USING_PTR(CLASS)                                        \
    class CLASS;                                                \
    using CLASS##P          = SafeSharedPtr<CLASS>;           \
    using CLASS##WP      = SafeWeakPtr<CLASS>;                \
    using CLASS##UP    = SafeUniquePtr<CLASS>;                \
    using CLASS##PVec       = std::vector<CLASS##P>;            \
    using CLASS##WPVec   = std::vector<CLASS##WP>;              \
    using CLASS##UPVec = std::vector<CLASS##UP>;                \
    using CLASS##Vec          = std::vector<CLASS*>;			\



using Vector2 = physx::PxVec2;
using Vector3 = physx::PxVec3;
using Vector4 = physx::PxVec4;
using Quaternion = physx::PxQuat;
using mat3x3 = physx::PxMat33;
using mat4x4 = physx::PxMat44;

#include "System/Color.h"

template <class T>
class SafeUniquePtr {
	std::unique_ptr<T> u_p = nullptr;

public:
	SafeUniquePtr() = default;
	~SafeUniquePtr() { u_p.reset(); }
	SafeUniquePtr(std::nullptr_t) : u_p(nullptr) {}

	SafeUniquePtr(std::unique_ptr<T>&& p) noexcept : u_p(std::move(p)) {}

	SafeUniquePtr(SafeUniquePtr&& other) noexcept = default;
	SafeUniquePtr& operator=(SafeUniquePtr&& other) noexcept = default;

	SafeUniquePtr(const SafeUniquePtr&) = delete;
	SafeUniquePtr& operator=(const SafeUniquePtr&) = delete;

	// 基底クラスへの暗黙変換コンストラクタ
	template <typename U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
	SafeUniquePtr(const SafeUniquePtr<U>& other)
		: u_p(std::move(other.raw_unique())) {
	}

	T* operator->() const {
#ifndef PACKAGE_BUILD
		if (!u_p) {
			throw NullptrException("もう知らん!ぬるぽ!");
		}
#endif
		return u_p.get();
	}

	T& operator*() const {
#ifndef PACKAGE_BUILD
		if (!u_p) {
			throw NullptrException("もう知らん!ぬるぽ!");
		}
#endif
		return *u_p;
	}
	operator bool() const { return u_p != nullptr; }
	T* release() { return u_p.release(); }
	void reset(T* ptr = nullptr) { u_p.reset(ptr); }
	void swap(std::unique_ptr<T>& other) { u_p.swap(other); }
	void swap(SafeUniquePtr<T>& other) { u_p.swap(other.raw_unique()); }

	const std::unique_ptr<T>& raw_unique() const { return u_p; }
	std::unique_ptr<T>& raw_unique() { return u_p; }
};

template <class T>
class SafeSharedPtr {
	std::shared_ptr<T> s_p = nullptr;

public:
	SafeSharedPtr() = default;
	~SafeSharedPtr() { s_p.reset(); }
	SafeSharedPtr(const std::shared_ptr<T>& p) : s_p(p) {}
	SafeSharedPtr(std::nullptr_t) : s_p(nullptr) {}

	// 基底クラスへの暗黙変換コンストラクタ
	template <typename U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
	SafeSharedPtr(const SafeSharedPtr<U>& other)
		: s_p(other.raw_shared()) {
	}

	// T = void の場合、この関数はインスタンス化されない
	template <typename U = T>
	typename std::enable_if<!std::is_void<U>::value, U&>::type
		operator*() const {
#ifndef PACKAGE_BUILD
		if (!s_p) {
			throw NullptrException("もう知らん!ぬるぽ!");
		}
#endif
		return *s_p;
	}

	// void以外でも -> が必要ならこちらも必要
	template <typename U = T>
	typename std::enable_if<!std::is_void<U>::value, U*>::type
		operator->() const {
#ifndef PACKAGE_BUILD
		if (!s_p) {
			throw NullptrException("もう知らん!ぬるぽ!");
		}
#endif
		return s_p.get();
	}

	explicit operator bool() const { return s_p.get() != nullptr; }
	bool operator==(std::nullptr_t) const { return !s_p; }
	bool operator!=(std::nullptr_t) const { return s_p != nullptr; }

	template<typename U>
	bool operator==(const SafeSharedPtr<U>& other) const {
		return s_p == other.raw_shared();
	}

	template<typename U>
	bool operator!=(const SafeSharedPtr<U>& other) const {
		return s_p != other.raw_shared();
	}

	void swap(std::shared_ptr<T>& other) { s_p.swap(other); }
	void swap(SafeSharedPtr<T>& other) { s_p.swap(other.raw_shared()); }
	void reset() { s_p.reset(); }
	long use_count() { return s_p.use_count(); }


	const std::shared_ptr<T>& raw_shared() const { return s_p; }
	std::shared_ptr<T>& raw_shared() { return s_p; }
};

template <class T, class... Args>
class SafeWeakPtr {

	std::weak_ptr<T> w_p;
public:
	SafeWeakPtr() = default;
	~SafeWeakPtr() { w_p.reset(); }
	SafeWeakPtr(const std::shared_ptr<T>& p) : w_p(p) {}
	SafeWeakPtr(SafeSharedPtr<T> p) : w_p(p.raw_shared()) {}
	SafeWeakPtr(nullptr_t) { w_p.reset(); }
	void operator=(SafeSharedPtr<T> p) { w_p = p.raw_shared(); }
	void operator=(SafeWeakPtr<T> p) { w_p = p.raw_weak(); }
	void operator=(nullptr_t) { w_p.reset(); }


	// 基底クラスへの暗黙変換コンストラクタ
	template <typename U, std::enable_if_t<std::is_convertible_v<U*, T*>, int> = 0>
	SafeWeakPtr(const SafeWeakPtr<U>& other)
		: w_p(other.raw_weak()) {
	}

	std::shared_ptr<T> operator->() const {
#ifndef PACKAGE_BUILD
		if (w_p.expired())
			throw NullptrException("もう知らん!ぬるぽ!");
#endif
		auto lock = w_p.lock();
		return lock;
	}

	T& operator*() const {

#ifndef PACKAGE_BUILD
		if (w_p.expired()) {
			throw NullptrException("もう知らん!ぬるぽ!");
		}
#endif
		return *(w_p.lock());
	}
	explicit operator bool() const { return !w_p.expired(); }

	bool operator==(std::nullptr_t) const { return !w_p.expired(); }
	bool operator!=(std::nullptr_t) const { return w_p.expired(); }

	template<typename U>
	bool operator==(const SafeWeakPtr<U>& other) const {
		return w_p.lock() == other.raw_weak().lock();
	}

	template<typename U>
	bool operator!=(const SafeWeakPtr<U>& other) const {
		return w_p.lock() != other.raw_weak().lock();
	}

	SafeSharedPtr<T> lock() const { return SafeSharedPtr<T>(w_p.lock()); }
	bool expired() { return w_p.expired(); }
	void reset() { w_p.reset(); }
	void swap(std::weak_ptr<T> other) { w_p.swap(other); }
	void swap(SafeWeakPtr<T> other) { w_p.swap(other.raw_weak()); }
	long use_count() { return w_p.use_count(); }


	const std::weak_ptr<T>& raw_weak() const { return w_p; }
	std::weak_ptr<T>& raw_weak() { return w_p; }
};


template <typename T, typename... Args>
SafeSharedPtr<T> make_safe_shared(Args&&... args) {
	return SafeSharedPtr<T>(std::make_shared<T>(std::forward<Args>(args)...));
}

template <typename T, typename... Args>
SafeUniquePtr<T> make_safe_unique(Args&&... args) {
	return SafeUniquePtr<T>(std::make_unique<T>(std::forward<Args>(args)...));
}




template<typename To, typename From>
SafeSharedPtr<To> SafeDynamicCast(const SafeSharedPtr<From>& from) {
	return SafeSharedPtr<To>(std::dynamic_pointer_cast<To>(from.raw_shared()));
}
template<typename To, typename From>
SafeSharedPtr<To> SafeStaticCast(const SafeSharedPtr<From>& from) {
	return SafeSharedPtr<To>(std::static_pointer_cast<To>(from.raw_shared()));
}

#if 1
class TypeInfo {
public:
	TypeInfo(std::string_view name, size_t size, TypeInfo* parent_info) {

		child = nullptr;
		sibling = nullptr;
		// パラメーターの保存
		auto ite = name.find("class");
		if (ite != name.npos)
			class_name = &name.data()[6];
		else if (ite = name.find("struct"); ite != class_name.npos)
			class_name = &name.data()[7];
		else
			class_name = name;
		class_size = size;

		//----------------------------------------------------------
		// 継承ツリー構造の構築
		//----------------------------------------------------------
		if (parent_info == nullptr && strcmp(class_name.c_str(), "root")) {    // "root"ではない
			parent_info = &TypeInfo::Root();                            // 基底クラスはルートに接続する
		}

		parent = parent_info;
		if (parent_info) {
			auto child = parent_info->Child();
			// 親クラスの子があった場合は追加登録、子が一つもない場合は新規登録
			if (child) {
				sibling = child;
			}
			parent_info->child = this;
		}
	}
	//  クラス名を取得
	std::string_view ClassName() const;
	//  クラスのサイズを取得
	size_t ClassSize() const;
	//  親ノードを取得
//! @return ノードへのポインタ (存在しない場合はnullptr)
	const TypeInfo* Parent() const;
	const TypeInfo* Child() const;
	const TypeInfo* Sibling() const;
	static inline TypeInfo& Root() {

		static TypeInfo root("root", sizeof(TypeInfo), nullptr);
		return root;

	}
private:
	std::string class_name;
	size_t class_size = 0;
	const TypeInfo* parent;
	const TypeInfo* child;
	const TypeInfo* sibling;
public:
	virtual SafeSharedPtr<void> Create() const { return nullptr; }
};
#endif


//! インスタンス作成クラス
template <typename T, bool is_abstract = std::is_abstract_v<T>, bool is_default_constractible = std::is_default_constructible_v<T>>
class CreateInstance
{
public:
	static SafeSharedPtr<void> create() { return SafeStaticCast<void>(make_safe_shared<T>()); }
};

//! 抽象クラスの場合はnewできないためnullptrを返す特殊化  
//! 非デフォルト構築不可の場合はnewできないためnullptrを返す特殊化  
template <typename T>
class CreateInstance<T, false, false>
{
public:
	static SafeSharedPtr<void> create() { return SafeSharedPtr<void>(nullptr); }

};
template <typename T>
class CreateInstance<T, true, true>
{
public:
	static SafeSharedPtr<void> create() { return SafeSharedPtr<void>(nullptr); }

};
//! どっちもの場合はそもそも論外。nullptrを返す特殊化  
template <typename T>
class CreateInstance<T, true, false>
{
public:
	static SafeSharedPtr<void> create() { return SafeSharedPtr<void>(nullptr); }

};


template <class T>
class ClassTypeInfo :public TypeInfo {
public:
	ClassTypeInfo(const char* name, TypeInfo* parent_info = nullptr)
		:TypeInfo(name, sizeof(T), parent_info)
	{
	}

	//  インスタンスを作成(クラスをnewしてポインタを返す)
	SafeSharedPtr<void> Create() const override { return SafeStaticCast<void>(CreateInstance<T>::create()); }
};
class Class {
public:
	static inline TypeInfo& info = TypeInfo::Root();
	inline virtual TypeInfo* Info() { return static_cast<TypeInfo*>(&info); }
};



#if 1
#define USING_SUPER(CLASS)																					\
		using Super = Class;																				\
		using Class = CLASS;																				\
		static inline const char* Name() {return typeid(CLASS).name();};											\
		static inline ClassTypeInfo<CLASS> info=ClassTypeInfo<CLASS>(Name(),&Super::info);							\
		inline virtual TypeInfo* Info() {return static_cast<TypeInfo*>(&info);};									\

#endif


using s8 = int8_t;      //!<  8bit 符号あり整数
using s16 = int16_t;     //!< 16bit 符号あり整数
using s32 = int32_t;     //!< 32bit 符号あり整数
using s64 = int64_t;     //!< 64bit 符号あり整数
using u8 = uint8_t;     //!<  8bit 符号なし整数
using u16 = uint16_t;    //!< 16bit 符号なし整数
using u32 = uint32_t;    //!< 32bit 符号なし整数
using u64 = uint64_t;    //!< 64bit 符号なし整数
using f32 = float;       //!< 単精度浮動小数点数
using f64 = double;      //!< 倍精度浮動小数点数


//ステータス管理用ビット演算テンプレート
template <class T, class V = u32> struct SBit {
	enum struct Bits : V
	{
	};

	inline V on(T b)																	//ビットのオン
	{
		status_bit_ |= 1ui64 << static_cast<V>(b);
		return status_bit_;
	}
	inline V off(T b)																	//ビットのオフ
	{
		status_bit_ &= ~(1ui64 << static_cast<V>(b));
		return status_bit_;
	}
	inline V  set(T b, bool _on) { return _on ? on(b) : off(b); }						//1ビットのみのセット
	inline V  set(T b) { status_bit_ = b; return status_bit_; }							//全ビットのセット
	inline V  is(T b) const { return status_bit_ & (1ui64 << static_cast<int>(b)); }	//ビットの正否を返す
	inline V& get() { return status_bit_; }												//全ビットの情報を返す

private:
	V status_bit_ = 0;
};

std::wstring Str2Wstr(std::string in);			//通常stringをワイドstringに変換
std::string WStr2Str(std::wstring in);			//ワイドstringを通常stringに変換

std::wstring Str2WstrU8(std::string in);


std::string WStr2StrU8(std::wstring in);

std::string ShiftJISToUTF8(const std::string& shiftJisStr);

//(旧仕様ではnewしていたので無視しちゃダメだったが、shared_ptr管理にしたことで無視してもよくなったのでは...?)
[[nodiscard]] SafeSharedPtr<void> CreateInstanceFromName(std::string_view name, TypeInfo& type = TypeInfo::Root());

template<class T>
[[nodiscard]] SafeSharedPtr<T> CreateInstanceFromName(std::string_view name) { return SafeStaticCast<T>(CreateInstanceFromName(name, T::info)); }
