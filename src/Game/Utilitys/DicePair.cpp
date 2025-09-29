#include "precompile.h"
#include "DicePair.h"
#include "Game/Objects/RLyeh/Dices/DiceBase.h"
#include "System/Components/RigidBody.h"


namespace RLyeh {

	//1d100とかは10面ダイス2つで一つの結果が欲しかったりするので、ダイスのペアを作る
	DicePair::DicePair(MODE mode_, DiceBaseP x1, DiceBaseP x2)
	{
		mode = mode_;
		pair[0] = x1;
		pair[1] = x2;
	}

	int DicePair::Result()
	{
		if (!pair[0])
			return 0;
		for (int i = 0; i < 2; i++) {
			//ちょろちょろ動かれると厄介なので、物理演算を止める
			if (pair[i]) {
				if (auto rb = pair[i]->GetComponent<RigidBody>()) {
					rb->is_kinematic = true;
				}
				pair[i]->FetchResult();
			}
		}
		switch (mode) {
		case D4:
		case D6:
		case D8:
		case D10:
		case D12:
		case D20:
			return pair[0]->selected_number + 1;
			break;
		case D100:
			//これは問題なので、とりあえずD10と同じ挙動をさせる
			if (!pair[1])
				return pair[0]->selected_number + 1;
			//2つのうち1つを十の位,もう一つを一の位として扱う
			int number = 0;
			number += (pair[0]->selected_number + 1) * 10;
			number += (pair[1]->selected_number + 1) % 10;
			//ここでできた数値は10~109なので、100で割った余りで一旦0~99とする
			number %= 100;
			//0,0の組み合わせのみ100と扱う
			if (number == 0)
				number = 100;
			return number;
			break;
		}
		return 0;
	}

}