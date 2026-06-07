#include "BTActions_PetrovaIva/HideAction_PetrovaIva.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "StudentPerceptor_PetrovaIva.h"

namespace GameAI::BT
{
	HideAction_PetrovaIva::HideAction_PetrovaIva(UStudentPerceptor_PetrovaIva* Perceptor)
		: m_pPerceptor(Perceptor)
	{
		// Arrive parameters: stop exactly inside the house, start slowing at 600
		m_ArriveSteering.m_TargetRadius = 150.f;
		m_ArriveSteering.m_SlowRadius = 600.f;
	}

	void HideAction_PetrovaIva::OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_pTargetHouse = nullptr;
		// sprint to the house
		Survivor.StartRunning(); 
	}

	ENodeStatus HideAction_PetrovaIva::Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Hide Action Ticking"));

		if (!m_pPerceptor) return ENodeStatus::Failed;

		// Pick a target house from perception if we don't have one yet
		if (!IsValid(m_pTargetHouse))
		{
			auto houses = m_pPerceptor->GetPerceivedHouses();

			// no house in sight — can't hide
			if (houses.IsEmpty()) return ENodeStatus::Failed; 

			// Choose the closest perceived house
			float bestDistSq = FLT_MAX;
			for (AHouse* pHouse : houses)
			{
				if (!IsValid(pHouse)) continue;
				float distSq = FVector::DistSquared(Survivor.GetActorLocation(), pHouse->GetActorLocation());
				if (distSq < bestDistSq)
				{
					bestDistSq = distSq;
					m_pTargetHouse = pHouse;
				}
			}

			if (!IsValid(m_pTargetHouse)) return ENodeStatus::Failed;
		}

		if (IsInsideHouse(Survivor, m_pTargetHouse)) return ENodeStatus::Succeeded;

		FHouseBounds bounds = m_pTargetHouse->GetBounds();
		FTargetData houseTarget{};
		houseTarget.Position = FVector2D(bounds.Origin.X, bounds.Origin.Y);

		FSurvivorSteeringProxy proxy(Survivor);
		m_ArriveSteering.m_Target = houseTarget;
		SteeringOutput out = m_ArriveSteering.CalculateSteering(DeltaTime, proxy);

		if (out.LinearVelocity.SizeSquared() > 0.01f)
		{
			FVector2D dir2D = out.LinearVelocity.GetSafeNormal();
			float speedScale = FMath::Clamp(out.LinearVelocity.Size() / m_ArriveSteering.m_SlowRadius, 0.1f, 1.f);

			FVector dir(dir2D.X, dir2D.Y, 0.f);
			Survivor.AddMovementInput(dir, speedScale);
		}

		return ENodeStatus::Running;
	}

	void HideAction_PetrovaIva::OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_pTargetHouse = nullptr;
		Survivor.StopRunning();
	}

	bool HideAction_PetrovaIva::IsInsideHouse(ASurvivorPawn& Survivor, AHouse* House) const
	{
		FHouseBounds bounds = House->GetBounds();
		FBox box(bounds.Origin - bounds.Extent * 0.8f, bounds.Origin + bounds.Extent * 0.8f);
		return box.IsInside(Survivor.GetActorLocation());
	}
}