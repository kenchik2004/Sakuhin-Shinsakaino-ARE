#include "precompile.h"
#include "CardFist.h"
namespace {
	SafeSharedPtr<Texture> mytex = nullptr;

}
namespace RLyeh {

	int CardFist::Init()
	{
		if (!mytex) {
			TextureManager::Load(reinterpret_cast<const char*>(u8"data/カード/CardTexture_Cobushi.png"), "card_fist");
			mytex = TextureManager::CloneByName("card_fist");
		}
		my_texture = mytex;
		Super::Init();
		return 0;
	}
	void CardFist::Exit()
	{
		mytex.reset();
		Super::Exit();
	}
}
