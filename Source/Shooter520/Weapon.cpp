// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"
#include "AmmoType.h"

AWeapon::AWeapon() : 
    ThrowWeaponTime(0.7f), 
    bFalling(false), 
    Ammo(30), 
    MagazineCapacity(30),
    WeaponType(EWeaponType::EWT_SubmachineGun), 
    AmmoType(EAmmoType::EAT_9mm),
    //ReloadMontageSection(FName(TEXT("Reload"))),//_SMG"))), 
    ReloadMontageSection(FName(TEXT("Reload_SMG"))), 
    ClipBoneName(TEXT("smg_clip")),
    SlideDisplacement(0.0f),
    SlideDisplacementTime(0.1f),
    bMovingSlide(false),
    MaxSlideDisplacement(4.0f),
    MaxRecoilRatation(20.0f),
    bAutomatic(true)
{
    PrimaryActorTick.bCanEverTick = true;
}

void AWeapon::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    
    const FString WeaponTablePath = TEXT("/Script/Engine.DataTable'/Game/_Game/DataTable/WeaponDataTable.WeaponDataTable'");
    UDataTable* WeaponTableObject = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), NULL, *WeaponTablePath));

    if(WeaponTableObject)
    {
        FWeaponDataTable* WeaponDataRow = NULL;
        switch(WeaponType)
        {
        case EWeaponType::EWT_SubmachineGun:
            WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("SubmachineGun"), TEXT(""));
            break;
        case EWeaponType::EWT_AssaultRifle:
            WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("AssaultRifle"), TEXT(""));
            break;
        case EWeaponType::EWT_Pistol:
            WeaponDataRow = WeaponTableObject->FindRow<FWeaponDataTable>(FName("Pistol"), TEXT(""));
            break;
        }

        if(WeaponDataRow)
        {
            AmmoType = WeaponDataRow->AmmoType;
            Ammo = WeaponDataRow->WeaponAmmo;
            MagazineCapacity = WeaponDataRow->MagazineCapacity;
            SetPickupSound( WeaponDataRow->PickupSound );
            SetEquipSound( WeaponDataRow->EquipSound );
            GetItemMesh()->SetSkeletalMesh(WeaponDataRow->ItemMesh);
            SetItemName(WeaponDataRow->ItemName);
            SetIconItem(WeaponDataRow->InventoryIcon);
            SetAmmoIcon(WeaponDataRow->AmmoIcon);

            SetMaterialInstance(WeaponDataRow->MaterialInstance);
            PreviousMaterialIndex = GetMaterialIndex();
            GetItemMesh()->SetMaterial(PreviousMaterialIndex, NULL);
            SetMaterialIndex(WeaponDataRow->MaterialIndex);
            SetClipBoneName(WeaponDataRow->ClipBoneName);
            SetReloadMontageSection(WeaponDataRow->ReloadMontageSection);
            GetItemMesh()->SetAnimInstanceClass(WeaponDataRow->AnimBP);

            CrosshairsMiddle = WeaponDataRow->CrosshairsMiddle;
            CrosshairsLeft = WeaponDataRow->CrosshairsLeft;
            CrosshairsRight = WeaponDataRow->CrosshairsRight;
            CrosshairsBottom = WeaponDataRow->CrosshairsBottom;
            CrosshairsTop = WeaponDataRow->CrosshairsTop;

            AutoFireRate = WeaponDataRow->AutoFireRate;
            MuzzleFlash = WeaponDataRow->MuzzleFlash;
            FireSound = WeaponDataRow->FireSound;

            BoneToHide = WeaponDataRow->BoneToHide;
            bAutomatic = WeaponDataRow->bAutomatic;

            Damage = WeaponDataRow->Damage;
            HeadShotDamage = WeaponDataRow->HeadShotDamage;
        }

        if(GetMaterialInstance())
        {
            SetDynamicMaterialInstance( UMaterialInstanceDynamic::Create(GetMaterialInstance(), this) );
            GetDynamicMaterialInstance()->SetVectorParameterValue(TEXT("FresnelColor"), GetGlowColor());
            GetItemMesh()->SetMaterial(GetMaterialIndex(), GetDynamicMaterialInstance());
            
            EnableGlowMaterial();
        }
    }
}

void AWeapon::BeginPlay()
{
    Super::BeginPlay();
    if(BoneToHide != FName(""))
        GetItemMesh()->HideBoneByName(BoneToHide, EPhysBodyOp::PBO_None);
}

void AWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if(GetItemState() == EItemState::EIS_Falling && bFalling)
    {
        const FRotator MeshRotation{.0f, GetItemMesh()->GetComponentRotation().Yaw, .0f};
        GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
    }

    UpdateSlideDisplament();
}
void AWeapon::ThrowWeapon()
{
    FRotator MeshRotation{.0f, GetItemMesh()->GetComponentRotation().Yaw, .0f };
    GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);

    const FVector MeshForward{GetItemMesh()->GetForwardVector()};
    const FVector MeshRight{ GetItemMesh()->GetRightVector() };
    // weaphon throw direction
    FVector ImpulseDirection = MeshRight.RotateAngleAxis(-20.0f, MeshForward);
    
    float RandomRotation{ 30.0f };
    ImpulseDirection = ImpulseDirection.RotateAngleAxis(RandomRotation, FVector(.0f, .0f, 1.0f));

    ImpulseDirection *= 30000.0f;
    GetItemMesh()->AddImpulse(ImpulseDirection);

    bFalling = true;
    GetWorldTimerManager().SetTimer(ThrowWeaponTimer, this, &AWeapon::StopFalling, ThrowWeaponTime);

    EnableGlowMaterial();
}

void AWeapon::StopFalling()
{
    bFalling = false;
    SetItemState(EItemState::EIS_PickedUp);
    StartPulseTimer();
}

void AWeapon::DecrementAmmo()
{
    if(Ammo - 1 <= 0)
        Ammo = 0;
    else 
        --Ammo;
}

void AWeapon::ReloadAmmo(int32 amount)
{
    checkf(Ammo + amount <= MagazineCapacity, TEXT("Attempted to reload with than magazine capacity"));

    Ammo += amount;
}

bool AWeapon::ClipIsFull()
{
    return Ammo >= MagazineCapacity;
}

void AWeapon::FinishMovingSlide()
{
    bMovingSlide = false;
}

void AWeapon::StartSlideTimer()
{
    bMovingSlide = true;

    GetWorldTimerManager().SetTimer(SlideTimer, this, &AWeapon::FinishMovingSlide, SlideDisplacementTime);
}

void AWeapon::UpdateSlideDisplament()
{
    if(SlideDisplacementCurve != NULL && bMovingSlide)
    {
        const float ElapsedTime = GetWorldTimerManager().GetTimerElapsed(SlideTimer);
        const float CurveValue = SlideDisplacementCurve->GetFloatValue(ElapsedTime);
        SlideDisplacement = CurveValue * MaxSlideDisplacement;
        RecoilRotation = CurveValue * MaxRecoilRatation;
    }
    else 
    {
        SlideDisplacement = .0f;
        RecoilRotation = .0f;
    }

}