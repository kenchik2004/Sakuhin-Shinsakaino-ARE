#pragma once
#include "System/Object.h"
#include "System/Components/ModelRenderer.h"
namespace RLyeh {
	class CardBase :
		public GameObject
	{
	public:
		USING_SUPER(CardBase);
		int Init() override;
		void Update() override;
		void LateUpdate() override;
		void Exit() override;
		void Draw() override;
		void LateDraw() override;
		void SetTexture(SafeSharedPtr<Texture> tex);
		ModelRendererWP model;
		int mode = 0;
		float delta = 0;
		SafeSharedPtr<Texture> my_texture = nullptr;
		Vector3 start_pos;
	};
}

