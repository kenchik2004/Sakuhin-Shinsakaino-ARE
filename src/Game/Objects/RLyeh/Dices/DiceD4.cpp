#include "precompile.h"
#include "DiceD4.h"

namespace RLyeh {


	DiceD4::DiceD4() :DiceBase()
	{
		model_name = "D4";
		dice_vectors = {

		Vector3(0.0f, 1.0f,0.0f),
		Vector3(0.0f, -0.35f,	1.0f),
		Vector3(0.86f,  -0.35f,  -0.5f),
		Vector3(-0.86f, -0.35f,  -0.5f),
		};
	}

}