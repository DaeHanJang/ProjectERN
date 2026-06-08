// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ERNDownedComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDownedGaugeChanged, float, CurrentHealth, float, MaxHealth, int32, PenaltyStacks);
DECLARE_MULTICAST_DELEGATE(FOnReviveGaugeDepleted);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTERN_API UERNDownedComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UERNDownedComponent();

	// 서버의 값을 클라이언트로 복제할 멤버 변수 목록을 등록하는 함수
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// 기절 상태 진입
	void EnterDownedState(int32 DeathPenaltyStacks);
	// 기절 상태에서의 Hit판정 적용 (살리기)
	void ApplyReviveHit(AController* Reviver);
	// 기절 상태 탈출
	void ExitDownedState();

	// Getter
	UFUNCTION(BlueprintPure, Category="ERN|Downed")
	float GetDownedHealth() const { return DownedHealth; }

	UFUNCTION(BlueprintPure, Category="ERN|Downed")
	float GetMaxDownedHealth() const { return MaxDownedHealth; }

	UFUNCTION(BlueprintPure, Category="ERN|Downed")
	float GetDownedHealthPercent() const;

	UFUNCTION(BlueprintPure, Category="ERN|Downed")
	int32 GetPenaltyStacks() const { return PenaltyStacks; }

	// 기절 상태 HP 변화 알림 이벤트
	UPROPERTY(BlueprintAssignable, Category="ERN|Downed")
	FOnDownedGaugeChanged OnDownedGaugeChanged;

	// 기절 상태 HP이 모두 소진되었음을 알리는 이벤트
	FOnReviveGaugeDepleted OnReviveGaugeDepleted;
	
protected:
	// 기절 상태 HP Base
	UPROPERTY(EditDefaultsOnly, Category="ERN|Downed")
	float BaseDownedHealth = 0.f;

	// 죽음 패널티 당 추가 체력
	UPROPERTY(EditDefaultsOnly, Category="ERN|Downed")
	float DownedHealthPerPenaltyStack = 30.f;

	// 매 타격 적용할 고정 대미지
	UPROPERTY(EditDefaultsOnly, Category="ERN|Downed")
	float ReviveHitAmount = 10.f;

	// 죽음 패널티 최대 스택
	UPROPERTY(EditDefaultsOnly, Category="ERN|Downed")
	int32 MaxPenaltyStacks = 5;

	// 현재 기절 상태 체력
	UPROPERTY(ReplicatedUsing=OnRep_DownedHealth)
	float DownedHealth = 0.f;

	// 전체 기절 상태 체력
	UPROPERTY(ReplicatedUsing=OnRep_MaxDownedHealth)
	float MaxDownedHealth = 0.f;

	// 죽음 패널티 스택
	UPROPERTY(ReplicatedUsing=OnRep_PenaltyStacks)
	int32 PenaltyStacks = 0;

	// 기절 상태 체력 Replicate
	UFUNCTION()
	void OnRep_DownedHealth();

	// 기절 상태 최대 체력 Replicate
	UFUNCTION()
	void OnRep_MaxDownedHealth();

	// 죽음 패널티 스택 Replicate
	UFUNCTION()
	void OnRep_PenaltyStacks();

private:
	// 기절 체력의 값이 바뀌었음을 UI에 알림
	void BroadcastGaugeChanged();
	
public:
	// PenaltyStacks Getter - 최대 패널티 기준 전체 바로 그리기 위한 Getter
	UFUNCTION(BlueprintPure, Category="ERN|Downed")
	int32 GetMaxPenaltyStacks() const { return MaxPenaltyStacks; }

	// Down상태에서 가질 수 있는 최대 최대 체력
	UFUNCTION(BlueprintPure, Category="ERN|Downed")
	float GetMaxPossibleDownedHealth() const;

	// 현재 기절 HP가 전체 최대 가능 기절 HP 기준으로 몇 % 차 있는지 반환
	UFUNCTION(BlueprintPure, Category="ERN|Downed")
	float GetDownedHealthGlobalPercent() const;

	// 현재 패널티 스택으로 적용된 이번 기절의 최대 HP 영역이 전체 바 기준으로 어디까지인지 반환
	UFUNCTION(BlueprintPure, Category="ERN|Downed")
	float GetActiveDownedMaxGlobalPercent() const;
};
