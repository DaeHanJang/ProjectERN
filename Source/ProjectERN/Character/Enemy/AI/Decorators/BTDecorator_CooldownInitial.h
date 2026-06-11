// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Decorators/BTDecorator_Cooldown.h"
#include "BTDecorator_CooldownInitial.generated.h"

/**
 * 초기 잠금 쿨다운 데코레이터
 * 기본 Cooldown은 노드가 한 번 실행된 후부터 잠그지만,
 * 이 데코레이터는 트리 시작 시점부터 CoolDownTime 동안 잠긴 상태로 시작
 * (쿨 → 실행 → 쿨 → 실행 ... 순서. BT 시작/페이즈 전환 직후 패턴이 바로 도는 것 방지)
 */
UCLASS(DisplayName = "Cooldown (Initial Lock)")
class PROJECTERN_API UBTDecorator_CooldownInitial : public UBTDecorator_Cooldown
{
	GENERATED_BODY()

public:
	UBTDecorator_CooldownInitial();

protected:
	virtual void InitializeMemory(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory,
		EBTMemoryInit::Type InitType) const override;
};
