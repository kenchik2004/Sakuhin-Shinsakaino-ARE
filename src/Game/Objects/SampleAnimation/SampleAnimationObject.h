#pragma once
#include "System/Object.h"
USING_PTR(ModelRenderer);
USING_PTR(Animator);
namespace SampleAnimation
{

	class SampleAnimationObject :
		public GameObject
	{
	public:
		USING_SUPER(SampleAnimationObject);
		int Init() override;
		void Update() override;
		void DebugDraw() override;
		void Exit() override;
	private:
		ModelRendererWP my_model = nullptr;
		AnimatorWP my_animator = nullptr;
	};
}

