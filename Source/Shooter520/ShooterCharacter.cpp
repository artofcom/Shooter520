// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/characterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"
#include "particles/particleSystemComponent.h"
#include "Item.h"
#include "Components/WidgetComponent.h"
#include "Weapon.h"
#include "Components/SphereComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include <utility>
#include "Ammo.h"

//#include <future>

// Constructor -------------------------------------------------//
//
AShooterCharacter::AShooterCharacter():
	// Turning/Looking up
	BaseTurnRate(45.0f), 
	BaseLookUpRate(45.0f),

	HipTurnRate(90.0f),
	HipLookUpRate(90.0f),
	AimingTurnRate(20.0f),
	AimingLookUpRate(20.0f),
	
	// Mouse sensitivity
	MouseHipTurnRate(1.0f),
	MouseHipLookUpRate(1.0f),
	MouseAimingTurnRate(0.6f),
	MouseAimingLookUpRate(0.6f),

	//
	bAiming(false), 

	// Camera FOV values.
	CameraDefaultFOV(.0f), 
	CameraZoomedFOV(25.0f), 
	CameraCurrentFOV(.0f), 
	ZoomInterpSpeed(20.0f), 

	CrosshairSpreadMultiplier(0.0f),
	CrosshairVelocityFactor(.0f),
	CrosshairInAirFactor(.0f),
	CrosshairAimFactor(.0f),
	CrosshairShootingFactor(.0f), 

	ShootTimeDuration(0.05f),
	bFiringBullet(false), 

	bFireButtonPressed(false),
	bShouldFire(true),
	AutomaticFireRate(0.1f), 

	bShouldTraceForItems(false),
	OverlappedItemCount(0), 
	TraceHitItemLastFrame(NULL), 

	CameraInterpDistance(250.0f),
	CameraInterpElevation(64.0f), 

	Starting9mmAmmo(85),
	StartingARAmmo(120), 

	CombatState(ECombatState::ECS_Unoccupied),
	bCrouching(false), 

	BaseMovementSpeed(650.0f),
	CrouchMovementSpeed(300.0f), 

	StandingCapsuleHalfHeight(88.0f),
	CrouchingCapsuleHalfHeight(44.0f),

	BaseGroundFriction(2.0f),
	CrouchingGroundFriction(100.0f), 
	bAimingButtonPressed(false), 

	bShouldPlayPickupSound(true),
	bShouldPlayEquipSound(true), 

	PickupSoundResetTime(0.2f),
	EquipSoundResetTime(0.2)
{

 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create CameraBoom.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 180.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(.0f, 50.0f, 70.0f);

	// Create Follow Camera.
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Don't Rotate when the controller rotates. Let the controller only affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true; // !!!
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // character moves in the direction of input.
	GetCharacterMovement()->RotationRate = FRotator(.0f, 540.0f, .0f);
	GetCharacterMovement()->JumpZVelocity = 600.0f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create hand for magazine.
	HandSceneCompnent = CreateDefaultSubobject<USceneComponent>(TEXT("HandSceneComp"));


	WeaponInterpComp = CreateDefaultSubobject<USceneComponent>(TEXT("Weapon Interpolation Comp"));
	WeaponInterpComp->SetupAttachment(GetFollowCamera());

	InterpComp1 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Comp1"));
	InterpComp1->SetupAttachment(GetFollowCamera());
	InterpComp2 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Comp2"));
	InterpComp2->SetupAttachment(GetFollowCamera());
	InterpComp3 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Comp3"));
	InterpComp3->SetupAttachment(GetFollowCamera());
	InterpComp4 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Comp4"));
	InterpComp4->SetupAttachment(GetFollowCamera());
	InterpComp5 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Comp5"));
	InterpComp5->SetupAttachment(GetFollowCamera());
	InterpComp6 = CreateDefaultSubobject<USceneComponent>(TEXT("Interpolation Comp6"));
	InterpComp6->SetupAttachment(GetFollowCamera());

}



// Engine Event Overriders -------------------------------------------------//
//
void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();
	if(FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraCurrentFOV = CameraDefaultFOV;
	}

	EquipWeapon( SpawnDefaultWeapon() );
	Inventory.Add(EquippedWeapon);

	EquippedWeapon->DisableCustomDepth();
	EquippedWeapon->DisableGlowMaterial();

	InitializeAmmoMap();
	GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;

	InitializeInterpLocations();
}
void AShooterCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CameraInterpZoom(DeltaTime);

	SetLookRates();
	 
	CalculateCrosshairSpread(DeltaTime);  

	TraceForItems();

	InterpCapsuleHalfHeight(DeltaTime);
}

void AShooterCharacter::TraceForItems()
{
	//bool showWidget = false;
	if(bShouldTraceForItems)
	{
		FHitResult ItemTraceResult;
		FVector HitLocation;
		TraceUnderCrosshairs(ItemTraceResult, HitLocation);
		if(ItemTraceResult.bBlockingHit)
		{
			// UE_LOG(LogTemp, Warning, TEXT("Crosshair blocked by something..."));
			TraceHitItem  = Cast<AItem>(ItemTraceResult.GetActor());
			if(TraceHitItem && TraceHitItem->GetPickupWidget())
			{
				// UE_LOG(LogTemp, Warning, TEXT("Crosshair blocked by AItem..."));
				//showWidget = true;
				TraceHitItem->GetPickupWidget()->SetVisibility( true );
				TraceHitItem->EnableCustomDepth();
			}
			if(TraceHitItemLastFrame)
			{
				if(TraceHitItemLastFrame != TraceHitItem)
				{
					TraceHitItemLastFrame->GetPickupWidget()->SetVisibility( false );
					TraceHitItemLastFrame->DisableCustomDepth();
				}
			}
			TraceHitItemLastFrame = TraceHitItem;
		}
	}
	else if(TraceHitItemLastFrame)
	{
		TraceHitItemLastFrame->GetPickupWidget()->SetVisibility( false );
		TraceHitItemLastFrame->DisableCustomDepth();
	}
}

void AShooterCharacter::SetLookRates()
{
	if(bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseLookUpRate = AimingLookUpRate;
	}
	else 
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}


// Crosshair -------------------------------------------------//
//
void AShooterCharacter::CalculateCrosshairSpread(float DeltaTime)
{
	FVector2D WalkSpeedRange{ 0.0f, 600.0f };
	FVector2D VelocityMultiplierRange{.0f, 1.0f};
	FVector Velocity { GetVelocity() };
	Velocity.Z = .0f;

	// Cross hair param Setting.
	CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplierRange, Velocity.Size());

	// 
	if(GetCharacterMovement()->IsFalling())
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.25f);
	else 
		CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, .0f, DeltaTime, 30.0f);

	if(bAiming)
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.45f, DeltaTime, 30.0f);
	else 
		CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, .0f, DeltaTime, 30.0f);

	if(bFiringBullet)
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.3f, DeltaTime, 60.0f);
	else 
		CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.0f, DeltaTime, 60.0f);


	// Running will make this bigger. 
	CrosshairSpreadMultiplier = 0.5f + CrosshairVelocityFactor + CrosshairInAirFactor - CrosshairAimFactor + 
							CrosshairShootingFactor ;
}
float AShooterCharacter::GetCrosshairSpreadMulplier() const 
{
	return CrosshairSpreadMultiplier;
}
void AShooterCharacter::StartCrosshairBulletFire()
{
	bFiringBullet = true;
	GetWorldTimerManager().SetTimer(CrosshairShootTimer, this, &AShooterCharacter::FinishCrosshairBulletFire, ShootTimeDuration);
}

void AShooterCharacter::InterpCapsuleHalfHeight(float DeltaTime)
{
	float TargetCapsuleHalfHeight;
	if(bCrouching)
		TargetCapsuleHalfHeight = CrouchingCapsuleHalfHeight;
	else 
		TargetCapsuleHalfHeight = StandingCapsuleHalfHeight;

	const float InterpHalfHeight = FMath::FInterpTo(GetCapsuleComponent()->GetScaledCapsuleHalfHeight(), TargetCapsuleHalfHeight, DeltaTime, 20.0f);
	
	const float DeltaCapsuleHalfHeight = InterpHalfHeight - GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	const FVector MeshOffset {.0f, .0f, -DeltaCapsuleHalfHeight };
	GetMesh()->AddLocalOffset(MeshOffset);

	GetCapsuleComponent()->SetCapsuleHalfHeight(InterpHalfHeight);
}

void AShooterCharacter::FinishCrosshairBulletFire()
{
	bFiringBullet = false;
}

void AShooterCharacter::CameraInterpZoom(float DeltaTime)
{
	if(bAiming)
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
	else 
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
	
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}


// Called to bind functionality to input
void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AShooterCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AShooterCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AShooterCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AShooterCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &AShooterCharacter::Turn);		// APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &AShooterCharacter::LookUp);	// APawn::AddControllerPitchInput);

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AShooterCharacter::Jump);
    PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);
    
	// PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireWeapon);
	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AShooterCharacter::FireButtonPressed);
	PlayerInputComponent->BindAction("FireButton", IE_Released, this, &AShooterCharacter::FireButtonReleased);

	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AShooterCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AShooterCharacter::AimingButtonReleased);

	PlayerInputComponent->BindAction("Select", IE_Pressed, this, &AShooterCharacter::SelectButtonPressed);
	PlayerInputComponent->BindAction("Select", IE_Released, this, &AShooterCharacter::SelectButtonReleased);

	PlayerInputComponent->BindAction("ReloadButton", IE_Pressed, this, &AShooterCharacter::ReloadButtonPressed);
	PlayerInputComponent->BindAction("Crouch", IE_Pressed, this, &AShooterCharacter::CrouchButtonPressed);
}

void AShooterCharacter::Jump()
{
	if(bCrouching)	
	{
		bCrouching = false;
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
	}
	else 			ACharacter::Jump();
}

int32 AShooterCharacter::GetInterpLocationIndex()
{
	int32 LowestIndex = 1;
	int32 LowestCount = INT_MAX;
	for(int32 i = 1; i < InterpLocations.Num(); i++)
	{
		if(InterpLocations[i].ItemCount < LowestCount)
		{
			LowestIndex = i;
			LowestCount = InterpLocations[i].ItemCount;
		}
	}
	return LowestIndex;
}

void AShooterCharacter::IncrementInterpLocItemCount(int32 Index, int32 Amount)
{
	if(Amount < -1 || Amount > 1)
		return;

	if(InterpLocations.Num() >= Index)
	{
		InterpLocations[Index].ItemCount += Amount;
	}
}

void AShooterCharacter::Turn(float Value)
{
	float TurnScaleFactor{};
	if(bAiming)
		TurnScaleFactor = MouseAimingTurnRate;
	else 
		TurnScaleFactor = MouseHipTurnRate;

	APawn::AddControllerYawInput(Value * TurnScaleFactor);
}
void AShooterCharacter::LookUp(float Value)
{
	float LookUpScaleFactor{};
	if(bAiming)
		LookUpScaleFactor = MouseAimingLookUpRate;
	else 
		LookUpScaleFactor = MouseHipLookUpRate;

	APawn::AddControllerPitchInput(Value * LookUpScaleFactor);
}

void AShooterCharacter::FireButtonPressed()
{
	bFireButtonPressed = true;
	FireWeapon();
}
void AShooterCharacter::FireButtonReleased()
{
	bFireButtonPressed = false;
}
void AShooterCharacter::StartFireTimer()
{
	CombatState = ECombatState::ECS_FireTimerInProgress;

	GetWorldTimerManager().SetTimer(AutoFireTimer, this, &AShooterCharacter::AutoFireReset, AutomaticFireRate);
}
void AShooterCharacter::AutoFireReset()
{
	CombatState = ECombatState::ECS_Unoccupied;
	
	if(WeaponHasAmmo())
	{
		if(bFireButtonPressed)
		{
			FireWeapon();
		}
	}
	else 
		ReloadWeapon();
}
bool AShooterCharacter::TraceUnderCrosshairs(FHitResult& OutHitResult, FVector& OutHitLocation)
{
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
		GEngine->GameViewport->GetViewportSize(ViewportSize);

	FVector2D CrosshairLocation(ViewportSize.X * 0.5f, ViewportSize.Y * 0.5f);
	// CrosshairLocation.Y -= 50.0f;
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld)
	{
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ CrosshairWorldPosition + CrosshairWorldDirection * 50'000.0f };
		OutHitLocation = End;
		GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECollisionChannel::ECC_Visibility);
		if(OutHitResult.bBlockingHit)
		{
			OutHitLocation = OutHitResult.Location;
			return true;	
		}
	}

	return false;
}
void AShooterCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != .0f))
	{
		// find out which way is forward
		const FRotator Rotation { Controller->GetControlRotation() };
		const FRotator YawRotation{ .0f, Rotation.Yaw, .0f };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };
		AddMovementInput(Direction, Value);
	}
}
void AShooterCharacter::PlayFireSound()
{
	// Making firing sound.
	if (FireSound != nullptr)
		UGameplayStatics::PlaySound2D(this, FireSound);
}
void AShooterCharacter::SendBullet()
{
	// Send Bullet.
	const USkeletalMeshSocket* BarrelSocket = 
		// GetMesh()->GetSocketByName("BarrelSocket");
		EquippedWeapon->GetItemMesh()->GetSocketByName("BarrelSocket");

	if (BarrelSocket != nullptr)
	{
		const FTransform SocketTransform = BarrelSocket->GetSocketTransform(
			//GetMesh());
			EquippedWeapon->GetItemMesh());
			
		if (MuzzleFlash != nullptr)
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		
		FVector BeamEnd;
		bool bBeamEnd = GetBeamEndLocation(SocketTransform.GetLocation(), BeamEnd);
		if (bBeamEnd)
		{
			// Hit Particle at the hit point.
			if (ImpactParticles != nullptr)
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamEnd);
			
			// Bullet Trail.
			if (BeamParticles != nullptr)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform.GetLocation());
				if (Beam != nullptr)
				{
					// Name 'Target' is from P_SmokeTrail_Faded particle.
					Beam->SetVectorParameter(FName("Target"), BeamEnd);
				}
			}
		}

		{
		// Gun Pointer Driven.
		/*
		// Line Tracing.
		FHitResult FireHit;
		const FVector Start{ SocketTransform.GetLocation() };
		const FQuat Rotation{ SocketTransform.GetRotation() };
		const FVector RotationAxis{ Rotation.GetAxisX() };
		const FVector End{ Start + RotationAxis * 50'000.0f };

		FVector BeamEndPoint{ End };

		GetWorld()->LineTraceSingleByChannel(FireHit, Start, End, ECollisionChannel::ECC_Visibility);
		if (FireHit.bBlockingHit)
		{
			// DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f);
			// DrawDebugPoint(GetWorld(), FireHit.Location, 5.0f, FColor::Red, false, 2.0f);

			BeamEndPoint = FireHit.Location;

			if (ImpactParticles != nullptr)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.Location);
			}
		}

		if (BeamParticles != nullptr)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);
			if (Beam != nullptr)
			{
				// Name 'Target' is from P_SmokeTrail_Faded particle.
				Beam->SetVectorParameter(FName("Target"), BeamEndPoint);
			}
		}*/
		}
	}
}
void AShooterCharacter::PlayGunFireMontage()
{
	// Blending Fire Anim.
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance != nullptr && HipFireMontage != nullptr)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

void AShooterCharacter::FireWeapon()
{
	// UE_LOG(LogTemp, Warning, TEXT("Fire Weapon."));

	if(EquippedWeapon == NULL)	return;
	if(CombatState != ECombatState::ECS_Unoccupied)
		return;

	if(WeaponHasAmmo())
	{
		PlayFireSound();
		SendBullet();
		PlayGunFireMontage();
		EquippedWeapon->DecrementAmmo();

		StartFireTimer();
	}
	// StartCrosshairBulletFire();
}

void AShooterCharacter::AimingButtonPressed()
{
	bAimingButtonPressed = true;
	if(CombatState != ECombatState::ECS_Reloading)
		Aim();
}
void AShooterCharacter::AimingButtonReleased()
{
	bAimingButtonPressed = false;
	StopAiming();
}

void AShooterCharacter::Aim()
{
	bAiming = true;
	GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
}
void AShooterCharacter::StopAiming()
{
	bAiming = false;
	if(!bCrouching)
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
}

void AShooterCharacter::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(WeaponToEquip)
	{
		const USkeletalMeshSocket* HandSocket = GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if(HandSocket)
		{
			HandSocket->AttachActor(WeaponToEquip, GetMesh());
		}
		EquippedWeapon = WeaponToEquip;
		EquippedWeapon->SetItemState(EItemState::EIS_Equipped);
	}
}

void AShooterCharacter::DropWeapon()
{
	if(EquippedWeapon)
	{
		FDetachmentTransformRules DetachmentTransformRules(EDetachmentRule::KeepWorld, true);
		EquippedWeapon->GetItemMesh()->DetachFromComponent(DetachmentTransformRules);

		EquippedWeapon->SetItemState(EItemState::EIS_Falling);
		EquippedWeapon->ThrowWeapon();
	}
}

AWeapon* AShooterCharacter::SpawnDefaultWeapon()
{
	if(DefaultWeaponClass)
	{
		AWeapon* DefaultWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass);
		return DefaultWeapon;
	}
	return NULL;
}
bool AShooterCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& OutBeamLocation)
{
	// Cross hair check.
	FHitResult CrosshairHitResult;
	bool bCrosshairHit = TraceUnderCrosshairs(CrosshairHitResult, OutBeamLocation);

	if(bCrosshairHit)
	{
		// Tentative beam location.
		OutBeamLocation = CrosshairHitResult.Location;
	}

	// Trace from gun barrel.
	FHitResult WeaponTraceHit;
	const FVector WeaponTraceStart{ MuzzleSocketLocation };
	const FVector StartToEnd{ OutBeamLocation - MuzzleSocketLocation };
	const FVector WeaponTraceEnd{ MuzzleSocketLocation + StartToEnd * 1.25f };
	GetWorld()->LineTraceSingleByChannel(WeaponTraceHit, WeaponTraceStart, WeaponTraceEnd, ECollisionChannel::ECC_Visibility);
	if (WeaponTraceHit.bBlockingHit)
	{
		OutBeamLocation = WeaponTraceHit.Location;
		return true;
	}
	return false;

	/*/

	// Crosshair Driven.
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
		GEngine->GameViewport->GetViewportSize(ViewportSize);

	FVector2D CrosshairLocation(ViewportSize.X * 0.5f, ViewportSize.Y * 0.5f);
	// CrosshairLocation.Y -= 50.0f;
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;

	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld)
	{
		// Trace from crosshair.
		FHitResult ScreenTraceHit;
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ CrosshairWorldPosition + CrosshairWorldDirection * 50'000.0f };
		OutBeamLocation = End;
		GetWorld()->LineTraceSingleByChannel(ScreenTraceHit, Start, End, ECollisionChannel::ECC_Visibility);
		if (ScreenTraceHit.bBlockingHit)
		{
			// DrawDebugLine(GetWorld(), Start, End, FColor::Red, false, 2.0f);
			// DrawDebugPoint(GetWorld(), FireHit.Location, 5.0f, FColor::Red, false, 2.0f);
			OutBeamLocation = ScreenTraceHit.Location;
		}
		return true;
	}
	return false;
	*/
}
	
// Called for side to side input.
void AShooterCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != .0f))
	{
		// find out which way is right
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ .0f, Rotation.Yaw, .0f };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };
		AddMovementInput(Direction, Value);
	}
}

// Called via input to turn at a given rate.
// Rate : This is a normalized rate, ie, 1.0f means 100% desired turn rate.
void AShooterCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

// Called via input to look up/down at a given rate.
void AShooterCharacter::LookUpAtRate(float Rate)
{
	// Calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AShooterCharacter::IncrementOverlappedItemCount(int8 Amount)
{
	if(OverlappedItemCount + Amount <= 0)
	{
		OverlappedItemCount = 0;
		bShouldTraceForItems = false;
	}
	else 
	{
		OverlappedItemCount += Amount;
		bShouldTraceForItems = true;
	}
}

void AShooterCharacter::SelectButtonPressed()
{
	if(TraceHitItem)
	{
		TraceHitItem->StartItemCurve(this);
	}
}

void AShooterCharacter::SelectButtonReleased()
{

}

void AShooterCharacter::CrouchButtonPressed()
{
	if(!GetCharacterMovement()->IsFalling())
	{
		bCrouching = !bCrouching;
	}

	if(bCrouching)
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchMovementSpeed;
		GetCharacterMovement()->GroundFriction = CrouchingGroundFriction;
	}
	else 
	{
		GetCharacterMovement()->MaxWalkSpeed = BaseMovementSpeed;
		GetCharacterMovement()->GroundFriction = BaseGroundFriction;
	}
}

void AShooterCharacter::SwapWeapon(AWeapon* WeaponToSwap)
{
	DropWeapon();
	EquipWeapon(WeaponToSwap);
	TraceHitItem = NULL;
	TraceHitItemLastFrame = NULL;
}

/*
FVector AShooterCharacter::GetCameraInterpLocation()
{
	const FVector CameraWorldLocation { FollowCamera->GetComponentLocation() };
	const FVector CameraForward { FollowCamera->GetForwardVector() };

	//CameraInterpDistance(250.0f),
	//CameraInterpElevation(64.0f)

	return CameraWorldLocation + CameraForward * CameraInterpDistance + FVector(.0f, .0f, CameraInterpElevation);
}*/

void AShooterCharacter::GetPickupItem(AItem* Item)
{
	//if(Item->GetEquipSound())
	//	UGameplayStatics::PlaySound2D(this, Item->GetEquipSound());
	if(Item)
		Item->PlayEquipSound();
	//

	auto Weapon = Cast<AWeapon>(Item);
	if(Weapon)
	{
		if(Inventory.Num() < INVENTORY_CAPACITY)
			Inventory.Add(Weapon);
		else 
			SwapWeapon(Weapon);
	}
	
	auto Ammo = Cast<AAmmo>(Item);
	if(Ammo)
		PickupAmmo(Ammo);
}

void AShooterCharacter::PickupAmmo(class AAmmo* Ammo)
{
	auto AmmoType = Ammo->GetAmmoType();
	if(AmmoMap.Find(AmmoType))
	{
		int32 AmmoCount = AmmoMap[ AmmoType ];
		AmmoCount += Ammo->GetItemCount();
		AmmoMap[AmmoType] = AmmoCount;
	}

	if(EquippedWeapon->GetAmmoType() == AmmoType)
	{
		if(EquippedWeapon->GetAmmo() == 0)
			ReloadWeapon();
	}

	Ammo->Destroy();
}

void AShooterCharacter::InitializeAmmoMap()
{
	AmmoMap.Add(EAmmoType::EAT_9mm, Starting9mmAmmo);
	AmmoMap.Add(EAmmoType::EAT_AR, StartingARAmmo);
}

bool AShooterCharacter::WeaponHasAmmo()
{
	if(EquippedWeapon == NULL)	return false;

	return EquippedWeapon->GetAmmo()>0;
}

void AShooterCharacter::ReloadButtonPressed()
{
	ReloadWeapon();
}

void AShooterCharacter::ReloadWeapon()
{
	if(CombatState != ECombatState::ECS_Unoccupied)
		return;
	
	if(EquippedWeapon == NULL)
		return;

	if(CarryingAmmo() && !EquippedWeapon->ClipIsFull())
	{
		if(bAiming)
			StopAiming();

		// TODO : Weapon type req.
		//UE_LOG(LogTemp, Warning, TEXT("Reload Step 2..."));

		CombatState = ECombatState::ECS_Reloading;
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		if(AnimInstance && ReloadMontage)
		{
			//UE_LOG(LogTemp, Warning, TEXT("Reload Step 3..."));
			AnimInstance->Montage_Play(ReloadMontage);
			AnimInstance->Montage_JumpToSection(EquippedWeapon->GetReloadMontageSection());
		}
	}
}

void AShooterCharacter::FinishReloading()
{
	CombatState = ECombatState::ECS_Unoccupied;

	if(bAimingButtonPressed)
		Aim();

	if(EquippedWeapon == NULL)
		return;

	const auto AmmoType { EquippedWeapon->GetAmmoType() }; 
	if(AmmoMap.Contains(AmmoType))
	{
		int32 CarriedAmmo = AmmoMap[ AmmoType ];
		const int32 MagEmptySpace = EquippedWeapon->GetMagazineCapacity()-EquippedWeapon->GetAmmo();
		if(MagEmptySpace > CarriedAmmo)
		{
			EquippedWeapon->ReloadAmmo(CarriedAmmo);
			CarriedAmmo = 0;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
		else 
		{
			EquippedWeapon->ReloadAmmo(MagEmptySpace);
			CarriedAmmo -= MagEmptySpace;
			AmmoMap.Add(AmmoType, CarriedAmmo);
		}
	}
}

bool AShooterCharacter::CarryingAmmo()
{
	if(EquippedWeapon == NULL)
		return false;

	auto AmmoType = EquippedWeapon->GetAmmoType();
	if(AmmoMap.Contains(AmmoType))
		return AmmoMap[AmmoType]>0;
	
	return false;
}

void AShooterCharacter::GrabClip()
{
	if(EquippedWeapon == NULL)
		return;
	if(HandSceneCompnent == NULL)
		return;

	int32 ClipBoneIndex { EquippedWeapon->GetItemMesh()->GetBoneIndex( EquippedWeapon->GetClipBoneName() ) };
	ClipTransform = EquippedWeapon->GetItemMesh()->GetBoneTransform(ClipBoneIndex);

	FAttachmentTransformRules AttachmentRules(EAttachmentRule::KeepRelative, true);
	HandSceneCompnent->AttachToComponent(GetMesh(), AttachmentRules, FName(TEXT("Hand_L")));
	HandSceneCompnent->SetWorldTransform(ClipTransform);

	EquippedWeapon->SetMovingClip(true);
}

	
void AShooterCharacter::ReleaseClip()
{
	if(EquippedWeapon == NULL)
		return;
	
	EquippedWeapon->SetMovingClip(false);
}

FInterpLocation AShooterCharacter::GetInterpLocation(int32 Index)
{
	if(Index <= InterpLocations.Num())
		return InterpLocations[Index];

	return FInterpLocation();
}

void AShooterCharacter::InitializeInterpLocations()
{
	FInterpLocation WeaponLocation{WeaponInterpComp, 0 };
	InterpLocations.Add(WeaponLocation);

	FInterpLocation InterpLoc1{ InterpComp1, 0 };
	InterpLocations.Add(InterpLoc1);
	FInterpLocation InterpLoc2{ InterpComp2, 0 };
	InterpLocations.Add(InterpLoc2);
	FInterpLocation InterpLoc3{ InterpComp3, 0 };
	InterpLocations.Add(InterpLoc3);
	FInterpLocation InterpLoc4{ InterpComp4, 0 };
	InterpLocations.Add(InterpLoc4);
	FInterpLocation InterpLoc5{ InterpComp5, 0 };
	InterpLocations.Add(InterpLoc5);
	FInterpLocation InterpLoc6{ InterpComp6, 0 };
	InterpLocations.Add(InterpLoc6);
}

void AShooterCharacter::ResetPickupSoundTimer()
{
	bShouldPlayPickupSound = true;
}
void AShooterCharacter::ResetEquipSoundTimer()
{
	bShouldPlayEquipSound = true;
}

void AShooterCharacter::StartPickupSoundTimer()
{
	bShouldPlayPickupSound = true;
	GetWorldTimerManager().SetTimer(PickupSoundTimer, this, &AShooterCharacter::ResetPickupSoundTimer, PickupSoundResetTime);
}
void AShooterCharacter::StartEquipSoundTimer()
{
	bShouldPlayEquipSound = true;
	GetWorldTimerManager().SetTimer(EquipSoundTimer, this, &AShooterCharacter::ResetEquipSoundTimer, EquipSoundResetTime);
}
