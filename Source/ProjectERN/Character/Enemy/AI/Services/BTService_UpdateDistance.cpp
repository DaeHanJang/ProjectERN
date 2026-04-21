// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/Services/BTService_UpdateDistance.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTService_UpdateDistance::UBTService_UpdateDistance()
{
	NodeName = TEXT("Update Distance");
	Interval = 0.1f;
	RandomDeviation = 0.0f;

	// 블랙보드 키 필터 설정
	TargetKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateDistance, TargetKey), AActor::StaticClass());
	DistanceKey.AddFloatFilter(this, GET_MEMBER_NAME_CHECKED(UBTService_UpdateDistance, DistanceKey));
}

void UBTService_UpdateDistance::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	AAIController* AIController = OwnerComp.GetAIOwner();
	if (!AIController)
	{
		return;
	}

	APawn* ControlledPawn = AIController->GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	AActor* TargetActor = Cast<AActor>(BlackboardComp->GetValueAsObject(TargetKey.SelectedKeyName));

	if (TargetActor)
	{
		const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), TargetActor->GetActorLocation());
		BlackboardComp->SetValueAsFloat(DistanceKey.SelectedKeyName, Distance);
	}
	else
	{
		// 타겟이 없으면 매우 큰 값으로 설정
		BlackboardComp->SetValueAsFloat(DistanceKey.SelectedKeyName, MAX_FLT);
	}
}
