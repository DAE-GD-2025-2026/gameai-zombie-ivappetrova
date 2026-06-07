#pragma once

#include "BehaviorTree_PetrovaIva.h"
#include "Items/ItemType.h"

namespace GameAI::BT
{
	class UseItemAction_PetrovaIva : public Action
	{
	public:
		explicit UseItemAction_PetrovaIva(EItemType TypeToUse);

		virtual ENodeStatus Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;

	private:
		EItemType m_TypeToUse{};
	};
}