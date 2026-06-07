#pragma once

#include "CoreMinimal.h"
#include "SteeringHelpers_PetrovaIva.generated.h"

USTRUCT(BlueprintType)
struct FSteeringParams final
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D Position;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Orientation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D LinearVelocity;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AngularVelocity;

	explicit FSteeringParams( const FVector2D& position = FVector2D::ZeroVector, float orientation = 0.f,
							  const FVector2D& linearVel = FVector2D::ZeroVector, float angularVel = 0.f);

	void Clear();

	FSteeringParams(const FSteeringParams& other);
	FSteeringParams& operator=(const FSteeringParams& other);
	bool operator==(const FSteeringParams& other) const;
	bool operator!=(const FSteeringParams& other) const;
};

using FTargetData = FSteeringParams;

struct SteeringOutput final
{
	FVector2D LinearVelocity{ FVector2D::ZeroVector };
	float AngularVelocity{ 0.f };
	bool IsValid{ true };

	SteeringOutput(const FVector2D& linearVelocity = FVector2D::ZeroVector, float angularVelocity = 0.f);

	SteeringOutput& operator=(const SteeringOutput& other);
	SteeringOutput& operator+=(const SteeringOutput& other);
	SteeringOutput& operator*=(float f);
	SteeringOutput& operator/=(float f);
};