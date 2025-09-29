#include "precompile.h"
#include "CardDoman.h"

namespace {
	SafeSharedPtr<Texture> mytex = nullptr;

}
namespace RLyeh {

	int CardDoman::Init()
	{
		if (!mytex) {
			TextureManager::Load(reinterpret_cast<const char*>(u8"data/カード/CardTexture_Doman.png"), "card_doman");
			mytex = TextureManager::CloneByName("card_doman");
		}
		my_texture = mytex;
		Super::Init();
		return 0;
	}
	void CardDoman::Exit()
	{
		mytex.reset();
		Super::Exit();
	}
}
