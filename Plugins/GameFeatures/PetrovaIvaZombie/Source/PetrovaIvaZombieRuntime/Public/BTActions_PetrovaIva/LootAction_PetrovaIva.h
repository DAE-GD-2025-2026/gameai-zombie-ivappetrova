#pragma once

#include "BehaviorTree_PetrovaIva.h"
#include "SteeringBehaviors_PetrovaIva.h"
#include "Items/BaseItem.h"

class UInventoryComponent;
class UStudentPerceptor_PetrovaIva;

namespace GameAI::BT
{
	class LootAction_PetrovaIva : public Action
	{
	public:
		explicit LootAction_PetrovaIva(UStudentPerceptor_PetrovaIva* Perceptor = nullptr);

		virtual void OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual ENodeStatus Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual void OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;

	private:
		UStudentPerceptor_PetrovaIva* m_pPerceptor{ nullptr };
		ABaseItem* m_pTargetItem{ nullptr };
		Arrive m_ArriveSteering{};

		TArray<ABaseItem*> m_BlacklistedItems{};

		float m_StuckTimer{ 0.f };
		static constexpr float MAX_LOOT_TIME{ 5.f };

		int FindEmptySlot(UInventoryComponent* Inventory) const;
	};
}