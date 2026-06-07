#include "SteeringHelpers_PetrovaIva.h"

///////////////////////////////////////////////////////////////////////////// FSteeringParams

FSteeringParams::FSteeringParams( const FVector2D& position, float orientation, const FVector2D& linearVel, float angularVel)
	: Position(position)
	, Orientation(orientation)
	, LinearVelocity(linearVel)
	, AngularVelocity(angularVel)
{
}

void FSteeringParams::Clear()
{
	Position = FVector2D::ZeroVector;
	LinearVelocity = FVector2D::ZeroVector;
	Orientation = 0.f;
	AngularVelocity = 0.f;
}

FSteeringParams::FSteeringParams(const FSteeringParams& other)
	: Position(other.Position)
	, Orientation(other.Orientation)
	, LinearVelocity(other.LinearVelocity)
	, AngularVelocity(other.AngularVelocity)
{}

FSteeringParams& FSteeringParams::operator=(const FSteeringParams& other)
{
	Position = other.Position;
	Orientation = other.Orientation;
	LinearVelocity = other.LinearVelocity;
	AngularVelocity = other.AngularVelocity;
	return *this;
}

bool FSteeringParams::operator==(const FSteeringParams& other) const
{
	return Position == other.Position && Orientation == other.Orientation
		&& LinearVelocity == other.LinearVelocity && AngularVelocity == other.AngularVelocity;
}

bool FSteeringParams::operator!=(const FSteeringParams& other) const
{
	return !(*this == other);
}

/////////////////////////////////////////////////////////////////////////////// SteeringOutput

SteeringOutput::SteeringOutput(const FVector2D& linearVelocity, float angularVelocity)
	: LinearVelocity(linearVelocity)
	, AngularVelocity(angularVelocity)
{
}

SteeringOutput& SteeringOutput::operator=(const SteeringOutput& other)
{
	LinearVelocity = other.LinearVelocity;
	AngularVelocity = other.AngularVelocity;
	IsValid = other.IsValid;
	return *this;
}

SteeringOutput& SteeringOutput::operator+=(const SteeringOutput& other)
{
	LinearVelocity += other.LinearVelocity;
	AngularVelocity += other.AngularVelocity;
	return *this;
}

SteeringOutput& SteeringOutput::operator*=(float f)
{
	LinearVelocity *= f;
	AngularVelocity *= f;
	return *this;
}

SteeringOutput& SteeringOutput::operator/=(float f)
{
	LinearVelocity /= f;
	AngularVelocity /= f;
	return *this;
}