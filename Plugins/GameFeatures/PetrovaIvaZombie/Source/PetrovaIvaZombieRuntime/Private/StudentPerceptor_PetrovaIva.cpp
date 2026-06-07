#include "StudentPerceptor_PetrovaIva.h"

#include "AIController.h"
#include "SurvivorBTComponent_PetrovaIva.h"
#include "Survivor/SurvivorPawn.h"
#include "Common/HealthComponent.h"
#include "Common/InventoryComponent.h"
#include "Common/StaminaComponent.h"
#include "Items/BaseItem.h"
#include "Items/ItemType.h"
#include "BTActions_PetrovaIva/PurgeZoneAction_PetrovaIva.h"
#include "BTActions_PetrovaIva/HideAction_PetrovaIva.h"
#include "BTActions_PetrovaIva/FleeAction_PetrovaIva.h"
#include "BTActions_PetrovaIva/FightAction_PetrovaIva.h"
#include "BTActions_PetrovaIva/UseItemAction_PetrovaIva.h"
#include "BTActions_PetrovaIva/LootAction_PetrovaIva.h"
#include "BTActions_PetrovaIva/ExploreAction_PetrovaIva.h"

#include "Kismet/GameplayStatics.h"

#include "PurgeZones/PurgeZone.h"

UStudentPerceptor_PetrovaIva::UStudentPerceptor_PetrovaIva()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UStudentPerceptor_PetrovaIva::BeginPlay()
{
	Super::BeginPlay();

	if (UAIPerceptionComponent* pPerceptionComp = GetOwner()->GetComponentByClass<UAIPerceptionComponent>())
	{
		pPerceptionComp->OnTargetPerceptionUpdated.AddDynamic(this, &UStudentPerceptor_PetrovaIva::OnPerceptionUpdated);
	}

	FTimerHandle timerHandle;
	GetWorld()->GetTimerManager().SetTimer(timerHandle, [this]()
		{
			ASurvivorPawn* pSurvivor = Cast<ASurvivorPawn>(GetOwner());
			if (!pSurvivor) return;

			AAIController* pController = Cast<AAIController>(pSurvivor->GetController());
			if (!pController) return;

			USurvivorBTComponent_PetrovaIva* pBTComp = NewObject<USurvivorBTComponent_PetrovaIva>(pController, TEXT("SurvivorBTComponent_PetrovaIva"));
			pBTComp->RegisterComponent();
			pController->BrainComponent = pBTComp;

			BuildBehaviorTree(pSurvivor, pBTComp);
			pBTComp->StartLogic();
		}, 0.5f, false);
}

void UStudentPerceptor_PetrovaIva::BuildBehaviorTree(ASurvivorPawn* Survivor, USurvivorBTComponent_PetrovaIva* BTComp)
{
	using namespace GameAI::BT;

	// ---- Blackboard query helpers ----

	auto GetHealth = [Survivor]() -> UHealthComponent*
		{ return Survivor ? Survivor->GetComponentByClass<UHealthComponent>() : nullptr; };

	auto GetInventory = [Survivor]() -> UInventoryComponent*
		{ return Survivor ? Survivor->GetComponentByClass<UInventoryComponent>() : nullptr; };

	auto GetStamina = [Survivor]() -> UStaminaComponent*
		{ return Survivor ? Survivor->GetComponentByClass<UStaminaComponent>() : nullptr; };

	auto HasItemOfType = [GetInventory](EItemType Type) -> bool
		{
			UInventoryComponent* pInv = GetInventory();
			if (!pInv) return false;
			for (ABaseItem* pItem : pInv->GetInventory())
			{
				if (pItem && pItem->GetItemType() == Type && pItem->GetValue() > 0)
				{
					return true;
				}
			}
				
			return false;
		};

	auto InventoryFull = [GetInventory]() -> bool
		{
			UInventoryComponent* pInv = GetInventory();
			if (!pInv) return false;
			for (ABaseItem* pItem : pInv->GetInventory())
			{
				if (pItem == nullptr) return false;
			}
				
			return true;
		};

	auto CleanInventory = [GetInventory]() -> bool
		{
			UInventoryComponent* pInv = GetInventory();
			if (!pInv) return false;
			bool isCleaned = false;
			auto items = pInv->GetInventory();
			for (int32 index{}; index < items.Num(); ++index)
			{
				if (items[index] && items[index]->GetValue() <= 0)
				{
					pInv->RemoveItem(index);
					isCleaned = true;
				}
			}
			return isCleaned;
		};

	struct FPurgeZoneCache
	{
		TArray<AActor*> Zones;
		float Timer{};
	};
	auto PurgeCache = std::make_shared<FPurgeZoneCache>();

	constexpr float PURGE_ZONE_CACHE_INTERVAL = 1.f;
	constexpr float PURGE_ZONE_SENTINEL_RADIUS = 5000.f; // replace with Zone->GetDiameter()*0.5f when available

	auto IsInPurgeZone = [Survivor, PurgeCache]() -> bool
		{
			if (!Survivor) return false;

			// Refresh the actor list at most once per second
			PurgeCache->Timer += Survivor->GetWorld()->GetDeltaSeconds();
			if (PurgeCache->Timer >= PURGE_ZONE_CACHE_INTERVAL || PurgeCache->Zones.IsEmpty())
			{
				PurgeCache->Timer = 0.f;
				UGameplayStatics::GetAllActorsOfClass(Survivor->GetWorld(), APurgeZone::StaticClass(), PurgeCache->Zones);
			}

			FVector survivorPos = Survivor->GetActorLocation();
			for (AActor* pActor : PurgeCache->Zones)
			{
				APurgeZone* pZone = Cast<APurgeZone>(pActor);
				if (!pZone) continue;
				if (FVector::Dist(survivorPos, pZone->GetActorLocation()) < PURGE_ZONE_SENTINEL_RADIUS) return true;
			}
			return false;
		};

	auto Root = std::make_unique<Selector>();

	//////////////////////////////////////////////////////////////////////////////// Branch -1: Death guard
	Root->AddChild(std::make_unique<Condition>([GetHealth, HasItemOfType, Survivor, BTComp]()
		{
			UHealthComponent* pHealth = GetHealth();
			if (!pHealth) return false;
			if (pHealth->GetHealth() <= 0 && !HasItemOfType(EItemType::Medkit))
			{
				if (BTComp) BTComp->StopLogic(TEXT("Dead"));
				if (Survivor) Survivor->StopRunning();
				return true;
			}
			return false;
		}));

	////////////////////////////////////////////////////////////////////////////// Branch 0: Garbage cleanup (always fails so Selector continues)
	Root->AddChild(std::make_unique<Condition>([CleanInventory]()
		{
			CleanInventory();
			return false;
		}));

	////////////////////////////////////////////////////////////////////////////// Branch 1: PURGE ZONE 
	{
		auto seq = std::make_unique<Sequence>();
		seq->AddChild(std::make_unique<Condition>([IsInPurgeZone]() mutable
			{
				return IsInPurgeZone();
			}));
		seq->AddChild(std::make_unique<PurgeZoneAction_PetrovaIva>());
		Root->AddChild(std::move(seq));
	}

	////////////////////////////////////////////////////////////////////////////// Branch 2: Hide — zombie nearby, no weapon, but a house is in sight.
	{
		auto seq = std::make_unique<Sequence>();
		seq->AddChild(std::make_unique<Condition>([this]() { return HasNearbyZombie(); }));
		seq->AddChild(std::make_unique<Condition>([HasItemOfType]()
			{ return !HasItemOfType(EItemType::Pistol) && !HasItemOfType(EItemType::Shotgun); }));
		seq->AddChild(std::make_unique<Condition>([this]()
			{ return !GetPerceivedHouses().IsEmpty(); }));
		seq->AddChild(std::make_unique<HideAction_PetrovaIva>(this));
		Root->AddChild(std::move(seq));
	}

	////////////////////////////////////////////////////////////////////////////// Branch 3: Flee — zombie nearby, no weapon
	{
		auto seq = std::make_unique<Sequence>();
		seq->AddChild(std::make_unique<Condition>([this]() { return HasNearbyZombie(); }));
		seq->AddChild(std::make_unique<Condition>([HasItemOfType]()
			{ return !HasItemOfType(EItemType::Pistol) && !HasItemOfType(EItemType::Shotgun); }));
		seq->AddChild(std::make_unique<FleeAction_PetrovaIva>(1200.f, 1800.f, this));
		Root->AddChild(std::move(seq));
	}

	////////////////////////////////////////////////////////////////////////////// Branch 4: Fight — zombie nearby, has weapon
	{
		auto seq = std::make_unique<Sequence>();
		seq->AddChild(std::make_unique<Condition>([this]() { return HasNearbyZombie(); }));
		seq->AddChild(std::make_unique<Condition>([HasItemOfType]()
			{ return HasItemOfType(EItemType::Pistol) || HasItemOfType(EItemType::Shotgun); }));
		seq->AddChild(std::make_unique<FightAction_PetrovaIva>(600.f));
		Root->AddChild(std::move(seq));
	}

	////////////////////////////////////////////////////////////////////////////// Branch 5: Heal — low health, has medkit
	{
		auto seq = std::make_unique<Sequence>();
		seq->AddChild(std::make_unique<Condition>([GetHealth]()
			{
				UHealthComponent* pHealth = GetHealth();
				return pHealth && (static_cast<float>(pHealth->GetHealth()) / static_cast<float>(pHealth->GetMaxHealth()) <= 0.75f);
			}));
		seq->AddChild(std::make_unique<Condition>([HasItemOfType]()
			{ return HasItemOfType(EItemType::Medkit); }));
		seq->AddChild(std::make_unique<UseItemAction_PetrovaIva>(EItemType::Medkit));
		Root->AddChild(std::move(seq));
	}

	////////////////////////////////////////////////////////////////////////////// Branch 6: Eat — low stamina, has food
	{
		auto seq = std::make_unique<Sequence>();
		seq->AddChild(std::make_unique<Condition>([GetStamina]()
			{
				UStaminaComponent* pStamina = GetStamina();
				return pStamina && (pStamina->GetCurrentStamina() / pStamina->GetMaxStamina() <= 0.5f);
			}));
		seq->AddChild(std::make_unique<Condition>([HasItemOfType]()
			{ return HasItemOfType(EItemType::Food); }));
		seq->AddChild(std::make_unique<UseItemAction_PetrovaIva>(EItemType::Food));
		Root->AddChild(std::move(seq));
	}

	////////////////////////////////////////////////////////////////////////////// Branch 7: Loot — inventory not full
	{
		auto seq = std::make_unique<Sequence>();
		seq->AddChild(std::make_unique<Condition>([InventoryFull]()
			{ return !InventoryFull(); }));
		seq->AddChild(std::make_unique<LootAction_PetrovaIva>(this));
		Root->AddChild(std::move(seq));
	}

	////////////////////////////////////////////////////////////////////////////// Branch 8: Explore
	Root->AddChild(std::make_unique<ExploreAction_PetrovaIva>());

	BTComp->SetRoot(std::move(Root));
}

void UStudentPerceptor_PetrovaIva::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!Actor) return;

	bool wasSensed = Stimulus.WasSuccessfullySensed();

	if (ABaseZombie* pZombie = Cast<ABaseZombie>(Actor))
	{
		if (wasSensed)
		{
			PerceivedZombies.AddUnique(pZombie);
		}
		else
		{
			PerceivedZombies.Remove(pZombie);
		}
		return;
	}

	if (ABaseItem* pItem = Cast<ABaseItem>(Actor))
	{
		if (wasSensed)
		{
			PerceivedItems.AddUnique(pItem);
		}
		else
		{
			PerceivedItems.Remove(pItem);
		}
		return;
	}

	if (AHouse* pHouse = Cast<AHouse>(Actor))
	{
		if (wasSensed)
		{
			PerceivedHouses.AddUnique(pHouse);
		}
		else
		{
			PerceivedHouses.Remove(pHouse);
		}
		return;
	}
}