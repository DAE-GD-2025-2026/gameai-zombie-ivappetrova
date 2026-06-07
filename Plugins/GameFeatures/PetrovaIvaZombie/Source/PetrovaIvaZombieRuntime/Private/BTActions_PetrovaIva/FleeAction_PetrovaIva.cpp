#include "BTActions_PetrovaIva/FleeAction_PetrovaIva.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "Zombies/BaseZombie.h"
#include "StudentPerceptor_PetrovaIva.h"

static bool IsHeavyZombie(ABaseZombie* Z)
{
	return Z && Z->GetClass()->GetName().Contains(TEXT("Heavy"));
}

static bool IsRunnerZombie(ABaseZombie* Z)
{
	return Z && Z->GetClass()->GetName().Contains(TEXT("Runner"));
}

namespace GameAI::BT
{
	FleeAction_PetrovaIva::FleeAction_PetrovaIva(float SafeDistance, float FleeDistance,
		UStudentPerceptor_PetrovaIva* Perceptor)
		: m_SafeDistance(SafeDistance)
		, m_FleeDistance(FleeDistance)
		, m_pPerceptor(Perceptor)
	{
		m_EvadeSteering.m_EvadeRadius = FleeDistance;
	}

	void FleeAction_PetrovaIva::OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_IsMoving = false;
		Survivor.StartRunning();
	}

	ENodeStatus FleeAction_PetrovaIva::Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Flee Action Ticking"));

		if (!m_pPerceptor) return ENodeStatus::Failed;

		auto perceivedZombies = m_pPerceptor->GetPerceivedZombies();
		if (perceivedZombies.IsEmpty()) return ENodeStatus::Failed;

		// Pick the most dangerous zombie to flee from
		ABaseZombie* pMostDangerous{ nullptr };
		float mostDangerousDist{ FLT_MAX };
		int mostDangerousTier{ -1 };

		for (ABaseZombie* pZombie : perceivedZombies)
		{
			if (!IsValid(pZombie)) continue;

			float dist = FVector::Dist(Survivor.GetActorLocation(), pZombie->GetActorLocation());

			// Only react to zombies within flee distance
			if (dist >= m_FleeDistance) continue;

			int tier{ 0 };
			if (IsHeavyZombie(pZombie))  
			{
				tier = 2;
			}
			else if (IsRunnerZombie(pZombie))
			{
				tier = 1;
			}

			if (tier > mostDangerousTier || (tier == mostDangerousTier && dist < mostDangerousDist))
			{
				mostDangerousTier = tier;
				mostDangerousDist = dist;
				pMostDangerous = pZombie;
			}
		}

		// all zombies are out of range
		if (!pMostDangerous) return ENodeStatus::Succeeded; 

		// Already far enough from the most dangerous zombie — done
		if (mostDangerousDist >= m_SafeDistance) return ENodeStatus::Succeeded;

		// Build target data for Evade
		FTargetData zombieTarget{};
		FVector zombiePos = pMostDangerous->GetActorLocation();
		FVector zombieVel = pMostDangerous->GetVelocity();
		zombieTarget.Position = FVector2D(zombiePos.X, zombiePos.Y);
		zombieTarget.LinearVelocity = FVector2D(zombieVel.X, zombieVel.Y);
		FSurvivorSteeringProxy proxy(Survivor);
		m_EvadeSteering.m_Target = zombieTarget;
		SteeringOutput Out = m_EvadeSteering.CalculateSteering(DeltaTime, proxy);

		if (!Out.IsValid || Out.LinearVelocity.SizeSquared() < 0.01f)
		{
			FVector2D awayDir = proxy.GetPosition() - zombieTarget.Position;
			Out.LinearVelocity = awayDir;
		}

		FVector dir(Out.LinearVelocity.X, Out.LinearVelocity.Y, 0.f);
		Survivor.AddMovementInput(dir.GetSafeNormal());

		return ENodeStatus::Running;
	}

	void FleeAction_PetrovaIva::OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_IsMoving = false;
		Survivor.StopRunning();
	}
}