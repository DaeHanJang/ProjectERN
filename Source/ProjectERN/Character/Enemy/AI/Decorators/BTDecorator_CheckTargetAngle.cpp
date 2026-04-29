// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/Decorators/BTDecorator_CheckTargetAngle.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UBTDecorator_CheckTargetAngle::UBTDecorator_CheckTargetAngle()
{
	NodeName = TEXT("Check Target Angle");
}

bool UBTDecorator_CheckTargetAngle::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC)
	{
		return false;
	}

	APawn* OwnerPawn = AIC->GetPawn();
	if (!OwnerPawn)
	{
		return false;
	}

	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	if (!Blackboard)
	{
		return false;
	}

	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor")));
	if (!Target)
	{
		return false;
	}

	// AI → 타겟 방향 벡터
	FVector DirectionToTarget = (Target->GetActorLocation() - OwnerPawn->GetActorLocation()).GetSafeNormal2D();

	// AI의 전방/오른쪽 벡터
	FVector ForwardVector = OwnerPawn->GetActorForwardVector().GetSafeNormal2D();
	FVector RightVector = OwnerPawn->GetActorRightVector().GetSafeNormal2D();

	// 타겟 각도 계산 (전방 기준 시계방향 0~360)
	float ForwardDot = FVector::DotProduct(ForwardVector, DirectionToTarget);
	float RightDot = FVector::DotProduct(RightVector, DirectionToTarget);

	float AngleRad = FMath::Atan2(RightDot, ForwardDot);
	float AngleDegrees = FMath::RadiansToDegrees(AngleRad);

	// -180~180 → 0~360 변환
	if (AngleDegrees < 0)
	{
		AngleDegrees += 360.0f;
	}

	// 범위 체크
	bool bInRange = false;
	if (MinAngle <= MaxAngle)
	{
		// 일반 범위
		bInRange = (AngleDegrees >= MinAngle && AngleDegrees <= MaxAngle);
	}
	else
	{
		// 0도를 넘는 범위
		bInRange = (AngleDegrees >= MinAngle || AngleDegrees <= MaxAngle);
	}

	return bInRange;
}
