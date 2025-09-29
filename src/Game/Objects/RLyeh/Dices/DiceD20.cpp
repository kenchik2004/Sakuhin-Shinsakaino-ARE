
#include "precompile.h"
#include "DiceD20.h"

namespace  RLyeh {

	DiceD20::DiceD20(int texture) :DiceBase(texture)
	{
		model_name = "D20";
		dice_vectors = {
	Vector3(0.188f,-0.795f,-0.577f),
	Vector3(0.607f,-0.795f,0.000f),
	Vector3(-0.491f,-0.795f,-0.357f),
	Vector3(-0.491f,-0.795f,0.357f),
	Vector3(0.188f,-0.795f,0.577f),
	Vector3(0.982f,-0.188f,0.000f),
	Vector3(0.304f,-0.188f,-0.934f),
	Vector3(-0.795f,-0.188f,-0.577f),
	Vector3(-0.795f,-0.188f,0.577f),
	Vector3(0.304f,-0.188f,0.934f),
	Vector3(0.795f,0.188f,-0.577f),
	Vector3(-0.304f,0.188f,-0.934f),
	Vector3(-0.982f,0.188f,0.000f),
	Vector3(-0.304f,0.188f,0.934f),
	Vector3(0.795f,0.188f,0.577f),
	Vector3(0.491f,0.795f,-0.357f),
	Vector3(-0.188f,0.795f,-0.577f),
	Vector3(-0.607f,0.795f,0.000f),
	Vector3(-0.188f,0.795f,0.577f),
	Vector3(0.491f,0.795f,0.357f)
		};
	}
}

