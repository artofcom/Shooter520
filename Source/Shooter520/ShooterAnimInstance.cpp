// Fill out your copyright notice in the Description page of Project Settings.


#include "ShooterAnimInstance.h"
#include "ShooterCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include <future>


UShooterAnimInstance::UShooterAnimInstance() : 
 Speed(.0f),
 bIsInAir(false),
 bIsAccelerating(false),
 MovementOffsetYaw(.0f),
 LastMovementOffsetYaw(.0f),
 bAiming(false),
 TIPCharacterYaw(.0f),
 TIPCharacterYawLastFrame(.0f),
 RootYawOffset(.0f),
 Pitch(.0f),
 bReloading(false),
 OffsetState(EOffsetState::EOS_Hip),
 CharacterRotation(FRotator(.0f)),
 CharacterRotationLastFrame(FRotator(.0f)),
 YawDelta(.0f),
 RecoilWeight(1.0f),
 bTurningInPlace(false)
{

}

// Init.
void UShooterAnimInstance::NativeInitializeAnimation()
{
    ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner());
}


// Update. 
void UShooterAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
    if(ShooterCharacter == nullptr)
        ShooterCharacter = Cast<AShooterCharacter>(TryGetPawnOwner()); 
    
    // 
    if(ShooterCharacter != nullptr)
    {
        bCrouching = ShooterCharacter->GetCrouching();
        bReloading = ShooterCharacter->GetCombatState()==ECombatState::ECS_Reloading;
        bEquipping = ShooterCharacter->GetCombatState()==ECombatState::ECS_Equipping;

        FVector Velocity{ ShooterCharacter->GetVelocity()} ;
        Velocity.Z = .0f;

        // Update Animation Parameters.
        //
        Speed = Velocity.Size();

        bIsInAir = ShooterCharacter->GetCharacterMovement()->IsFalling();

        bIsAccelerating = (ShooterCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > .0f);

        bAiming = ShooterCharacter->GetAiming();

        // MovementOffsetYaw. - Character's Heading Direction Setting.
        FRotator AimRotation = ShooterCharacter->GetBaseAimRotation();  
        FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX( ShooterCharacter->GetVelocity() );    
        MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
        
        // For Stopping. 
        if(ShooterCharacter->GetVelocity().Size() > .0f)
            LastMovementOffsetYaw = MovementOffsetYaw;

        // Set Offset-State.
        if(bReloading)
            OffsetState = EOffsetState::EOS_Reloading;
        else if(bIsInAir) 
            OffsetState = EOffsetState::EOS_InAir;
        else if(ShooterCharacter->GetAiming())
            OffsetState = EOffsetState::EOS_Aiming;
        else 
            OffsetState = EOffsetState::EOS_Hip;

        TurnInPlace();
        Lean(DeltaTime);

        
        //
        // Loggers.
        // FString RotationMessage = FString::Printf(TEXT("Base Aim Rotation : %f"), AimRotation.Yaw);
        // FString MovementRotationMessage = FString::Printf(TEXT("Movement Rotation : %f"), MovementRotation.Yaw);
        // FString OffsetMessage = FString::Printf(TEXT("Movement Offset Yaw : %f"), MovementOffsetYaw);
        // if(GEngine)
        //    GEngine->AddOnScreenDebugMessage(1, .0f, FColor::White, OffsetMessage);
    }
}

void UShooterAnimInstance::TurnInPlace()
{
    if(ShooterCharacter == NULL)
        return;
    
    Pitch = ShooterCharacter->GetBaseAimRotation().Pitch;

    if(Speed > 0 || bIsInAir)
    {
        // Do Nothing. 
        RootYawOffset = .0f;
        TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;
        TIPCharacterYawLastFrame = TIPCharacterYaw;
        RotationCurveLastFrame = .0f;
        RotationCurve = .0f;
    }
    else 
    {
        TIPCharacterYawLastFrame = TIPCharacterYaw;
        TIPCharacterYaw = ShooterCharacter->GetActorRotation().Yaw;

        const float yawDelta = TIPCharacterYaw - TIPCharacterYawLastFrame;
        // [-180 ~ 180]
        RootYawOffset = UKismetMathLibrary::NormalizeAxis(RootYawOffset - yawDelta); 

        const float Turning = GetCurveValue(TEXT("Turning"));
        if(Turning > .0f)
        {
            bTurningInPlace = true;
            RotationCurveLastFrame = RotationCurve;
            RotationCurve = GetCurveValue(TEXT("Rotation"));
            const float DeltaRotation = RotationCurve - RotationCurveLastFrame;

            // RootYawOffset > 0 => Turning Left. RootYawOffset < 0 => Turning Right. 
            RootYawOffset = (RootYawOffset > .0f) ? RootYawOffset - DeltaRotation : RootYawOffset + DeltaRotation;

            const float ABSRootYawOffset = FMath::Abs(RootYawOffset);
            if(ABSRootYawOffset > 90.0f)
            {
                const float YawExcess = ABSRootYawOffset - 90.0f;
                RootYawOffset = (RootYawOffset > .0f) ? RootYawOffset - YawExcess : RootYawOffset + YawExcess;
            }
        }
        else bTurningInPlace = false;

        if(GEngine)
        {
            //GEngine->AddOnScreenDebugMessage(1, -1, FColor::Blue, FString::Printf(TEXT("TIPCharacterYaw: %f"), TIPCharacterYaw));
            //GEngine->AddOnScreenDebugMessage(2, -1, FColor::Red,  FString::Printf(TEXT("RootYawOffset: %f"), RootYawOffset));
        }
    }

    // Recoil weight check.
    if(bTurningInPlace)
    {
        if(bReloading || bEquipping)
            RecoilWeight = 1.0f;
        else 
            RecoilWeight = .0f;
    }
    else 
    {
        if(bCrouching)
        {
            if(bReloading || bEquipping)
                RecoilWeight = 1.0f;
            else 
                RecoilWeight = 0.1f;
        }
        else 
        {
            if(bAiming || bReloading || bEquipping)
                RecoilWeight = 1.0f;
            else 
                RecoilWeight = 0.5f;
        }
    }
}

void UShooterAnimInstance::Lean(float DeltaTime)
{
    if(ShooterCharacter == NULL)
        return;

    CharacterRotationLastFrame = CharacterRotation;
    CharacterRotation = ShooterCharacter->GetActorRotation();

    const FRotator Delta = UKismetMathLibrary::NormalizedDeltaRotator(CharacterRotation, CharacterRotationLastFrame);

    const float Target = Delta.Yaw / DeltaTime;
    const float Interp = FMath::FInterpTo(YawDelta, Target, DeltaTime, 6.0f);

    YawDelta = FMath::Clamp(Interp, -90.0f, 90.0f);

    //if(GEngine)
        // GEngine->AddOnScreenDebugMessage(2, -1, FColor::Cyan, FString::Printf(TEXT("YawDelta: %f"), YawDelta));
}