// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_TeleportBehindTarget.generated.h"

/**
 * 타겟 액터의 뒤로 순간이동하는 BT Task (보스 패턴용)
 * - 서버에서만 이동, 위치는 자동 리플리케이트
 * - 시작→도착 번개 빔 잔상은 보스의 Multicast_PlayTeleportTrail로 동기화
 */
UCLASS()
class PROJECTERN_API UBTTask_TeleportBehindTarget : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_TeleportBehindTarget();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	// 보스→타겟 벡터 기준 오프셋. X=타겟 너머(등 뒤), Y=좌우, Z=상하
	UPROPERTY(EditAnywhere, Category = "Teleport")
	FVector TeleportOffset = FVector(200.f, 0.f, 0.f);

	// 바닥에 스냅할지 여부 (허공/지오메트리 박힘 방지)
	UPROPERTY(EditAnywhere, Category = "Teleport")
	bool bSnapToGround = true;

	// 지면 탐색 트레이스 길이 (위/아래)
	UPROPERTY(EditAnywhere, Category = "Teleport", meta = (EditCondition = "bSnapToGround"))
	float GroundTraceHalfHeight = 500.f;
};
