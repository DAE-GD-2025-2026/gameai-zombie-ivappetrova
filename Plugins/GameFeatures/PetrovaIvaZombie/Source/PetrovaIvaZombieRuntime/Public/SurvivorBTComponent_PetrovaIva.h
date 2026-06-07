#pragma once

#include <memory>
#include <functional>

#include "CoreMinimal.h"
#include "BrainComponent.h"
#include "BehaviorTree_PetrovaIva.h"
#include "SurvivorBTComponent_PetrovaIva.generated.h"

class ASurvivorPawn;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class PETROVAIVAZOMBIERUNTIME_API USurvivorBTComponent_PetrovaIva : public UBrainComponent
{
	GENERATED_BODY()

public:
	USurvivorBTComponent_PetrovaIva();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void StartLogic() override;
	virtual void StopLogic(const FString& Reason) override;
	virtual bool IsRunning() const override;

	void SetRoot(std::unique_ptr<GameAI::BT::Node>&& Root);

protected:
	virtual void BeginPlay() override;

private:
	std::unique_ptr<GameAI::BT::BehaviorTree> m_BTInstance;
	bool m_IsRunning{ false };
};