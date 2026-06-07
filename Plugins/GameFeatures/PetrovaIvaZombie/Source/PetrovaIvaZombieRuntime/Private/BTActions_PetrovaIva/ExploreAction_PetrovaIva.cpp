#include "BTActions_PetrovaIva/ExploreAction_PetrovaIva.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "Village/House/House.h"
#include "Kismet/GameplayStatics.h"

namespace GameAI::BT
{
	void ExploreAction_PetrovaIva::OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_TargetHouse = nullptr;
		m_LastPosition = Survivor.GetActorLocation();
		m_StuckTimer = 0.f;
		m_IsUnstucking = false;
		m_UnstuckTimer = 0.f;

		// Stop any leftover nav-mesh movement from a previous action
		if (AAIController* pController = Cast<AAIController>(Survivor.GetController()))
		{
			pController->StopMovement();
		}
	}

	ENodeStatus ExploreAction_PetrovaIva::Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Explore Action Ticking"));

		FSurvivorSteeringProxy proxy(Survivor);
		FVector currentPos = Survivor.GetActorLocation();

		// Stuck detection — if the player hasn't moved much in STUCK_TIME_THRESHOLD seconds, he might be stuck on geometry
		m_StuckTimer += DeltaTime;
		if (m_StuckTimer >= STUCK_TIME_THRESHOLD)
		{
			float distMoved = FVector::Dist(currentPos, m_LastPosition);
			m_StuckTimer = 0.f;
			m_LastPosition = currentPos;

			if (distMoved < STUCK_DIST_THRESHOLD)
			{
				// Switch to Wander for a short burst to break free
				m_IsUnstucking = true;
				m_UnstuckTimer = 0.f;
			}
		}

		// Unstuck mode — wander randomly for UNSTUCK_DURATION seconds
		if (m_IsUnstucking)
		{
			m_UnstuckTimer += DeltaTime;

			// Pick a random lateral direction to break free from the wall
			FVector forward = Survivor.GetActorForwardVector();
			FVector right = Survivor.GetActorRightVector();
			FVector escapeDir = (-forward + (FMath::RandBool() ? right : -right)).GetSafeNormal();
			Survivor.AddMovementInput(escapeDir);

			if (m_UnstuckTimer >= UNSTUCK_DURATION)
			{
				m_IsUnstucking = false;
				m_TargetHouse = nullptr;
			}

			return ENodeStatus::Running;
		}

		// Normal mode — MoveToLocation toward the closest unvisited house
		if (!IsValid(m_TargetHouse))
		{
			m_TargetHouse = FindClosestUnvisitedHouse(Survivor);
			if (!m_TargetHouse)
			{
				m_VisitedHouses.Empty();
				return ENodeStatus::Failed;
			}

			// Issue a nav-mesh move request, the controller will handle pathfinding and obstacle avoidance
			FHouseBounds bounds = m_TargetHouse->GetBounds();
			if (AAIController* pController = Cast<AAIController>(Survivor.GetController()))
			{
				float ACCEPTANCE_RADIUS{ 150.f };
				pController->MoveToLocation(bounds.Origin, ACCEPTANCE_RADIUS, true, true, true);
			}
		}

		if (IsInsideHouse(Survivor, m_TargetHouse))
		{
			m_VisitedHouses.AddUnique(m_TargetHouse);
			if (AAIController* pController = Cast<AAIController>(Survivor.GetController()))
			{
				pController->StopMovement();
			}
			m_TargetHouse = nullptr;
			return ENodeStatus::Succeeded;
		}

		return ENodeStatus::Running;
	}

	void ExploreAction_PetrovaIva::OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_TargetHouse = nullptr;
		m_StuckTimer = 0.f;
		m_IsUnstucking = false;
		m_UnstuckTimer = 0.f;
		if (AAIController* pController = Cast<AAIController>(Survivor.GetController()))
		{
			pController->StopMovement();
		}
	}

	AHouse* ExploreAction_PetrovaIva::FindClosestUnvisitedHouse(ASurvivorPawn& Survivor) const
	{
		TArray<AActor*> pFoundHouses;
		UGameplayStatics::GetAllActorsOfClass(Survivor.GetWorld(), AHouse::StaticClass(), pFoundHouses);

		AHouse* pBest = nullptr;
		float bestDistSq = FLT_MAX;

		for (AActor* pActor : pFoundHouses)
		{
			AHouse* pHouse = Cast<AHouse>(pActor);
			if (!pHouse || m_VisitedHouses.Contains(pHouse)) continue;

			float distSq = FVector::DistSquared(Survivor.GetActorLocation(), pHouse->GetActorLocation());
			if (distSq < bestDistSq)
			{
				bestDistSq = distSq;
				pBest = pHouse;
			}
		}

		return pBest;
	}

	bool ExploreAction_PetrovaIva::IsInsideHouse(ASurvivorPawn& Survivor, AHouse* House) const
	{
		FHouseBounds bounds = House->GetBounds();
		FBox Box(bounds.Origin - bounds.Extent * 0.8f, bounds.Origin + bounds.Extent * 0.8f);
		return Box.IsInside(Survivor.GetActorLocation());
	}
}