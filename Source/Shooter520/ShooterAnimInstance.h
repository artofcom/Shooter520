// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ShooterAnimInstance.generated.h"


UENUM(BlueprintType)
enum class EOffsetState : uint8 
{
	EOS_Aiming UMETA(DisplayName="Aiming"),
	EOS_Hip UMETA(DisplayName="Hip"),
	EOS_Reloading UMETA(DisplayName="Reloading"),
	EOS_InAir UMETA(DisplayName="InAir"),
	
	EOS_MAX UMETA(DisplayName="DefaultMAX")
};


/**
 * 
 */
UCLASS()
class SHOOTER520_API UShooterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:

	UShooterAnimInstance();

	virtual void NativeInitializeAnimation() override;


	// Set All the parameters - Called every frame.
	UFUNCTION(BlueprintCallable)
	void UpdateAnimationProperties(float DeltatTime);


protected:

	void TurnInPlace();

	void Lean(float DeltaTime);

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	class AShooterCharacter* ShooterCharacter;

	// Animation Referenced Data Field.-----------------------------------//
	//
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float Speed;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	bool bIsInAir;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	bool bIsAccelerating;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float MovementOffsetYaw;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float LastMovementOffsetYaw;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	bool bAiming;

	// Character Yaw Rotation. 
	float TIPCharacterYaw;			// TIP - Turn In Place.
	float TIPCharacterYawLastFrame;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turn In Place", meta=(AllowPrivateAccess="true"))
	float RootYawOffset;

	float RotationCurve;

	float RotationCurveLastFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turn In Place", meta=(AllowPrivateAccess="true"))
	float Pitch;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turn In Place", meta=(AllowPrivateAccess="true"))
	bool bReloading;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Turn In Place", meta=(AllowPrivateAccess="true"))
	EOffsetState OffsetState;

	FRotator CharacterRotation;
	FRotator CharacterRotationLastFrame;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Lean, meta=(AllowPrivateAccess="true"))
	float YawDelta;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Crouching, meta=(AllowPrivateAccess="true"))
	bool bCrouching;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Combat, meta=(AllowPrivateAccess="true"))
	float RecoilWeight;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Combat, meta=(AllowPrivateAccess="true"))
	bool bTurningInPlace;
};
