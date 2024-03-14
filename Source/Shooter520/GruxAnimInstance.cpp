// Fill out your copyright notice in the Description page of Project Settings.


#include "GruxAnimInstance.h"
#include "Enermy.h"


void UGruxAnimInstance::UpdateAnimationProperties(float DeltatTime)
{
    if(Enemy == NULL)
        Enemy = Cast<AEnermy>(TryGetPawnOwner());

    if(Enemy != NULL)
    {
        FVector Velocity = Enemy->GetVelocity();
        Velocity.Z = .0f;
        Speed = Velocity.Size();
    }

}