#pragma once

#include "BehaviorTree_PetrovaIva.h"
#include "SteeringBehaviors_PetrovaIva.h"

namespace GameAI::BT
{
	class PurgeZoneAction_PetrovaIva : public Action
	{
	public:
		virtual void OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual ENodeStatus Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual void OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;

	private:
		Flee m_FleeSteering{};
	};
}