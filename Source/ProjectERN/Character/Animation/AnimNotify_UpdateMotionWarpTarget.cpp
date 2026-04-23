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

	// Motion Warping 타겟 위치 업데이트
	MotionWarping->AddOrUpdateWarpTargetFromLocation(WarpTargetName, Target->GetActorLocation());
}
