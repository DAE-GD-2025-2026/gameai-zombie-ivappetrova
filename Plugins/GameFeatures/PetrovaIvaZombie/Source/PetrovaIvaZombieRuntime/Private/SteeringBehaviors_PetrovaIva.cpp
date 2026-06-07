#include "SteeringBehaviors_PetrovaIva.h"

#include "Survivor/SurvivorPawn.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "DrawDebugHelpers.h"

//////////////////////////////////////////////////////////////////////////////
// FSurvivorSteeringProxy
//////////////////////////////////////////////////////////////////////////////

FSurvivorSteeringProxy::FSurvivorSteeringProxy(ASurvivorPawn& InPawn)
	: Pawn(InPawn)
{
}

FVector2D FSurvivorSteeringProxy::GetPosition() const
{
	FVector location = Pawn.GetActorLocation();
	return FVector2D(location.X, location.Y);
}

FVector2D FSurvivorSteeringProxy::GetLinearVelocity() const
{
	FVector velocity = Pawn.GetVelocity();
	return FVector2D(velocity.X, velocity.Y);
}

float FSurvivorSteeringProxy::GetRotationDeg() const
{
	return Pawn.GetActorRotation().Yaw;
}

float FSurvivorSteeringProxy::GetMaxLinearSpeed() const
{
	if (UFloatingPawnMovement* FPM = Pawn.GetComponentByClass<UFloatingPawnMovement>()) return FPM->MaxSpeed;
	return 400.f;
}

void FSurvivorSteeringProxy::SetMaxLinearSpeed(float NewSpeed)
{
	if (UFloatingPawnMovement* FPM = Pawn.GetComponentByClass<UFloatingPawnMovement>())
	{
		FPM->MaxSpeed = NewSpeed;
	}
}

//////////////////////////////////////////////////////////////////////////////
// Seek
//////////////////////////////////////////////////////////////////////////////
SteeringOutput Seek::CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy)
{
	SteeringOutput steering{};
	FVector2D toTarget = Target.Position - Proxy.GetPosition();
	steering.LinearVelocity = (toTarget.Size() < 1.f) ? FVector2D::ZeroVector : toTarget;
	return steering;
}

//////////////////////////////////////////////////////////////////////////////
// Flee
//////////////////////////////////////////////////////////////////////////////
SteeringOutput Flee::CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy)
{
	SteeringOutput steering{};
	steering.LinearVelocity = Proxy.GetPosition() - Target.Position;
	return steering;
}

//////////////////////////////////////////////////////////////////////////////
// Arrive
//////////////////////////////////////////////////////////////////////////////
SteeringOutput Arrive::CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy)
{
	SteeringOutput steering{};
	FVector2D toTarget = Target.Position - Proxy.GetPosition();
	float distance = toTarget.Size();

	float currentMax = Proxy.GetMaxLinearSpeed();
	if (currentMax > 1.f)
		m_OriginalMaxSpeed = currentMax;

	if (distance < m_TargetRadius)
	{
		steering.LinearVelocity = FVector2D::ZeroVector;
	}
	else if (distance < m_SlowRadius)
	{
		float speedScale = distance / m_SlowRadius;
		steering.LinearVelocity = toTarget.GetSafeNormal() * (m_OriginalMaxSpeed * speedScale);
	}
	else
	{
		steering.LinearVelocity = toTarget;
	}

	return steering;
}

//////////////////////////////////////////////////////////////////////////////
// Face
//////////////////////////////////////////////////////////////////////////////
SteeringOutput Face::CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy)
{
	SteeringOutput steering{};
	FVector2D toTarget = Target.Position - Proxy.GetPosition();
	const float ORBIT_RADIUS = 200.f;

	if (toTarget.Size() > ORBIT_RADIUS)
	{
		steering.LinearVelocity = toTarget;
	}
	else
	{
		FVector2D perp(-toTarget.Y, toTarget.X);
		steering.LinearVelocity = perp.GetSafeNormal() * Proxy.GetMaxLinearSpeed();
	}

	float desiredOrientation = FMath::RadiansToDegrees(FMath::Atan2(toTarget.Y, toTarget.X));
	float angleDiff = desiredOrientation - Proxy.GetRotationDeg();

	while (angleDiff > 180.f) angleDiff -= 360.f;
	while (angleDiff < -180.f) angleDiff += 360.f;

	steering.AngularVelocity = angleDiff;
	return steering;
}

//////////////////////////////////////////////////////////////////////////////
// Pursuit
//////////////////////////////////////////////////////////////////////////////
SteeringOutput Pursuit::CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy)
{
	SteeringOutput steering{};
	FVector2D toTarget = Target.Position - Proxy.GetPosition();
	float t = toTarget.Size() / (Proxy.GetMaxLinearSpeed() + 0.01f);
	FVector2D futurePos = Target.Position + Target.LinearVelocity * t;

	steering.LinearVelocity = futurePos - Proxy.GetPosition();
	return steering;
}

//////////////////////////////////////////////////////////////////////////////
// Evade
//////////////////////////////////////////////////////////////////////////////
SteeringOutput Evade::CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy)
{
	SteeringOutput steering{};
	FVector2D toTarget = Target.Position - Proxy.GetPosition();

	if (toTarget.Size() > m_EvadeRadius)
	{
		steering.IsValid = false;
		return steering;
	}

	float t = toTarget.Size() / (Proxy.GetMaxLinearSpeed() + 0.01f);
	FVector2D futurePos = Target.Position + Target.LinearVelocity * t;

	steering.LinearVelocity = Proxy.GetPosition() - futurePos;
	steering.IsValid = true;
	return steering;
}

//////////////////////////////////////////////////////////////////////////////
// Wander
//////////////////////////////////////////////////////////////////////////////
SteeringOutput Wander::CalculateSteering(float DeltaT, FSurvivorSteeringProxy& Proxy)
{
	SteeringOutput steering{};

	const float CIRCLE_DISTANCE = 100.f;
	const float CIRCLE_RADIUS = 50.f;
	const float ANGLE_CHANGE = 45.f;

	m_WanderAngle += FMath::RandRange(-1.f, 1.f) * ANGLE_CHANGE * DeltaT;

	FVector2D velocity = Proxy.GetLinearVelocity();
	float dirAngle = (velocity.SizeSquared() > 0.01f)
		? FMath::Atan2(velocity.Y, velocity.X)
		: FMath::DegreesToRadians(Proxy.GetRotationDeg());

	FVector2D forward(FMath::Cos(dirAngle), FMath::Sin(dirAngle));
	FVector2D circleCenter = Proxy.GetPosition() + forward * CIRCLE_DISTANCE;

	float targetAngle = dirAngle + FMath::DegreesToRadians(m_WanderAngle);
	FVector2D displacement(FMath::Cos(targetAngle), FMath::Sin(targetAngle));
	displacement *= CIRCLE_RADIUS;

	FVector2D targetPoint = circleCenter + displacement;
	steering.LinearVelocity = targetPoint - Proxy.GetPosition();

	return steering;
}