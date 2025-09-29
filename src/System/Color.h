#pragma once
class Color final
{
public:
	float r;
	float g;
	float b;
	float a;
	Color() :r(0), g(0), b(0), a(1) {}
	Color(const unsigned int uint_color) {
		r = float((uint_color & 0xff0000) >> 16) / 255; g = float((uint_color & 0x00ff00) >> 8) / 255; b = float(uint_color & 0x0000ff) / 255; a = 1.0f;
	}
	Color(const Color& other) { r = other.r; g = other.g; b = other.b; a = other.a; }
	Color(const Vector4& other) { r = other.x; g = other.y; b = other.z; a = other.w; }
	Color(float red, float green, float blue, float alpha = 1.0f) { r = red; g = green; b = blue; a = alpha; }
	operator Vector4() const { return Vector4(r, g, b, a); }
	operator COLOR_F() const { COLOR_F dx_color; dx_color.r = r; dx_color.g = g; dx_color.b = b; dx_color.a = a; return dx_color; }
	operator unsigned int() const {
		unsigned int color;
		color = (std::clamp((unsigned int)(r * 255), (unsigned int)0, (unsigned int)255)) << 16;
		color |= (std::clamp((unsigned int)(g * 255), (unsigned int)0, (unsigned int)255)) << 8;
		color |= (std::clamp((unsigned int)(b * 255), (unsigned int)0, (unsigned int)255));
		return color;
	}
	//各色のGetColorやGetColorFを使用せずに扱うための定数
	static const Color BLACK;		//黒
	static const Color WHITE;		//白
	static const Color RED;			//赤
	static const Color GREEN;		//緑
	static const Color BLUE;		//青
	static const Color YELLOW;		//黄
	static const Color CYAN;		//シアン
	static const Color MAGENTA;		//マゼンタ
	static const Color GRAY;		//灰
	static const Color DARK_RED;	//ダークレッド
	static const Color DARK_GREEN;	//ダークグリーン
	static const Color DARK_BLUE;	//ダークブルー
	static const Color ORANGE;		//オレンジ
	static const Color PINK;		//ピンク
	static const Color BROWN;		//茶
	static const Color PURPLE;		//紫
};


