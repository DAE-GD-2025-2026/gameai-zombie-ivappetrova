#pragma once

#include <functional>
#include <memory>
#include <vector>

class ASurvivorPawn;
class UBlackboardComponent;

namespace GameAI::BT
{
	enum class ENodeStatus : uint8
	{
		Running,
		Succeeded,
		Failed
	};

	class Node
	{
	public:
		virtual ~Node() = default;

		virtual void OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) {}
		virtual ENodeStatus Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) = 0;
		virtual void OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) {}
	};

	class Composite : public Node
	{
	public:
		void AddChild(std::unique_ptr<Node>&& Child);

	protected:
		std::vector<std::unique_ptr<Node>> m_pChildren;
	};

	class Sequence : public Composite
	{
	public:
		virtual void OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual ENodeStatus Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual void OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;

	private:
		size_t m_CurrentChildIndex{ 0 };
	};

	class Selector : public Composite
	{
	public:
		virtual void OnEnter(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual ENodeStatus Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;
		virtual void OnExit(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;

	private:
		size_t m_CurrentChildIndex{ 0 };
	};

	class Action : public Node {};

	class Condition : public Node
	{
	public:
		explicit Condition(std::function<bool()> Predicate);
		virtual ENodeStatus Tick(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard) override;

	private:
		std::function<bool()> m_Predicate;
	};

	class BehaviorTree
	{
	public:
		BehaviorTree() = default;
		~BehaviorTree() = default;

		void SetRoot(std::unique_ptr<Node>&& Root);
		void Start(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard);
		void Update(float DeltaTime, ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard);
		void Stop(ASurvivorPawn& Survivor, UBlackboardComponent* Blackboard);

		bool IsRunning() const { return m_IsRunning; }

	private:
		std::unique_ptr<Node> m_pRoot;
		bool m_IsRunning{ false };
	};
}