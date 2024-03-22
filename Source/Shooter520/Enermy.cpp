// Fill out your copyright notice in the Description page of Project Settings.


#include "Enermy.h"
#include "GruxAnimInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "particles/particleSystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "EnemyController.h"
#include "behaviorTree/BlackboardComponent.h"
#include "Components/SphereComponent.h"
#include "ShooterCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/SkeletalMeshSocket.h"


// Sets default values
AEnermy::AEnermy() : 
	Health(100.0f), 
	MaxHealth(100.0f), 
	HealthBarDisplayTime(4.0f), 
	HitReactTimeMin(0.5f), 
	HitReactTimeMax(3.0f),
	bCanHitReact(true), 
	HitNumberDestroyTime(1.5f), 
	bStunned(false), 
	StunChance(0.5f),
	AttackLFast(TEXT("AttackLFast")),
	AttackRFast(TEXT("AttackRFast")),
	AttackL(TEXT("AttackL")),
	AttackR(TEXT("AttackR")), 
	BaseDamage(20.0f),
	LeftWeaponSocket(TEXT("FX_Trail_L_01")), 
	RightWeaponSocket(TEXT("FX_Trail_R_01")) 
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	AgroSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AgroSphere"));
	AgroSphere->SetupAttachment(GetRootComponent());

	CombatRangeSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CombatRangeSphere"));
	CombatRangeSphere->SetupAttachment(GetRootComponent());

	LeftWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Left Weapon Box"));
	LeftWeaponCollision->SetupAttachment(GetMesh(), FName("LeftWeaponBone"));
	RightWeaponCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("Right Weapon Box"));
	RightWeaponCollision->SetupAttachment(GetMesh(), FName("RightWeaponBone"));
}

// Called when the game starts or when spawned
void AEnermy::BeginPlay()
{
	Super::BeginPlay();

	AgroSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnermy::AgroSphereOverlap);
	CombatRangeSphere->OnComponentBeginOverlap.AddDynamic(this, &AEnermy::CombatRangeOverlap);
	CombatRangeSphere->OnComponentEndOverlap.AddDynamic(this, &AEnermy::CombatRangeEndOverlap);

	// Weapon.
	LeftWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnermy::OnLeftWeaponOverlap);
	RightWeaponCollision->OnComponentBeginOverlap.AddDynamic(this, &AEnermy::OnRightWeaponOverlap);

	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftWeaponCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	LeftWeaponCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	LeftWeaponCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);

	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightWeaponCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	RightWeaponCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	RightWeaponCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);


	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);

	EnemyController = Cast<AEnemyController>(GetController());

	const FVector WorldPatrolPoint = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint);
	DrawDebugSphere(GetWorld(), WorldPatrolPoint, 25.0f, 12, FColor::Red, true);
	const FVector WorldPatrolPoint2 = UKismetMathLibrary::TransformLocation(GetActorTransform(), PatrolPoint2);
	DrawDebugSphere(GetWorld(), WorldPatrolPoint2, 25.0f, 12, FColor::Red, true);

	if(EnemyController)
	{
		EnemyController->GetBlackboardComponent()->SetValueAsVector(TEXT("PatrolPoint"), WorldPatrolPoint);
		EnemyController->GetBlackboardComponent()->SetValueAsVector(TEXT("PatrolPoint2"), WorldPatrolPoint2);
		EnemyController->RunBehaviorTree(BehaviorTree);
	}
}

void AEnermy::SetStunned(bool Stunned)
{
	bStunned = Stunned;
	if(EnemyController)
		EnemyController->GetBlackboardComponent()->SetValueAsBool(TEXT("Stunned"), bStunned);
}

// Called every frame
void AEnermy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateHitNumbers();
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

	const float Stunned = FMath::FRandRange(.0f, 1.0f);
	if(Stunned <= StunChance)
	{
		PlayHitMontage(FName("HitReactFront"));
		SetStunned(true);
	}
}

float AEnermy::TakeDamage(float DamageAmount, FDamageEvent const & DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if(Health-DamageAmount <= .0f)
	{
		Health = .0f;
		Die();
	}
	else 
		Health -= DamageAmount;
	
	return DamageAmount;
}

void AEnermy::ShowHealthBar_Implementation()
{
	GetWorldTimerManager().ClearTimer(HealthBarTimer);
	GetWorldTimerManager().SetTimer(HealthBarTimer, this, &AEnermy::HideHealthBar, HealthBarDisplayTime);
}

void AEnermy::Die()
{
	HideHealthBar();
}

void AEnermy::PlayHitMontage(FName Section, float PlayRate)
{
	if(bCanHitReact)
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();

		if(AnimInstance)
		{
			AnimInstance->Montage_Play(HitMontage, PlayRate);
			AnimInstance->Montage_JumpToSection(Section, HitMontage);
		}
		bCanHitReact = false;
		const float HitReactTime = FMath::FRandRange(HitReactTimeMin, HitReactTimeMax);
		GetWorldTimerManager().SetTimer(HitReactTimer, this, &AEnermy::ResetHitReactTimer, HitReactTime);
	}
}

void AEnermy::PlayAttackMontage(FName Section, float PlayRate)
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && AttackMontage)
	{
		AnimInstance->Montage_Play(AttackMontage, PlayRate);
		AnimInstance->Montage_JumpToSection(Section, AttackMontage);
	}
}

FName AEnermy::GetAttackSectionName()
{
	const int32 Section = FMath::RandRange(1, 4);
	switch(Section)
	{
	case 1:		return AttackLFast;
	case 2: 	return AttackRFast;
	case 3: 	return AttackL;
	default: 	return AttackR;
	}
}

void AEnermy::ResetHitReactTimer()
{
	bCanHitReact = true;
}

void AEnermy::StoreHitNumber(UUserWidget* HitNumber, FVector Location)
{
	HitNumbers.Add(HitNumber, Location);

	FTimerHandle HitNumberTimer;
	FTimerDelegate HitNumberDelegate;
	HitNumberDelegate.BindUFunction(this, FName("DestroyHitNumber"), HitNumber);
	GetWorld()->GetTimerManager().SetTimer(HitNumberTimer, HitNumberDelegate, HitNumberDestroyTime, false);
}

void AEnermy::DestroyHitNumber(UUserWidget* HitNumber)
{
	HitNumbers.Remove(HitNumber);
	HitNumber->RemoveFromParent();
}

void AEnermy::UpdateHitNumbers()
{
	for(auto& HitPair : HitNumbers)
	{
		UUserWidget* HitNumber = HitPair.Key;
		const FVector Location = HitPair.Value;
		FVector2D ScreenPosition;
		UGameplayStatics::ProjectWorldToScreen(GetWorld()->GetFirstPlayerController(), Location, ScreenPosition);
		HitNumber->SetPositionInViewport(ScreenPosition);
	}
}

void AEnermy::AgroSphereOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult)
{
	if(OtherActor == NULL)
		return;

	auto Character = Cast<AShooterCharacter>(OtherActor);	
	if(Character != NULL)
		EnemyController->GetBlackboardComponent()->SetValueAsObject(TEXT("Target"), Character);
}


void AEnermy::CombatRangeOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult)
{
	if(OtherActor == NULL)	return;
	auto ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
	if(ShooterCharacter == NULL)
		return;

	bInAttackRange = true;
	if(EnemyController)
		EnemyController->GetBlackboardComponent()->SetValueAsBool(TEXT("InAttackRange"), bInAttackRange);
}

	
void AEnermy::CombatRangeEndOverlap(UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex)
{
	if(OtherActor == NULL)	return;
	auto ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
	if(ShooterCharacter == NULL)
		return;

	bInAttackRange = false;
	if(EnemyController)
		EnemyController->GetBlackboardComponent()->SetValueAsBool(TEXT("InAttackRange"), bInAttackRange);
}

void AEnermy::DoDamage(AShooterCharacter* Victim)
{
	if(Victim == NULL)	return;
	
	UGameplayStatics::ApplyDamage(Victim, BaseDamage, EnemyController, this, UDamageType::StaticClass());
	if(Victim->GetMeleeImpactSound())
	{
		UGameplayStatics::PlaySoundAtLocation(this, Victim->GetMeleeImpactSound(), GetActorLocation());
	}
}

void AEnermy::SpawnBlood(AShooterCharacter* Victim, FName SocketName)
{
	const USkeletalMeshSocket* TipSocket = GetMesh()->GetSocketByName(SocketName);
	if(TipSocket && Victim)
	{
		const FTransform SocketTransform = TipSocket->GetSocketTransform(GetMesh());
		if(Victim->GetBloodParticles())
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), Victim->GetBloodParticles(), SocketTransform);
		}
	}
}

void AEnermy::OnLeftWeaponOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult)
{
	auto Character = Cast<AShooterCharacter>(OtherActor);
	if(Character)
	{
		DoDamage(Character);
		SpawnBlood(Character, LeftWeaponSocket);
	}
}

void AEnermy::OnRightWeaponOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult)
{
	auto Character = Cast<AShooterCharacter>(OtherActor);
	if(Character)
	{
		DoDamage(Character);
		SpawnBlood(Character, RightWeaponSocket);
	}
}


void AEnermy::ActivateLeftWeapon()
{
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	//LeftWeaponCollision->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	//LeftWeaponCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	//LeftWeaponCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
}
void AEnermy::DeactivateLeftWeapon()
{
	LeftWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void AEnermy::ActivateRightWeapon()
{
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}
void AEnermy::DeactivateRightWeapon()
{
	RightWeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}