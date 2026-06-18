// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/Tasks/BTTask_GetNextPatrolPoint.h"
#include "AIController.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTTask_GetNextPatrolPoint::UBTTask_GetNextPatrolPoint()
{
	NodeName = TEXT("Get Next Patrol Point");
}

EBTNodeResult::Type UBTTask_GetNextPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return EBTNodeResult::Failed;
	}

	AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(AIController->GetPawn());
	if (!Enemy)
	{
		return EBTNodeResult::Failed;
	}

	// 순찰 포인트 배열이 없으면 실패
	if (Enemy->PatrolPoints.Num() == 0)
	{
		//UE_LOG(LogTemp, Warning, TEXT("[%s] No patrol points assigned!"), *Enemy->GetName());
		return EBTNodeResult::Failed;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp)
	{
		return EBTNodeResult::Failed;
	}

	// 현재 인덱스 가져오기
	int32 CurrentIndex = BlackboardComp->GetValueAsInt(PatrolIndexKey);

	// 다음 인덱스로 증가 (순환)
	int32 NextIndex = (CurrentIndex + 1) % Enemy->PatrolPoints.Num();

	// 다음 순찰 포인트 위치 설정
	if (Enemy->PatrolPoints.IsValidIndex(NextIndex))
	{
		AActor* PatrolPoint = Enemy->PatrolPoints[NextIndex];
		if (PatrolPoint)
		{
			FVector PatrolLocation = PatrolPoint->GetActorLocation();
			BlackboardComp->SetValueAsVector(PatrolPointKey, PatrolLocation);
			BlackboardComp->SetValueAsInt(PatrolIndexKey, NextIndex);

			//UE_LOG(LogTemp, Log, TEXT("[%s] Moving to patrol point %d at %s"),
			//	*Enemy->GetName(), NextIndex, *PatrolLocation.ToString());

			return EBTNodeResult::Succeeded;
		}
	}

	return EBTNodeResult::Failed;
}
