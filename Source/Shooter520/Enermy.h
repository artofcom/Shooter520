// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "BulletHitInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Enermy.generated.h"

UCLASS()
class SHOOTER520_API AEnermy : public ACharacter, public IBulletHitInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AEnermy();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintNativeEvent)
	void ShowHealthBar();
	void ShowHealthBar_Implementation();

	UFUNCTION(BlueprintImplementableEvent)
	void HideHealthBar();

	void Die();

	void PlayHitMontage(FName Section, float PlayRate = 1.0f);

private:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Combat, meta=(AllowPrivateAccess="true"))
	class UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Combat, meta=(AllowPrivateAccess="true"))
	class USoundCue* ImpactSound;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Combat, meta=(AllowPrivateAccess="true"))
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Combat, meta=(AllowPrivateAccess="true"))
	float MaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Combat, meta=(AllowPrivateAccess="true"))
	FString HeadBone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Combat, meta=(AllowPrivateAccess="true"))
	float HealthBarDisplayTime;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Combat, meta=(AllowPrivateAccess="true"))
	FTimerHandle HealthBarTimer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Combat, meta=(AllowPrivateAccess="true"))
	UAnimMontage* HitMontage;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void BulletHit_Implementation(FHitResult HitResult) override;

	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const & DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;


	FORCEINLINE FString GetHeadBone() const { return HeadBone; }
};
