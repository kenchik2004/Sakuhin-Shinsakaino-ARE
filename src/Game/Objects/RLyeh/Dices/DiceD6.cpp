#include "precompile.h"
#include "DiceD6.h"

namespace RLyeh {

	DiceD6::DiceD6(int texture) :DiceBase(texture)
	{
		model_name = "D6";
		dice_vectors = {

		Vector3(0.0,  0.0f,  1.0f),
		Vector3(0.0f, -1.0f,  0.0f),
		Vector3(1.0f, 0.0f,	0.0f),
		Vector3(-1.0f, 0.0f,  0.0f),
		Vector3(0.0f, 1.0f,0.0f),
		Vector3(0.0f, 0.0f,  -1.0f),
		};
	}
}

