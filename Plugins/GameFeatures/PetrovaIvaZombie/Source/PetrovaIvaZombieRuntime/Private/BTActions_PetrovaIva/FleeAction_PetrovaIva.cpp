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
		m_LastPosition = Survivor.GetActorLocation();
		m_StuckTimer = 0.f;
		m_EscapeAngleStep = 0;
		m_TotalStuckTime = 0.f;
		Survivor.StartRunning();
	}

	ENodeStatus FleeAction_PetrovaIva::Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Flee Action Ticking"));

		if (!m_pPerceptor) return ENodeStatus::Failed;

		auto perceivedZombies = m_pPerceptor->GetPerceivedZombies();
		if (perceivedZombies.IsEmpty()) return ENodeStatus::Failed;

		FSurvivorSteeringProxy proxy(Survivor);
		FVector2D survivorPos2D = proxy.GetPosition();

		FVector2D aggregateFleeVec = FVector2D::ZeroVector;
		float closestDist = FLT_MAX;
		bool anyInRange = false;

		for (ABaseZombie* pZombie : perceivedZombies)
		{
			if (!IsValid(pZombie)) continue;

			FVector zombiePos = pZombie->GetActorLocation();
			float dist = FVector::Dist(Survivor.GetActorLocation(), zombiePos);

			if (dist >= m_FleeDistance) continue;
			anyInRange = true;
			closestDist = FMath::Min(closestDist, dist);

			// Tier multiplier: heavier/faster zombies push harder
			float tierMult = 1.f;
			if (IsHeavyZombie(pZombie))       tierMult = 3.f;
			else if (IsRunnerZombie(pZombie))  tierMult = 2.f;

			// Predict future zombie position and flee from that (evade logic)
			FVector zombieVel = pZombie->GetVelocity();
			FVector2D zombiePos2D(zombiePos.X, zombiePos.Y);
			FVector2D zombieVel2D(zombieVel.X, zombieVel.Y);
			float t = dist / (proxy.GetMaxLinearSpeed() + 0.01f);
			FVector2D futurePos = zombiePos2D + zombieVel2D * t;

			FVector2D awayFromFuture = survivorPos2D - futurePos;
			float weight = tierMult / FMath::Max(dist, 1.f);
			aggregateFleeVec += awayFromFuture.GetSafeNormal() * weight;
		}

		if (!anyInRange) return ENodeStatus::Succeeded;
		if (closestDist >= m_SafeDistance) return ENodeStatus::Succeeded;

		// Stuck detection — rotate escape direction in 75° steps when not moving
		m_StuckTimer += DeltaTime;
		if (m_StuckTimer >= STUCK_TIME_THRESHOLD)
		{
			float distMoved = FVector::Dist(Survivor.GetActorLocation(), m_LastPosition);
			m_LastPosition = Survivor.GetActorLocation();
			m_StuckTimer = 0.f;

			if (distMoved < STUCK_DIST_THRESHOLD)
				m_EscapeAngleStep++;
			else
				m_EscapeAngleStep = 0;
		}

		// Hard cap — if rotating hasn't helped after MAX_TOTAL_STUCK_TIME, give up
		if (m_EscapeAngleStep > 0)
		{
			m_TotalStuckTime += DeltaTime;
			if (m_TotalStuckTime >= MAX_TOTAL_STUCK_TIME)
				return ENodeStatus::Failed;
		}
		else
		{
			m_TotalStuckTime = 0.f;
		}

		FVector dir(aggregateFleeVec.X, aggregateFleeVec.Y, 0.f);
		if (dir.SizeSquared() < 0.01f)
		{
			dir = Survivor.GetActorRightVector();
		}

		float angleDeg = m_EscapeAngleStep * 75.f;
		dir = dir.RotateAngleAxis(angleDeg, FVector::UpVector);

		Survivor.AddMovementInput(dir.GetSafeNormal());

		return ENodeStatus::Running;
	}

	void FleeAction_PetrovaIva::OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_IsMoving = false;
		m_TotalStuckTime = 0.f;
		Survivor.StopRunning();
	}
}