// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/AI/Decorators/BTDecorator_CooldownInitial.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "Engine/World.h"

UBTDecorator_CooldownInitial::UBTDecorator_CooldownInitial()
{
	NodeName = TEXT("Cooldown (Initial Lock)");
}

void UBTDecorator_CooldownInitial::InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
	EBTMemoryInit::Type InitType) const
{
	Super::InitializeMemory(OwnerComp, NodeMemory, InitType);

	// 트리 시작 시 "방금 사용"한 것으로 기록 → 첫 CoolDownTime 동안 잠긴 상태로 시작
	if (InitType == EBTMemoryInit::Initialize)
	{
		FBTCooldownDecoratorMemory* Memory = CastInstanceNodeMemory<FBTCooldownDecoratorMemory>(NodeMemory);
		if (Memory && OwnerComp.GetWorld())
		{
			Memory->LastUseTimestamp = OwnerComp.GetWorld()->GetTimeSeconds();
		}
	}
}
