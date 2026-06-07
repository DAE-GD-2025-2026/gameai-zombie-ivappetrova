#pragma once

#include "SteeringHelpers_PetrovaIva.h"

class ASurvivorPawn;

// Use FSurvivorSteeringProxy for steering math to query what it needs from the pawn without requiring ASteeringAgent or touching protected members.
struct FSurvivorSteeringProxy
{
	explicit FSurvivorSteeringProxy(ASurvivorPawn& InPawn);

	FVector2D GetPosition()         const;
	FVector2D GetLinearVelocity()   const;   // from APawn::GetVelocity()
	float     GetRotationDeg()      const;   // yaw in degrees
	float     GetMaxLinearSpeed()   const;   // from UFloatingPawnMovement
	void      SetMaxLinearSpeed(float NewSpeed);

private:
	ASurvivorPawn& Pawn;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
class ISteeringBehavior
{
public:
	ISteeringBehavior() = default;
	virtual ~ISteeringBehavior() = default;

	virtual SteeringOutput CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy) = 0;

	template<class T, std::enable_if_t<std::is_base_of_v<ISteeringBehavior, T>>* = nullptr>
	T* As() { return static_cast<T*>(this); }

	FTargetData m_Target;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// STEERING BEHAVIORS
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Seek : public ISteeringBehavior
{
public:
	virtual SteeringOutput CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Flee : public ISteeringBehavior
{
public:
	virtual SteeringOutput CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Arrive : public ISteeringBehavior
{
public:
	virtual SteeringOutput CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy) override;

	float m_TargetRadius{ 200.f };
	float m_SlowRadius{ 400.f };

private:
	float m_OriginalMaxSpeed{ 400.f };
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Face : public ISteeringBehavior
{
public:
	virtual SteeringOutput CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Pursuit : public ISteeringBehavior
{
public:
	virtual SteeringOutput CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy) override;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Evade : public ISteeringBehavior
{
public:
	virtual SteeringOutput CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy) override;

	float m_EvadeRadius{ 600.f };
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Wander : public Seek
{
public:
	virtual SteeringOutput CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy) override;

	float m_OffsetDistance{ 6.f };
	float m_Radius{ 4.f };
	float m_MaxAngleChange{ 45.f * PI / 180.f };

protected:
	float m_WanderAngle{ 0.f };
};