// Fill out your copyright notice in the Description page of Project Settings.


#include "Enermy.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "particles/particleSystemComponent.h"

// Sets default values
AEnermy::AEnermy() : 
	Health(100.0f), 
	MaxHealth(100.0f), 
	HealthBarDisplayTime(4.0f)
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AEnermy::BeginPlay()
{
	Super::BeginPlay();

	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	
}

// Called every frame
void AEnermy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AEnermy::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AEnermy::BulletHit_Implementation(FHitResult HitResult)
{
	if(ImpactSound != NULL)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}

	if(ImpactParticles != NULL)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, HitResult.Location, FRotator(.0f), true);
	}

	ShowHealthBar();
}

float AEnermy::TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if(Health-DamageAmount <= .0f)
		Health = .0f;
	else 
		Health -= DamageAmount;
	
	return DamageAmount;
}

void AEnermy::ShowHealthBar_Implementation()
{
	GetWorldTimerManager().ClearTimer(HealthBarTimer);
	GetWorldTimerManager().SetTimer(HealthBarTimer, this, &AEnermy::HideHealthBar, HealthBarDisplayTime);
}