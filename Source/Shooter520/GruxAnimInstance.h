// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GruxAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class SHOOTER520_API UGruxAnimInstance : public UAnimInstance
{
public:

	GENERATED_BODY()

	UFUNCTION(BlueprintCallable)
	void UpdateAnimationProperties(float DeltatTime);

private:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Movement, meta=(AllowPrivateAccess="true"))
	float Speed;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	class AEnermy* Enemy;
};
