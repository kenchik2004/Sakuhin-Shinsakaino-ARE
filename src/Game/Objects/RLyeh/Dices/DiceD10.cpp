#include "precompile.h"
#include "DiceD10.h"

namespace RLyeh {

	DiceD10::DiceD10(int texture) :DiceBase(texture)
	{
		model_name = "D10";
		DiceD10::dice_vectors = {
		 Vector3(-0.672f, -0.707f, -0.220f),
	 Vector3(0.673f,  0.707f,  0.217f)	,
	 Vector3(0.417f,  0.707f, -0.571f)	,
	 Vector3(-0.672f,  0.707f,  0.220f),
	 Vector3(0.673f, -0.707f, -0.217f)	,
	 Vector3(-0.416f,  0.707f, -0.572f),
	 Vector3(0.001f,  0.707f,  0.707f)	,
	 Vector3(0.001f, -0.707f, -0.707f)	,
	 Vector3(-0.417f, -0.707f,  0.571f),
	 Vector3(0.415f, -0.707f,  0.573f),

		};
	}
}

