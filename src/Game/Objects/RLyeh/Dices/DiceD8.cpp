#include "precompile.h"
#include "DiceD8.h"

namespace RLyeh {

	DiceD8::DiceD8(int texture) :DiceBase(texture)
	{
		model_name = "D8";
		dice_vectors = {

	Vector3(0.577f,-0.577f,0.577f),
	Vector3(0.577f,0.577f,0.577f),
	Vector3(-0.577f,-0.577f,-0.577f),
	Vector3(-0.577f,-0.577f,0.577f),
	Vector3(0.577f,0.577f,-0.577f),
	Vector3(0.577f,-0.577f,-0.577f),
	Vector3(-0.577f,0.577f,0.577f),
	Vector3(-0.577f,0.577f,-0.577f),

		};
	}

}
