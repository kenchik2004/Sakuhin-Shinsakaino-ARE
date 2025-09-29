#include "System/Components/Text.h"



std::vector<CharToken> parseTextWithTags(const std::string& input, Color def_color) {
	std::vector<CharToken> tokens;
	tokens.reserve(500);
	Color current_color = def_color;
	size_t i = 0;

	while (i < input.size()) {
		if (input.substr(i, 2) == reinterpret_cast<const char*>(u8"<c")) {
			i += 2;
			std::string color;

			// 読み取り（色名は">>,"まで）
			while (i < input.size() && !(input.substr(i, 1) == reinterpret_cast<const char*>(u8">"))) {
				color += input[i++];
			}

			if (i + 1 <= input.size()) {
				if (color.empty()) {
					current_color = def_color;
					i++; // skip "c>"
					continue;
				}
				constexpr float cc = 1.0f / 255;
				current_color.r = (static_cast<int>(color[0]) - 48) * 0.1f;
				current_color.g = (static_cast<int>(color[1]) - 48) * 0.1f;
				current_color.b = (static_cast<int>(color[2]) - 48) * 0.1f;
				i++; // skip "c>"
			}
			continue;
		}

		// UTF-8 文字を1文字ずつ取得
		unsigned char c = static_cast<unsigned char>(input[i]);
		if (input.length() > i + 1)
			if (input[i] == '\\' && input[i + 1] == 'n') {
				c = '\n';
				CharToken char_token;
				char_token.utf8_char += c;
				char_token.color = current_color;
				tokens.push_back(char_token);
				i += 2;
				continue;
			}
		size_t char_len = 1;
		if ((c & 0x80) == 0x00) char_len = 1;
		else if ((c & 0xE0) == 0xC0) char_len = 2;
		else if ((c & 0xF0) == 0xE0) char_len = 3;
		else if ((c & 0xF8) == 0xF0) char_len = 4;

		tokens.push_back(CharToken{
			input.substr(i, char_len),
			current_color
			});

		i += char_len;
	}

	return tokens;
}


void Text::DrawAutoString(std::string str_, Vector3 draw_box_pos, Vector3 draw_box_size, float& draw_char_num, Color def_color = Color::WHITE, float speed = 100)
{
	int          font_size = GetFontSize();
	int          x = static_cast<int>(draw_box_pos.x);
	int          y = static_cast<int>(draw_box_pos.y);
	int          max_x = static_cast<int>(draw_box_pos.x + draw_box_size.x - font_size);
	int          max_y = static_cast<int>(draw_box_pos.y + draw_box_size.y - font_size);
	Color color = def_color;
	switch (GetUseCharCodeFormat()) {
	case DX_CHARCODEFORMAT_UTF8: {
		if (draw_char_num >= tokens.size())
			draw_char_num = static_cast<float>(tokens.size());
		for (size_t i = 0; i < draw_char_num; ++i) {
			const auto& token = tokens[i];

			if (token.utf8_char.c_str()[0] == '\n') {
				x = (int)draw_box_pos.x;
				y += font_size;
				continue;
			}
			if (x + font_size >= max_x) {
				x = (int)draw_box_pos.x;
				y += font_size;
			}
			if (y >= max_y)
				break;
			int wid;

			ChangeFont(ShiftJISToUTF8(font).c_str());
			wid = GetDrawFormatStringWidth(token.utf8_char.c_str());
			DrawFormatString(x, y, token.color, token.utf8_char.c_str());
			ChangeFont("");

			x += wid; // または固定幅

		}
		draw_char_num += Time::DrawDeltaTime() * speed;
		SetFontSize(DEFAULT_FONT_SIZE);
		return;
	}
	case DX_CHARCODEFORMAT_SHIFTJIS: {
		for (int i = 0; str_[i] != '\0' && i < (int)draw_char_num; i++) {
			if (str_[i] == '<' && str_[i + 1] != 'c') {
				char code[] = { str_[i + 1], str_[i + 2], str_[i + 3] };
				i += 4;
				int r, g, b;
				r = (code[0] - 48);
				g = (code[1] - 48);
				b = (code[2] - 48);
				color = GetColor(28 * r, 28 * g, 28 * b);
			}
			if (str_[i] == '<' && str_[i + 1] == '>') {
				color = def_color;
				i++;
				continue;
			}
			if (y > max_y || i >= str_.size())
				break;
			if (GetCharBytes(DX_CHARCODEFORMAT_SHIFTJIS, &str_[i]) == 2) {
				DrawFormatString(x, y, color, reinterpret_cast<const char*>(u8"%c%c"), str_[i], str_[i + 1]);
				i++;
				x += font_size;
			}
			else {
				DrawFormatString(x, y, color, "%c", str_[i]);
				x += font_size * 0.5f;
			}
			if (x >= max_x || str_[i] == '\n' || (str_[i] == '\\' && str_[i + 1] == 'n')) {
				x = (int)draw_box_pos.x;
				y += font_size;
			}
		}
		SetFontSize(DEFAULT_FONT_SIZE);
		if ((int)draw_char_num < str_.size())
			draw_char_num += Time::DrawDeltaTime() * speed;
	}
	}
}



int Text::Init()
{
	SetPriority(3);
	return Super::Init();
}

void Text::Update()
{

	if (owner->status.Type() != ObjStat::UI)
		RemoveThisComponent();
}



void Text::LateDraw()
{
	SetFontSize(font_size);
	auto owner_ = SafeStaticCast<UIObject>(owner.lock());
	int  div_x, div_y;
	GetDrawStringSize(&div_x, &div_y, 0, text.c_str(), text.size());
	Vector3 div = { static_cast<float>(div_x), static_cast<float>(div_y), 0.0f };
	Vector3 draw_pos = owner_->GetDrawPos();
	draw_pos += text_box_pos;
	Vector3 scale = owner_->transform->scale;
	scale += text_box_size;

	SetDrawBlendMode(DX_BLENDMODE_ALPHA, txt_color.a * 255);
	switch (alignment) {
	case Text::LEFT:
		draw_pos.y += scale.y * 0.5f - div.y * 0.5f;
		DrawStringF(draw_pos.x, draw_pos.y, text.c_str(), txt_color);
		break;
	case Text::MIDDLE:
		draw_pos += scale * 0.5f - div * 0.5f;
		DrawStringF(draw_pos.x, draw_pos.y, text.c_str(), txt_color);
		break;
	case Text::RIGHT:
		draw_pos.y += scale.y * 0.5f - div.y * 0.5f;
		DrawStringF(draw_pos.x + scale.x - div.x, draw_pos.y, text.c_str(), txt_color);

		break;
	case AUTO:
		DrawAutoString(text, draw_pos, scale, draw_char_num, txt_color, text_speed);
		break;
	}

	SetFontSize(DEFAULT_FONT_SIZE);
	SetDrawBlendMode(DX_BLENDMODE_NOBLEND, 0);
}

void Text::SetText(std::string new_text) {
	text = new_text;
	tokens = parseTextWithTags(text, txt_color);
}

