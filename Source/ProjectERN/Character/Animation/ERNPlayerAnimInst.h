// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ERNPlayerAnimInst.generated.h"

UCLASS()
class PROJECTERN_API UERNPlayerAnimInst : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// 애니메이션 업데이트 함수 (매 프레임 호출)
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	// 애니메이션 파라미터들 (Blueprint에서 읽기 전용으로 사용)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsInAir;  // 공중에 있는지 여부 (점프/낙하)
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsAccelerating;  // 가속 중인지 여부
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
	bool bIsAttacking = false;	// 공격 중인지 여부
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "LifeState")
	bool bIsDowned = false;	// 기절 상태 여부
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	FVector Velocity;  // 캐릭터의 속도 벡터 (방향 계산용)
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float Speed;  // 캐릭터의 이동 속도

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float FallSpeed;	// 낙하 속도 (착지 모션 분기용)
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float Direction;  // 이동 방향 (Yaw 각도, -180 ~ 180)

private:
	// 위치 기반 속도 계산용 (시퀀서 Transform Track 지원)
	FVector PreviousLocation = FVector::ZeroVector;
	bool bPreviousLocationValid = false;

	// 컷신 상태 전환 감지용
	bool bWasInCutscene = false;
};
