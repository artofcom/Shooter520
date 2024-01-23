// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SphereComponent.h"
#include "ShooterCharacter.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Curves/CurveVector.h"

// Sets default values
AItem::AItem() : 
	ItemName(FString("Default")), 
	ItemCount(0),
	ItemRarity(EItemRarity::EIR_Common), 
	ItemState(EItemState::EIS_Pickup),
	
	ItemInterpStartLocation(FVector(.0f)),
	CameraTargetLocation(FVector(.0f)),
	bInterping(false),
	ZCurveTime(0.7f),

	ItemInterpX(.0f),
	ItemInterpY(.0f), 
	InterpInitialYawOffset(.0f), 

	ItemType(EItemType::EIT_MAX),
	InterpLocIndex(0),
	MaterialIndex(0), 
	bCanChangeCustomDepth(true), 

	PulseCurveTime(5.0f),
	GlowAmount(150.0f),
	FresnelExponent(3.0f),
	FresnelReflectFraction(4.0f)
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	ItemMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ItemMesh"));
	SetRootComponent(ItemMesh);

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetupAttachment(ItemMesh);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(GetRootComponent());

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(GetRootComponent());
}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();
	
	if(PickupWidget)
		PickupWidget->SetVisibility(false);

	SetActiveStars();

	AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AItem::OnSphereOverlap);
	AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AItem::OnSphereEndOverlap);
	SetItemProperties(ItemState);

	InitializeCustomDepth();
	
	StartPulseTimer();
}

void AItem::StartPulseTimer()
{
	if(ItemState == EItemState::EIS_Pickup)
	{
		GetWorldTimerManager().SetTimer(PulseTimer, this, &AItem::ResetPulseTimer, PulseCurveTime);
	}
}
void AItem::ResetPulseTimer()
{
	StartPulseTimer();
}

void AItem::UpdatePulse()
{
	float ElapsedTime;
	FVector CurveValue;

	switch (ItemState)
	{
	case EItemState::EIS_Pickup:
		if (PulseCurve)
		{
			ElapsedTime = GetWorldTimerManager().GetTimerElapsed(PulseTimer);
			CurveValue = PulseCurve->GetVectorValue(ElapsedTime);
		}
		break;
	case EItemState::EIS_EquipInterping:
		if (InterpPulseCurve)
		{
			ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
			CurveValue = InterpPulseCurve->GetVectorValue(ElapsedTime);
		}
		break;
	default:
		return;
	}

	if (DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowAmount"), CurveValue.X * GlowAmount);
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelExponent"), CurveValue.Y * FresnelExponent);
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("FresnelReflectFraction"), CurveValue.Z * FresnelReflectFraction);
	}
}
void AItem::OnConstruction(const FTransform& Transform)
{
	if(MaterialInstance)
	{
		DynamicMaterialInstance = UMaterialInstanceDynamic::Create(MaterialInstance, this);
		ItemMesh->SetMaterial(MaterialIndex, DynamicMaterialInstance);
	}
	EnableGlowMaterial();
}
void AItem::OnSphereOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult)
{
	if(OtherActor)
	{
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if(ShooterCharacter)
		{
			ShooterCharacter->IncrementOverlappedItemCount(1);
		}
	}
}

void AItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex)
{
	if(OtherActor)
	{
		AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(OtherActor);
		if(ShooterCharacter)
		{
			ShooterCharacter->IncrementOverlappedItemCount(-1);
		}
	}
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	ItemInterp(DeltaTime);

	UpdatePulse();
}

void AItem::SetItemProperties(EItemState state)
{
	switch(state)
	{
	case EItemState::EIS_Pickup:
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;

	case EItemState::EIS_EquipInterping:
		PickupWidget->SetVisibility(false);

		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;

	case EItemState::EIS_PickedUp:
		break;

	case EItemState::EIS_Equipped:
		PickupWidget->SetVisibility(false);
		
		ItemMesh->SetEnableGravity(false);
		ItemMesh->SetSimulatePhysics(false);
		ItemMesh->SetVisibility(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		break;
	case EItemState::EIS_Falling:
		ItemMesh->SetSimulatePhysics(true);
		ItemMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		ItemMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
		ItemMesh->SetEnableGravity(true);

		AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Overlap);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

		CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
		CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		break;
	
	}
}

void AItem::SetItemState(EItemState state)
{
	ItemState = state;
	SetItemProperties(state);
}


void AItem::SetActiveStars()
{
	// idx 0 - no use.
	for(int32 k = 0; k <= 5; ++k)
	{
		ActiveStars.Add(false);
	}

	switch(ItemRarity)
	{
	case EItemRarity::EIR_Damaged:
		ActiveStars[1] = true;
		break;
	case EItemRarity::EIR_Common:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		break;
	case EItemRarity::EIR_Uncommon:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		break;
	case EItemRarity::EIR_Rare:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		ActiveStars[4] = true;
		break;
	case EItemRarity::EIR_Legendary:
		ActiveStars[1] = true;
		ActiveStars[2] = true;
		ActiveStars[3] = true;
		ActiveStars[4] = true;
		ActiveStars[5] = true;
		break;
	}
}

void AItem::FinishInterping()
{
	if(Character)
	{
		Character->IncrementInterpLocItemCount(InterpLocIndex, -1);
		Character->GetPickupItem(this);
		SetItemState(EItemState::EIS_Pickup);
	}
	bInterping = false;
	SetActorScale3D(FVector(1.0f));
	
	bCanChangeCustomDepth = true;
	DisableGlowMaterial();
	DisableCustomDepth();
}

void AItem::StartItemCurve(AShooterCharacter* Char)
{
	Character = Char;

	InterpLocIndex = Character->GetInterpLocationIndex();
	Character->IncrementInterpLocItemCount(InterpLocIndex, 1);

	PlayPickupSound();
	
	ItemInterpStartLocation = GetActorLocation();
	bInterping = true;
	SetItemState(EItemState::EIS_EquipInterping);
	GetWorldTimerManager().ClearTimer(PulseTimer);

	GetWorldTimerManager().SetTimer(ItemInterpTimer, this, &AItem::FinishInterping, ZCurveTime);

	const float CameraRotationYaw = Character->GetFollowCamera()->GetComponentRotation().Yaw;
	const float ItemRotationYaw = GetActorRotation().Yaw;
	InterpInitialYawOffset = ItemRotationYaw - CameraRotationYaw;

	bCanChangeCustomDepth = false;
}

void AItem::PlayPickupSound()
{
	if(Character)
	{
		if(Character->ShouldPlayPickupSound())
		{
			Character->StartPickupSoundTimer();
			if(PickupSound)
				UGameplayStatics::PlaySound2D(this, PickupSound);
		}
	}
}
void AItem::PlayEquipSound()
{
	if(Character)
	{
		if(Character->ShouldPlayEquipSound())
		{
			Character->StartEquipSoundTimer();
			if(EquipSound)
				UGameplayStatics::PlaySound2D(this, EquipSound);
		}
	}
}

FVector AItem::GetInterpLocation()
{
	if(Character == NULL)
		return FVector(.0f);

	switch(ItemType)
	{
		case EItemType::EIT_Ammo:
			return Character->GetInterpLocation(InterpLocIndex).SceneComponent->GetComponentLocation();
		case EItemType::EIT_Weapon:
			return Character->GetInterpLocation(0).SceneComponent->GetComponentLocation();
		default:
			break;
	}
	return FVector(.0f);
}
void AItem::ItemInterp(float DeltaTime)
{
	if(!bInterping) return;

	if(Character && ItemZCurve)
	{
		// elapsed time since we started interinterp timer.
		const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(ItemInterpTimer);
		const float CurveValue = ItemZCurve->GetFloatValue(ElapsedTime);

		FVector ItemLocation = ItemInterpStartLocation;
		//const FVector CameraInterpLocation { Character->GetCameraInterpLocation() };
		const FVector CameraInterpLocation { GetInterpLocation() };

		const FVector ItemToCamera{FVector(.0f, .0f, (CameraInterpLocation-ItemLocation).Z)};
		const float DeltaZ = ItemToCamera.Size();

		const FVector CurrentLocation{GetActorLocation()};
		const float InterpXValue = FMath::FInterpTo(CurrentLocation.X, CameraInterpLocation.X, DeltaTime, 30.0f);
		const float InterpYValue = FMath::FInterpTo(CurrentLocation.Y, CameraInterpLocation.Y, DeltaTime, 30.0f);

		ItemLocation.X = InterpXValue;
		ItemLocation.Y = InterpYValue;

		ItemLocation.Z += CurveValue * DeltaZ;
		SetActorLocation(ItemLocation, true, NULL, ETeleportType::TeleportPhysics);

		const FRotator CameraRotation { Character->GetFollowCamera()->GetComponentRotation() };
		FRotator ItemRotation { .0f, CameraRotation.Yaw + InterpInitialYawOffset, .0f };
		SetActorRotation(ItemRotation, ETeleportType::TeleportPhysics);

		if(ItemScaleCurve)
		{
			const float ScaleCurveValue = ItemScaleCurve->GetFloatValue(ElapsedTime);
			SetActorScale3D(FVector(ScaleCurveValue, ScaleCurveValue, ScaleCurveValue));
		}
	}
}

void AItem::InitializeCustomDepth()
{
	DisableCustomDepth();
}

void AItem::EnableCustomDepth()
{
	if(bCanChangeCustomDepth)
		ItemMesh->SetRenderCustomDepth(true);
}
void AItem::DisableCustomDepth()
{
	if(bCanChangeCustomDepth)
		ItemMesh->SetRenderCustomDepth(false);
}
void AItem::EnableGlowMaterial()
{
	if(DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 0);
	}
}

void AItem::DisableGlowMaterial()
{
	if(DynamicMaterialInstance)
	{
		DynamicMaterialInstance->SetScalarParameterValue(TEXT("GlowBlendAlpha"), 1);
	}
}