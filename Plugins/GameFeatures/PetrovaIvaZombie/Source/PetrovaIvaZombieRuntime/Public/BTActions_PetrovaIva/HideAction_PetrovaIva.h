#pragma once

#include "BehaviorTree_PetrovaIva.h"
#include "SteeringBehaviors_PetrovaIva.h"
#include "Village/House/House.h"

class UStudentPerceptor_PetrovaIva;

namespace GameAI::BT
{
	class HideAction_PetrovaIva : public Action
	{
	public:
		explicit HideAction_PetrovaIva(UStudentPerceptor_PetrovaIva* Perceptor = nullptr);

		virtual void OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual ENodeStatus Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual void OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;

	private:
		UStudentPerceptor_PetrovaIva* m_pPerceptor{ nullptr };
		AHouse* m_pTargetHouse{ nullptr };
		Arrive m_ArriveSteering{};

		bool IsInsideHouse(ASurvivorPawn& Survivor, AHouse* House) const;
	};
}