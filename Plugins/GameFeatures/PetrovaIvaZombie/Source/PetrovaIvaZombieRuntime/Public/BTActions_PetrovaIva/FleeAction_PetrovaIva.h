#pragma once

#include "BehaviorTree_PetrovaIva.h"
#include "SteeringBehaviors_PetrovaIva.h"
#include "Zombies/BaseZombie.h"

class UStudentPerceptor_PetrovaIva;

namespace GameAI::BT
{
	class FleeAction_PetrovaIva : public Action
	{
	public:
		explicit FleeAction_PetrovaIva(float SafeDistance = 1200.f, float FleeDistance = 1800.f, UStudentPerceptor_PetrovaIva* Perceptor = nullptr);

		virtual void OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual ENodeStatus Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual void OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;

	private:
		float m_SafeDistance{};
		float m_FleeDistance{};
		bool m_IsMoving{ false };
		UStudentPerceptor_PetrovaIva* m_pPerceptor{ nullptr };

		// Evade predicts where the zombie will be and steers away from that point
		Evade  m_EvadeSteering{};
	};
}