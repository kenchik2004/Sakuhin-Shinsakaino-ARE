#pragma once


enum struct KeyCode : unsigned char {
	Escape = (0x01),
	Alpha1 = (0x02),
	Alpha2 = (0x03),
	Alpha3 = (0x04),
	Alpha4 = (0x05),
	Alpha5 = (0x06),
	Alpha6 = (0x07),
	Alpha7 = (0x08),
	Alpha8 = (0x09),
	Alpha9 = (0x0A),
	Alpha0 = (0x0B),
	Minus = (0x0C),
	Tab = (0x0F),
	Q = (0x10),
	W = (0x11),
	E = (0x12),
	R = (0x13),
	T = (0x14),
	Y = (0x15),
	U = (0x16),
	I = (0x17),
	O = (0x18),
	P = (0x19),
	LBracket = (0x1A),
	RBracket = (0x1B),
	Return = (0x1C),
	LControl = (0x1D),
	A = (0x1E),
	S = (0x1F),
	D = (0x20),
	F = (0x21),
	G = (0x22),
	H = (0x23),
	J = (0x24),
	K = (0x25),
	L = (0x26),
	Semicolon = (0x27),
	Z = (0x2C),
	X = (0x2D),
	C = (0x2E),
	V = (0x2F),
	B = (0x30),
	N = (0x31),
	M = (0x32),
	Comma = (0x33),
	Period = (0x34),
	Slash = (0x35),
	LShift = (0x2A),
	RShift = (0x36),
	Multipuly = (0x37),
	LAlt = (0x38),
	Space = (0x39),
	CapsLock = (0x3A),
	F1 = (0x3B),
	F2 = (0x3C),
	F3 = (0x3D),
	F4 = (0x3E),
	F5 = (0x3F),
	F6 = (0x40),
	F7 = (0x41),
	F8 = (0x42),
	F9 = (0x43),
	F10 = (0x44),
	ScreenLock = (0x46),
	BackSlash = (0x2B),
	RControl = (0x9D),
	RAlt = (0xB8),
	Home = (0xC7),
	PageUp = (0xC9),
	End = (0xCF),
	Up = (0xC8),
	Down = (0xD0),
	Left = (0xCB),
	Right = (0xCD),
	PageDown = (0xD1),
	Insert = (0xD2),
	Delete = (0xD3),
	F11 = (0x57),
	F12 = (0x58),
	Tilde = (0x90),
	At = (0x91),
	Colon = (0x92),
};
enum struct MouseButton :unsigned char {
	ButtonLeft = 0x0001,
	ButtonRight = 0x0002,
	ButtonMiddle = 0x0004,
	Button4 = 0x0008,
	Button5 = 0x0010,
	Button6 = 0x0020,
	Button7 = 0x0040,
	Button8 = 0x0080,
};
namespace Input
{
	void Init();
	void Update();
	bool GetKeyDown(KeyCode key);
	bool GetKey(KeyCode key);
	bool GetKeyUp(KeyCode key);
	bool GetMouseButtonDown(MouseButton button);
	bool GetMouseButtonRepeat(MouseButton button);
	bool GetMouseButtonUp(MouseButton button);
	Vector2 GetMousePosition();
	void SetMousePosition(Vector2 new_pos, bool reset_delta = true);
	Vector2 GetMouseDelta();
};

