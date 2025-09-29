#include "precompile.h"
#include "CardShinsoku.h"
namespace {
	SafeSharedPtr<Texture> mytex = nullptr;

}
namespace RLyeh {

	int CardShinsoku::Init()
	{
		if (!mytex) {
			TextureManager::Load(reinterpret_cast<const char*>(u8"data/カード/CardTexture_Shinsoku.png"), "card_shinsoku");
			mytex = TextureManager::CloneByName("card_shinsoku");
		}
		my_texture = mytex;		
		Super::Init();
		return 0;
	}
	void CardShinsoku::Exit()
	{
		mytex.reset();
		Super::Exit();
	}
}
