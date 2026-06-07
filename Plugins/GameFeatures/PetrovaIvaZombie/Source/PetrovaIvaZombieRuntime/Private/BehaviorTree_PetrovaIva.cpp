#include "BehaviorTree_PetrovaIva.h"

namespace GameAI::BT
{
	void Composite::AddChild(std::unique_ptr<Node>&& Child)
	{
		m_pChildren.push_back(std::move(Child));
	}

	void Sequence::OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_CurrentChildIndex = 0;
		if (!m_pChildren.empty())
		{
			m_pChildren[0]->OnEnter(Survivor, Blackboard);
		}
	}

	ENodeStatus Sequence::Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		for (size_t index{}; index < m_pChildren.size(); ++index)
		{
			ENodeStatus status = m_pChildren[index]->Tick(DeltaTime, Survivor, Blackboard);

			if (status == ENodeStatus::Failed)
			{
				if (index != m_CurrentChildIndex)
				{
					m_pChildren[m_CurrentChildIndex]->OnExit(Survivor, Blackboard);
					m_CurrentChildIndex = 0;
				}
				return ENodeStatus::Failed;
			}

			if (status == ENodeStatus::Running)
			{
				if (index != m_CurrentChildIndex)
				{
					m_pChildren[m_CurrentChildIndex]->OnExit(Survivor, Blackboard);
					m_CurrentChildIndex = index;
					m_pChildren[m_CurrentChildIndex]->OnEnter(Survivor, Blackboard);
				}
				return ENodeStatus::Running;
			}
		}
		return ENodeStatus::Succeeded;
	}

	void Sequence::OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		if (m_CurrentChildIndex < m_pChildren.size())
		{
			m_pChildren[m_CurrentChildIndex]->OnExit(Survivor, Blackboard);
		}
		m_CurrentChildIndex = 0;
	}

	void Selector::OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		m_CurrentChildIndex = 0;
		if (!m_pChildren.empty())
		{
			m_pChildren[0]->OnEnter(Survivor, Blackboard);
		}
	}

	ENodeStatus Selector::Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		for (size_t index{}; index < m_CurrentChildIndex; ++index)
		{
			ENodeStatus higherStatus = m_pChildren[index]->Tick(DeltaTime, Survivor, Blackboard);
			if (higherStatus != ENodeStatus::Failed)
			{
				m_pChildren[m_CurrentChildIndex]->OnExit(Survivor, Blackboard);
				m_CurrentChildIndex = index;
				m_pChildren[m_CurrentChildIndex]->OnEnter(Survivor, Blackboard);
				return ENodeStatus::Running;
			}
		}

		while (m_CurrentChildIndex < m_pChildren.size())
		{
			ENodeStatus status = m_pChildren[m_CurrentChildIndex]->Tick(DeltaTime, Survivor, Blackboard);

			if (status == ENodeStatus::Running)
				return ENodeStatus::Running;

			m_pChildren[m_CurrentChildIndex]->OnExit(Survivor, Blackboard);

			if (status == ENodeStatus::Succeeded)
				return ENodeStatus::Succeeded;

			++m_CurrentChildIndex;
			if (m_CurrentChildIndex < m_pChildren.size())
			{
				m_pChildren[m_CurrentChildIndex]->OnEnter(Survivor, Blackboard);
			}
		}

		return ENodeStatus::Failed;
	}

	void Selector::OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		if (m_CurrentChildIndex < m_pChildren.size())
		{
			m_pChildren[m_CurrentChildIndex]->OnExit(Survivor, Blackboard);
		}
		m_CurrentChildIndex = 0;
	}

	Condition::Condition(std::function<bool()> Predicate)
		: m_Predicate(std::move(Predicate))
	{}

	ENodeStatus Condition::Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		return (m_Predicate && m_Predicate()) ? ENodeStatus::Succeeded : ENodeStatus::Failed;
	}

	void BehaviorTree::SetRoot(std::unique_ptr<Node>&& InRoot)
	{
		m_pRoot = std::move(InRoot);
	}

	void BehaviorTree::Start(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		if (!m_pRoot) return;
		m_IsRunning = true;
		m_pRoot->OnEnter(Survivor, Blackboard);
	}

	void BehaviorTree::Update(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		if (!m_IsRunning || !m_pRoot) return;
		ENodeStatus status = m_pRoot->Tick(DeltaTime, Survivor, Blackboard);
		if (status != ENodeStatus::Running)
		{
			m_pRoot->OnExit(Survivor, Blackboard);
			m_pRoot->OnEnter(Survivor, Blackboard);
		}
	}

	void BehaviorTree::Stop(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard)
	{
		if (m_pRoot)
		{
			m_pRoot->OnExit(Survivor, Blackboard);
		}

		m_IsRunning = false;
	}
}