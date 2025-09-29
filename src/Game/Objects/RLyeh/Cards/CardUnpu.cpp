#include "precompile.h"
#include "CardUnpu.h"


namespace {
	SafeSharedPtr<Texture> mytex = nullptr;

}
namespace RLyeh {

	int CardUnpu::Init()
	{
		if (!mytex) {
			TextureManager::Load(reinterpret_cast<const char*>(u8"data/カード/CardTexture_Unpu.png"), "card_unpu");
			mytex = TextureManager::CloneByName("card_unpu");
		}
		my_texture = mytex;
		Super::Init();
		return 0;
	}
	void CardUnpu::Exit()
	{
		mytex.reset();
		Super::Exit();
	}
}
