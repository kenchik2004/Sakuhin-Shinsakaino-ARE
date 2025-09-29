#pragma once
#include "Game/Objects/RLyeh/Cards/CardBase.h"
namespace RLyeh {

class CardFist :
    public CardBase
{
public:
    USING_SUPER(CardFist);
    int Init() override;
    void Exit() override;
};

}
