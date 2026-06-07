#pragma once

#include "BehaviorTree_PetrovaIva.h"
#include "SteeringBehaviors_PetrovaIva.h"
#include "Village/House/House.h"

namespace GameAI::BT
{
	class ExploreAction_PetrovaIva : public Action
	{
	public:
		virtual void OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual ENodeStatus Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual void OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;

	private:
		AHouse* m_TargetHouse{ nullptr };
		TArray<AHouse*> m_VisitedHouses{};

		// Seek steers toward a specific house target
		Seek m_SeekSteering{};
		// Wander is used when unstuck-ing
		Wander m_WanderSteering{};

		FVector m_LastPosition{ FVector::ZeroVector };
		float m_StuckTimer{};
		bool m_IsUnstucking{ false };
		float m_UnstuckTimer{};

		static constexpr float STUCK_TIME_THRESHOLD {1.5f};
		static constexpr float STUCK_DIST_THRESHOLD {50.f};
		static constexpr float UNSTUCK_DURATION {1.5f};

		AHouse* FindClosestUnvisitedHouse(ASurvivorPawn& Survivor) const;
		bool IsInsideHouse(ASurvivorPawn& Survivor, AHouse* House) const;
	};
}