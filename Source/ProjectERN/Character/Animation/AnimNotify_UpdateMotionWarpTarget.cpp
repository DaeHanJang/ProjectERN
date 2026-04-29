// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Animation/AnimNotify_UpdateMotionWarpTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "MotionWarpingComponent.h"

void UAnimNotify_UpdateMotionWarpTarget::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();

	// Motion Warping Component 가져오기
	UMotionWarpingComponent* MotionWarping = Owner->FindComponentByClass<UMotionWarpingComponent>();
	if (!MotionWarping)
	{
		return;
	}
	
	AAIController* AIC = Cast<AAIController>(Owner->GetInstigatorController());
	if (!AIC || !AIC->GetBlackboardComponent())
	{
		return;
	}
	
	// AI Controller에서 타겟 가져오기
	AActor* Target = Cast<AActor>(AIC->GetBlackboardComponent()->GetValueAsObject(TEXT("TargetActor")));
	if (!Target)
	{
		return;
	}

	// 워프 위치 계산
	FVector WarpLocation = Target->GetActorLocation();

	// 오프셋이 있으면 적용
	if (RightOffset != 0 || ForwardOffset != 0)
	{
		// 타겟 → 보스 방향 벡터
		FVector DirectionToTarget = (Owner->GetActorLocation() - Target->GetActorLocation()).GetSafeNormal2D();

		// 오른쪽 벡터 (Z축 기준 90도 회전)
		FVector RightVector = FVector::CrossProduct(FVector::UpVector, DirectionToTarget);

		// 오프셋 적용
		WarpLocation += RightVector * RightOffset;
		WarpLocation += DirectionToTarget * ForwardOffset;
	}

	// Motion Warping 타겟 위치 업데이트
	MotionWarping->AddOrUpdateWarpTargetFromLocation(WarpTargetName, WarpLocation);
}
