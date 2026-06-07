#include "SurvivorBTComponent_PetrovaIva.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Survivor/SurvivorPawn.h"

USurvivorBTComponent_PetrovaIva::USurvivorBTComponent_PetrovaIva()
{
	PrimaryComponentTick.bCanEverTick = true;
	m_BTInstance = std::make_unique<GameAI::BT::BehaviorTree>();
}

void USurvivorBTComponent_PetrovaIva::SetRoot(std::unique_ptr<GameAI::BT::Node>&& Root)
{
	check(m_BTInstance);
	m_BTInstance->SetRoot(std::move(Root));
}

void USurvivorBTComponent_PetrovaIva::BeginPlay()
{
	Super::BeginPlay();
}

void USurvivorBTComponent_PetrovaIva::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!m_IsRunning || !m_BTInstance) return;

	AAIController* pController = Cast<AAIController>(GetOwner());
	if (!pController) return;

	ASurvivorPawn* pSurvivor = Cast<ASurvivorPawn>(pController->GetPawn());
	UBlackboardComponent* pBB = pController->GetBlackboardComponent();

	if (pSurvivor)
	{
		m_BTInstance->Update(DeltaTime, *pSurvivor, pBB);
	}
}

void USurvivorBTComponent_PetrovaIva::StartLogic()
{
	Super::StartLogic();
	if (!m_BTInstance) return;

	AAIController* pController = Cast<AAIController>(GetOwner());
	if (!pController) return;

	ASurvivorPawn* pSurvivor = Cast<ASurvivorPawn>(pController->GetPawn());
	UBlackboardComponent* pBB = pController->GetBlackboardComponent();
	if (pSurvivor)
	{
		m_BTInstance->Start(*pSurvivor, pBB);
		m_IsRunning = true;
	}
}

void USurvivorBTComponent_PetrovaIva::StopLogic(const FString& Reason)
{
	if (!m_BTInstance) return;

	AAIController* pController = Cast<AAIController>(GetOwner());
	if (!pController) return;

	ASurvivorPawn* pSurvivor = Cast<ASurvivorPawn>(pController->GetPawn());
	UBlackboardComponent* pBB = pController->GetBlackboardComponent();
	if (pSurvivor)
	{
		m_BTInstance->Stop(*pSurvivor, pBB);
	}

	m_IsRunning = false;
}

bool USurvivorBTComponent_PetrovaIva::IsRunning() const
{
	return m_IsRunning;
}