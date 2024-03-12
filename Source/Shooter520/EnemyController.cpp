// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemyController.h"
#include "Enermy.h"
#include "behaviorTree/BlackboardComponent.h"
#include "behaviorTree/BehaviorTreeComponent.h"
#include "Enermy.h"
#include "BehaviorTree/BehaviorTree.h"

AEnemyController::AEnemyController()
{
    BlackboardComponent = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComponent"));
    check(BlackboardComponent);

    BehaviorTreeComponent = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorTreeComponent"));
    check(BehaviorTreeComponent);
}

void AEnemyController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    if(InPawn == NULL)  return;
    
    AEnermy* Enemy = Cast<AEnermy>(InPawn);
    if(Enemy)
    {
        if(Enemy->GetBehaviorTree())
        {
            BlackboardComponent->InitializeBlackboard(*(Enemy->GetBehaviorTree()->BlackboardAsset));
        }
    }

}