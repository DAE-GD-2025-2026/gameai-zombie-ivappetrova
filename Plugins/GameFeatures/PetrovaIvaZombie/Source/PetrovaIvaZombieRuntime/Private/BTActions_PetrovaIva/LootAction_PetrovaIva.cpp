#include "BTActions_PetrovaIva/LootAction_PetrovaIva.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/InventoryComponent.h"
#include "Common/HealthComponent.h"
#include "Common/StaminaComponent.h"
#include "Items/BaseItem.h"
#include "StudentPerceptor_PetrovaIva.h"

// Priority helpers
namespace
{
	int CountItemsOfType(UInventoryComponent* Inventory, EItemType Type)
	{
		int count{};
		for (ABaseItem* pItem : Inventory->GetInventory())
		{
			if (pItem && pItem->GetItemType() == Type && pItem->GetValue() > 0)
			{
				++count;
			}
		}
		return count;
	}

	int GetItemPriority(EItemType Type, UInventoryComponent* Inventory, float CurrentStamina, float MaxStamina, int CurrentHealth)
	{
		const bool IS_WEAPON = (Type == EItemType::Pistol || Type == EItemType::Shotgun);

		// Skip garbage
		if (Type == EItemType::Garbage) return -1;

		// Count how many of this type we already carry
		int countOwned = CountItemsOfType(Inventory, Type);

		// Weapons share the 2 weapons cap across both subtypes
		if (IS_WEAPON)
		{
			int weaponCount = CountItemsOfType(Inventory, EItemType::Pistol) + CountItemsOfType(Inventory, EItemType::Shotgun);
			if (weaponCount >= 2) return -1;
		}
		else
		{
			if (countOwned >= 2) return -1;
		}

		// Base scores
		int score{};
		switch (Type)
		{
		case EItemType::Food:   score = 0; break;
		case EItemType::Medkit: score = 1; break;
		case EItemType::Pistol:
		case EItemType::Shotgun: score = 2; break;
		default: return -1;
		}

		// Urgency overrides — swap food and medkit to the very front when critical
		const float staminaRatio = (MaxStamina > 0.f) ? (CurrentStamina / MaxStamina) : 1.f;
		const bool staminaLow = (staminaRatio < 0.75f);   // stamina < 7.5 on a 10-max scale
		const bool healthLow = (CurrentHealth < 7);

		if (Type == EItemType::Food && staminaLow)
		{
			score = -100;
		}
			
		if (Type == EItemType::Medkit && healthLow)
		{
			score = (staminaLow ? -99 : -100); // food still wins if both are critical
		}

		return score;
	}
}
// ---------------------------------------------------------------------------

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

			// Gather survivor stats needed for priority scoring
			float currentStamina = 0.f, maxStamina = 1.f;
			int currentHealth = 10;
			if (UStaminaComponent* pStamina = Survivor.GetComponentByClass<UStaminaComponent>())
			{
				currentStamina = pStamina->GetCurrentStamina();
				maxStamina = pStamina->GetMaxStamina();
			}
			if (UHealthComponent* pHealth = Survivor.GetComponentByClass<UHealthComponent>())
			{
				currentHealth = pHealth->GetHealth();
			}

			// Pick the highest-priority non-blacklisted perceived item;
			// break ties by distance (closest wins within the same priority tier)
			int   bestPriority = INT_MAX;
			float bestDistSq = FLT_MAX;
			for (ABaseItem* pItem : perceivedItems)
			{
				if (!IsValid(pItem) || pItem->GetValue() <= 0) continue;
				if (m_BlacklistedItems.Contains(pItem)) continue;

				int priority = GetItemPriority(pItem->GetItemType(), pInventory, currentStamina, maxStamina, currentHealth);
				if (priority < 0) continue; // skip unwanted types

				float distSq = FVector::DistSquared(Survivor.GetActorLocation(), pItem->GetActorLocation());

				if (priority < bestPriority || (priority == bestPriority && distSq < bestDistSq))
				{
					bestPriority = priority;
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
		m_ArriveSteering.m_Target = itemTarget;
		SteeringOutput out = m_ArriveSteering.CalculateSteering(DeltaTime, proxy);

		if (out.LinearVelocity.SizeSquared() > 0.01f)
		{
			FVector2D dir2D = out.LinearVelocity.GetSafeNormal();
			float speedScale = FMath::Clamp(out.LinearVelocity.Size() / m_ArriveSteering.m_SlowRadius, 0.1f, 1.f);
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