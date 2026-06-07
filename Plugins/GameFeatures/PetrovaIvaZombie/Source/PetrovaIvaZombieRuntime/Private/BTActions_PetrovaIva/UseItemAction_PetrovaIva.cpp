#include "BTActions_PetrovaIva/UseItemAction_PetrovaIva.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/InventoryComponent.h"
#include "Items/BaseItem.h"

namespace GameAI::BT
{
	UseItemAction_PetrovaIva::UseItemAction_PetrovaIva(EItemType TypeToUse)
		: m_TypeToUse(TypeToUse)
	{
	}

	ENodeStatus UseItemAction_PetrovaIva::Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		UE_LOG(LogTemp, Warning, TEXT("Use Item Action Ticking"));

		UInventoryComponent* pInventory = Survivor.GetComponentByClass<UInventoryComponent>();
		if (!pInventory) return ENodeStatus::Failed;

		auto items = pInventory->GetInventory();
		for (int32 index{}; index < items.Num(); ++index)
		{
			if (items[index] && items[index]->GetItemType() == m_TypeToUse && items[index]->GetValue() > 0)
			{
				pInventory->UseItem(index);

				// Remove the item from the slot if it has been fully consumed
				if (items[index] && items[index]->GetValue() <= 0)
				{
					pInventory->RemoveItem(index);
				}

				return ENodeStatus::Succeeded;
			}
		}

		return ENodeStatus::Failed;
	}
}