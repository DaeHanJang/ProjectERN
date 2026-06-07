// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNDownedStatusWidget.generated.h"

class AProjectERNCharacter;
class UERNDownedComponent;

/**
 * 캐릭터의 기절 상태에 표시될 UI의 뼈대가 될 클래스
 * 기절 체력은 Delegate로 갱신
 * 리스폰 카운트는 Tick으로 갱신
 */
UCLASS()
class PROJECTERN_API UERNDownedStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 관찰 대상 캐릭터를 받아옴
	UFUNCTION(BlueprintCallable, Category="ERN|Downed")
	void SetObservedCharacter(AProjectERNCharacter* InCharacter);
	
protected:
	// 위젯이 생성될 때 호출
	virtual void NativeConstruct() override;

	// 위젯이 제거될 때 호출
	virtual void NativeDestruct() override;

	// 매 프레임 호출
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// BP 위젯에서 실제 기절 UI를 갱신하기 위한 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category="ERN|Downed")
	void OnDownedVisualUpdated(
		// 현재 기절 HP
		float DownedHealthGlobalPercent,
		// 현재 PenaltyStacks 기준으로 활성화된 최대 기절 HP 영역 비율. ex) 전체 5칸 중 현재 3칸까지만 사용 가능하면 0.6
		float ActiveDownedMaxGlobalPercent,
		// 리스폰 카운트다운 남은 시간 비율
		float RespawnCountdownPercent,
		// 리스폰 카운트다운 적용 여부 확인
		bool bShowRespawnCountdown,
		// 현재 적용 중인 기절 패널티 스택 수
		int32 PenaltyStacks,
		// 기절 패널티의 최대 스택 수
		int32 MaxPenaltyStacks);

	// 기절 UI를 숨기기 위한 BP 이벤트
	UFUNCTION(BlueprintImplementableEvent, Category="ERN|Downed")
	void OnDownedVisualHidden();

private:
	// UERNDownedComponent의 OnDownedGaugeChanged 델리게이트를 받을 함수.
	// 기절 HP가 변경될 때 즉시 UI를 갱신
	UFUNCTION()
	void HandleDownedGaugeChanged(float CurrentHealth, float MaxHealth, int32 PenaltyStacks);

	// CachedCharacter에서 DownedComponent를 찾아 델리게이트에 바인딩
	void BindToDownedComponent();

	// 현재 바인딩된 DownedComponent의 델리게이트를 해제
	void UnbindFromDownedComponent();

	// 현재 캐릭터와 DownedComponent 상태를 읽어서 OnDownedVisualUpdated 또는 OnDownedVisualHidden을 호출한다.
	void UpdateDownedVisual();

	// 현재 UI가 바라보고 있는 플레이어 캐릭터
	UPROPERTY(Transient)
	TWeakObjectPtr<AProjectERNCharacter> CachedCharacter;

	// 현재 델리게이트가 바인딩된 DownedComponent.
	// 중복 바인딩 방지와 안전한 Unbind를 위해 저장한다.
	UPROPERTY(Transient)
	TWeakObjectPtr<UERNDownedComponent> BoundDownedComponent;

	// 직전 프레임에 UI가 보이는 상태였는지 기록한다.
	// Hidden 이벤트를 매 프레임 반복 호출하지 않기 위한 플래그
	bool bWasVisible = false;
};
