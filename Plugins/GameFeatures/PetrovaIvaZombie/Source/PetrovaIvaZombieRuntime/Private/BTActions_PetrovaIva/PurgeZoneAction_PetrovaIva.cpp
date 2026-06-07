#include "BTActions_PetrovaIva/PurgeZoneAction_PetrovaIva.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "PurgeZones/PurgeZone.h"

namespace GameAI::BT
{
	void PurgeZoneAction_PetrovaIva::OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		Survivor.StartRunning();
	}

	ENodeStatus PurgeZoneAction_PetrovaIva::Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		TArray<AActor*> pFoundZones;
		UGameplayStatics::GetAllActorsOfClass(Survivor.GetWorld(), APurgeZone::StaticClass(), pFoundZones);
		static constexpr float PURGE_ZONE_SENTINEL_RADIUS{ 5000.f };

		FVector survivorPos = Survivor.GetActorLocation();

		// Find the zone we are currently standing inside (if any)
		APurgeZone* pActiveZone = nullptr;
		for (AActor* pActor : pFoundZones)
		{
			APurgeZone* pZone = Cast<APurgeZone>(pActor);
			if (!pZone) continue;

			float dist = FVector::Dist(survivorPos, pZone->GetActorLocation());
			if (dist < PURGE_ZONE_SENTINEL_RADIUS)
			{
				pActiveZone = pZone;
				break;
			}
		}

		if (!pActiveZone) return ENodeStatus::Succeeded;

		// Steer away from the zone centre using Flee
		FVector zoneCenter = pActiveZone->GetActorLocation();
		FTargetData zoneTarget{};
		zoneTarget.Position = FVector2D(zoneCenter.X, zoneCenter.Y);

		FSurvivorSteeringProxy proxy(Survivor);
		m_FleeSteering.m_Target = zoneTarget;
		SteeringOutput out = m_FleeSteering.CalculateSteering(DeltaTime, proxy);

		if (out.LinearVelocity.SizeSquared() > 0.01f)
		{
			FVector dir(out.LinearVelocity.X, out.LinearVelocity.Y, 0.f);
			Survivor.AddMovementInput(dir.GetSafeNormal());
		}

		return ENodeStatus::Running;
	}

	void PurgeZoneAction_PetrovaIva::OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		Survivor.StopRunning();
	}
}