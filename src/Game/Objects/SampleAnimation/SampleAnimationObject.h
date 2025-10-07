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
		void Exit() override;
		void ManipulateAsAnotherPlayer(unsigned int anim_state);
		unsigned int GetCurrentAnimState();
		void LateDraw() override;
		bool manipulate_mode = false;
	private:
		ModelRendererWP my_model = nullptr;
		AnimatorWP my_animator = nullptr;
	};
}

