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
    ClipBoneName(TEXT("smg_clip"))
{
    PrimaryActorTick.bCanEverTick = true;
}

void AWeapon::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if(GetItemState() == EItemState::EIS_Falling && bFalling)
    {
        const FRotator MeshRotation{.0f, GetItemMesh()->GetComponentRotation().Yaw, .0f};
        GetItemMesh()->SetWorldRotation(MeshRotation, false, nullptr, ETeleportType::TeleportPhysics);
    }
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