#include "BTActions_PetrovaIva/FightAction_PetrovaIva.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "Zombies/BaseZombie.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"

static bool IsHeavyZombie_Fight(ABaseZombie* pZombie)
{
	return pZombie && pZombie->GetClass()->GetName().Contains(TEXT("Heavy"));
}

namespace GameAI::BT
{
	FightAction_PetrovaIva::FightAction_PetrovaIva(float EngageRange)
		: m_EngageRange(EngageRange)
	{
	}

	void FightAction_PetrovaIva::OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_pCachedZombie = nullptr;
		m_SearchTimer = SEARCH_INTERVAL;
		m_HasActiveMoveRequest = false;
		m_LastPathTarget = FVector::ZeroVector;
	}

	ENodeStatus FightAction_PetrovaIva::Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Fight Action Ticking"));

		UInventoryComponent* pInventory = Survivor.GetComponentByClass<UInventoryComponent>();

		if (!pInventory)
		{
			return ENodeStatus::Failed;
		}

		int weaponSlot { FindWeaponSlot(pInventory) };

		if (weaponSlot < 0)
		{
			return ENodeStatus::Failed;
		}

		// Refresh zombie cache every SEARCH_INTERVAL seconds
		m_SearchTimer += DeltaTime;

		if (m_SearchTimer >= SEARCH_INTERVAL || !IsValid(m_pCachedZombie))
		{
			m_SearchTimer = 0.f;

			TArray<AActor*> pFoundZombies;

			UGameplayStatics::GetAllActorsOfClass( Survivor.GetWorld(), ABaseZombie::StaticClass(), pFoundZombies);

			m_pCachedZombie = nullptr;

			float closestDistSq {m_EngageRange * m_EngageRange};

			for (AActor* pActor : pFoundZombies)
			{
				float distSq = FVector::DistSquared( Survivor.GetActorLocation(), pActor->GetActorLocation());

				if (distSq <= closestDistSq)
				{
					closestDistSq = distSq;
					m_pCachedZombie = pActor;
				}
			}
		}

		if (!IsValid(m_pCachedZombie))
		{
			return ENodeStatus::Failed;
		}

		ABaseZombie* pZombie = Cast<ABaseZombie>(m_pCachedZombie);

		float distToZombie = FVector::Dist( Survivor.GetActorLocation(), m_pCachedZombie->GetActorLocation());

		//////////// Zombie-type movement strategy
		// Runner— weak, rush it
		// Heavy— tanky, kite it
		// Normal— approach to engage range with Pursuit.

		FSurvivorSteeringProxy proxy(Survivor);

		FVector zombiePos = m_pCachedZombie->GetActorLocation();
		FVector zombieVel = m_pCachedZombie->GetVelocity();

		FTargetData zombieTarget{};
		zombieTarget.Position = FVector2D(zombiePos.X, zombiePos.Y);
		zombieTarget.LinearVelocity = FVector2D(zombieVel.X, zombieVel.Y);

		const float KITE_DISTANCE = m_EngageRange * 0.85f;
		SteeringOutput moveOut{};

		if (IsHeavyZombie_Fight(pZombie))
		{
			// Kite: if closer than kite distance, back away using plain Flee
			if (distToZombie < KITE_DISTANCE)
			{
				FTargetData staticTarget{};

				staticTarget.Position = zombieTarget.Position;

				Flee fleeSteering{};

				fleeSteering.m_Target= staticTarget;

				moveOut = fleeSteering.CalculateSteering(DeltaTime, proxy);
			}
		}
		else
		{
			// Runner or Normal: use MoveToLocation for navmesh pathfinding
			if (distToZombie > m_EngageRange * 0.5f)
			{
				constexpr float PATH_RECOMPUTE_DIST_SQ = 200.f * 200.f;

				FVector zombiePos3D = m_pCachedZombie->GetActorLocation();

				bool needsRepath =
					!m_HasActiveMoveRequest ||
					FVector::DistSquared(zombiePos3D, m_LastPathTarget) > PATH_RECOMPUTE_DIST_SQ;

				if (needsRepath)
				{
					if (AAIController* pController = Cast<AAIController>(Survivor.GetController()))
					{
						pController->MoveToLocation( zombiePos3D, m_EngageRange * 0.5f, true, true, true);

						m_HasActiveMoveRequest = true;
					}

					m_LastPathTarget = zombiePos3D;
				}
			}
			else
			{
				// Close enough — stop movement
				if (m_HasActiveMoveRequest)
				{
					if (AAIController* pController = Cast<AAIController>(Survivor.GetController()))
					{
						pController->StopMovement();
					}

					m_HasActiveMoveRequest = false;
				}
			}
		}

		if (moveOut.LinearVelocity.SizeSquared() > 0.01f)
		{
			FVector direction( moveOut.LinearVelocity.X, moveOut.LinearVelocity.Y, 0.f);

			Survivor.AddMovementInput(direction.GetSafeNormal());
		}

		// Rotate the pawn toward the zombie before firing
		m_FaceSteering.m_Target = zombieTarget;
		SteeringOutput faceOut = m_FaceSteering.CalculateSteering(DeltaTime, proxy);

		if (FMath::Abs(faceOut.AngularVelocity) > 0.1f)
		{
			FRotator current = Survivor.GetActorRotation();
			float newYaw = current.Yaw + FMath::Clamp(faceOut.AngularVelocity, -360.f, 360.f) * DeltaTime * 5.f;

			Survivor.SetActorRotation(FRotator(0.f, newYaw, 0.f));
		}

		// Fire weapon
		pInventory->UseItem(weaponSlot);
		const TArray<ABaseItem*>& ITEMS = pInventory->GetInventory();

		if (ITEMS[weaponSlot] && ITEMS[weaponSlot]->GetValue() <= 0)
		{
			pInventory->RemoveItem(weaponSlot);
		}

		return ENodeStatus::Running;
	}

	void FightAction_PetrovaIva::OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_pCachedZombie = nullptr;
		m_SearchTimer = 0.f;
		m_LastPathTarget = FVector::ZeroVector;

		if (m_HasActiveMoveRequest)
		{
			if (AAIController* pController = Cast<AAIController>(Survivor.GetController()))
			{
				pController->StopMovement();
			}

			m_HasActiveMoveRequest = false;
		}
	}

	int FightAction_PetrovaIva::FindWeaponSlot(UInventoryComponent* pInventory) const
	{
		auto ITEMS = pInventory->GetInventory();
		int bestSlot{-1};
		float bestValue{0.f};

		for (int32 index{}; index < ITEMS.Num(); ++index)
		{
			if (!ITEMS[index])
			{
				continue;
			}

			EItemType type = ITEMS[index]->GetItemType();

			if ((type == EItemType::Pistol || type == EItemType::Shotgun) && ITEMS[index]->GetValue() > 0)
			{
				if (ITEMS[index]->GetValue() > bestValue)
				{
					bestValue = ITEMS[index]->GetValue();
					bestSlot = index;
				}
			}
		}

		return bestSlot;
	}
}