#include "BTActions_PetrovaIva/LootAction_PetrovaIva.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"
#include "StudentPerceptor_PetrovaIva.h"

namespace GameAI::BT
{
	LootAction_PetrovaIva::LootAction_PetrovaIva(UStudentPerceptor_PetrovaIva* Perceptor)
		: m_pPerceptor(Perceptor)
	{
	}

	void LootAction_PetrovaIva::OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_pTargetItem = nullptr;
		m_StuckTimer = 0.f;

		if (AAIController* pController = Cast<AAIController>(Survivor.GetController()))
		{
			pController->StopMovement();
		}

		m_ArriveSteering.m_TargetRadius = 50.f;
		m_ArriveSteering.m_SlowRadius = 400.f;
	}

	ENodeStatus LootAction_PetrovaIva::Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Loot Action Ticking"));

		if (!m_pPerceptor) return ENodeStatus::Failed;

		UInventoryComponent* pInventory = Survivor.GetComponentByClass<UInventoryComponent>();
		if (!pInventory)return ENodeStatus::Failed;

		int emptySlot = FindEmptySlot(pInventory);
		if (emptySlot < 0) return ENodeStatus::Failed;

		// Drop cached target if it was consumed or destroyed
		if (IsValid(m_pTargetItem) && m_pTargetItem->GetValue() <= 0)
		{
			m_pTargetItem = nullptr;
		}
		
		// Pick a new target from perceived items, skipping blacklisted ones
		if (!IsValid(m_pTargetItem))
		{
			m_StuckTimer = 0.f;

			auto perceivedItems = m_pPerceptor->GetPerceivedItems();

			// Purge stale blacklist entries (items that have since disappeared)
			m_BlacklistedItems.RemoveAll([](ABaseItem* Item)
				{
					return !IsValid(Item) || Item->GetValue() <= 0;
				});

			int availableCount{};
			for (ABaseItem* pItem : perceivedItems)
			{
				if (IsValid(pItem) && pItem->GetValue() > 0 && !m_BlacklistedItems.Contains(pItem))
				{
					++availableCount;
				}
					
			}

			// If every perceived item is blacklisted, clear the blacklist and retry
			if (availableCount == 0)
			{
				m_BlacklistedItems.Empty();

				// Re-check after clearing
				for (ABaseItem* pItem : perceivedItems)
				{
					if (IsValid(pItem) && pItem->GetValue() > 0)
					{
						++availableCount;
					}
				}

				if (availableCount == 0) return ENodeStatus::Failed;
			}

			// Pick the closest non-blacklisted perceived item
			float bestDistSq = FLT_MAX;
			for (ABaseItem* pItem : perceivedItems)
			{
				if (!IsValid(pItem) || pItem->GetValue() <= 0) continue;
				if (m_BlacklistedItems.Contains(pItem)) continue;
				float distSq = FVector::DistSquared(Survivor.GetActorLocation(), pItem->GetActorLocation());
				if (distSq < bestDistSq)
				{
					bestDistSq = distSq;
					m_pTargetItem = pItem;
				}
			}

			if (!IsValid(m_pTargetItem)) return ENodeStatus::Failed;

			float pickupRadius = pInventory->GetPickupRange() * 0.8f;
			m_ArriveSteering.m_TargetRadius = pickupRadius;
			m_ArriveSteering.m_SlowRadius = pickupRadius * 4.f;
		}

		// Timeout — blacklist this item so we never try it again
		m_StuckTimer += DeltaTime;
		if (m_StuckTimer > MAX_LOOT_TIME)
		{
			UE_LOG(LogTemp, Warning, TEXT("LootAction: timed out on item, blacklisting it."));
			m_BlacklistedItems.AddUnique(m_pTargetItem);
			m_pTargetItem = nullptr;
			m_StuckTimer = 0.f;
			return ENodeStatus::Failed;
		}

		// Steer toward item with Arrive (pre-scaled velocity, no MaxSpeed mutation)
		FVector itemPos = m_pTargetItem->GetActorLocation();
		FTargetData itemTarget{};
		itemTarget.Position = FVector2D(itemPos.X, itemPos.Y);

		FSurvivorSteeringProxy proxy(Survivor);
		m_ArriveSteering.SetTarget(itemTarget);
		SteeringOutput out = m_ArriveSteering.CalculateSteering(DeltaTime, proxy);

		if (out.LinearVelocity.SizeSquared() > 0.01f)
		{
			FVector2D dir2D = out.LinearVelocity.GetSafeNormal();
			float speedScale = FMath::Clamp( out.LinearVelocity.Size() / m_ArriveSteering.m_SlowRadius, 0.1f, 1.f);
			Survivor.AddMovementInput(FVector(dir2D.X, dir2D.Y, 0.f), speedScale);
		}

		// Pickup check
		float pickupRange = pInventory->GetPickupRange();
		if (FVector::DistSquared(Survivor.GetActorLocation(), itemPos) <= pickupRange * pickupRange)
		{
			bool isGrabbed = pInventory->GrabItem(emptySlot, m_pTargetItem);
			m_pTargetItem = nullptr;
			m_StuckTimer = 0.f;
			m_BlacklistedItems.Empty();
			return isGrabbed ? ENodeStatus::Succeeded : ENodeStatus::Failed;
		}

		return ENodeStatus::Running;
	}

	void LootAction_PetrovaIva::OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_pTargetItem = nullptr;
		m_StuckTimer = 0.f;
	}

	int LootAction_PetrovaIva::FindEmptySlot(UInventoryComponent* Inventory) const
	{
		auto items = Inventory->GetInventory();
		for (int32 index{}; index < items.Num(); ++index)
		{
			if (items[index] == nullptr) return index;
		}
		return -1;
	}
}