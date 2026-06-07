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

	TArray<ABaseZombie*> const& GetPerceivedZombies() const { return m_PerceivedZombies; }
	TArray<ABaseItem*>   const& GetPerceivedItems()   const { return m_PerceivedItems; }
	TArray<AHouse*>      const& GetPerceivedHouses()  const { return m_PerceivedHouses; }

	bool HasNearbyZombie() const;
	bool HasVisibleItem()  const { return !m_PerceivedItems.IsEmpty(); }

private:
	UPROPERTY()
	TArray<ABaseZombie*> m_PerceivedZombies{};
	UPROPERTY()
	TArray<ABaseItem*> m_PerceivedItems{};
	UPROPERTY()
	TArray<AHouse*> m_PerceivedHouses{};
	void BuildBehaviorTree(ASurvivorPawn* Survivor, USurvivorBTComponent_PetrovaIva* BTComp);

	float m_SurvivalStartTime{ 0.f };
	bool  m_HasLoggedDeath{ false };

	FTimerHandle m_HUDTickHandle;
};