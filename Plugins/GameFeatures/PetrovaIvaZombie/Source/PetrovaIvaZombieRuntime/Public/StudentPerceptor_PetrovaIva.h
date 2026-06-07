#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISenseConfig_Damage.h"
#include "Perception/AISense_Damage.h"
#include "Zombies/BaseZombie.h"
#include "Items/BaseItem.h"
#include "Village/House/House.h"
#include "StudentPerceptor_PetrovaIva.generated.h"

class ASurvivorPawn;
class USurvivorBTComponent_PetrovaIva;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PETROVAIVAZOMBIERUNTIME_API UStudentPerceptor_PetrovaIva : public UActorComponent
{
	GENERATED_BODY()

public:
	UStudentPerceptor_PetrovaIva();

	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	TArray<ABaseZombie*> const& GetPerceivedZombies() const { return PerceivedZombies; }
	TArray<ABaseItem*> const& GetPerceivedItems()   const { return PerceivedItems; }
	TArray<AHouse*> const& GetPerceivedHouses()  const { return PerceivedHouses; }

	bool HasNearbyZombie() const { return !PerceivedZombies.IsEmpty(); }
	bool HasVisibleItem() const { return !PerceivedItems.IsEmpty(); }

private:
	UPROPERTY()
	TArray<ABaseZombie*> PerceivedZombies{};
	UPROPERTY()
	TArray<ABaseItem*> PerceivedItems{};
	UPROPERTY()
	TArray<AHouse*> PerceivedHouses{};

	void BuildBehaviorTree(ASurvivorPawn* Survivor, USurvivorBTComponent_PetrovaIva* BTComp);
};