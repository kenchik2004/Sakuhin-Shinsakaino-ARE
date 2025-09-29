#include "precompile.h"
#include "System/Object.h"
#include "Component.h"



void Component::SetPriority(unsigned int prio)
{
	if (owner)
		owner->SetComponentPriority(prio, shared_from_this());
}


void Component::RemoveThisComponent()
{
	if (owner)
		owner->RemoveComponent(shared_from_this());
}
