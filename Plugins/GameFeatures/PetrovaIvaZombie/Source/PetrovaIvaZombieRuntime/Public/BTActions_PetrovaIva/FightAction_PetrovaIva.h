#pragma once

#include "BehaviorTree_PetrovaIva.h"
#include "SteeringBehaviors_PetrovaIva.h"

class UInventoryComponent;

namespace GameAI::BT
{
	class FightAction_PetrovaIva : public Action
	{
	public:
		explicit FightAction_PetrovaIva(float EngageRange = 600.f);

		virtual void OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual ENodeStatus Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual void OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;

	private:
		float m_EngageRange{};
		AActor* m_pCachedZombie{ nullptr };
		float m_SearchTimer{};
		static constexpr float SEARCH_INTERVAL{ 0.2f };

		bool m_HasActiveMoveRequest{ false };
		FVector m_LastPathTarget{ FVector::ZeroVector };

		// Pursuit closes the distance to where the zombie will be
		Pursuit m_PursuitSteering{};

		// Face rotates the pawn to aim at the target before firing
		Face m_FaceSteering{};

		int FindWeaponSlot(UInventoryComponent* pInventory) const;
	};
}