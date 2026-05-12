// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNBossHealthBarWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * UERNBossHealthBarWidget - 보스 체력바 위젯 (화면 하단 표시)
 * - 체력바 + 보스 이름 + 누적 데미지 표시
 */
UCLASS()
class PROJECTERN_API UERNBossHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 보스 정보 설정 (최초 표시 시)
	UFUNCTION(BlueprintCallable, Category = "BossHealthBar")
	void SetBossInfo(const FText& Name, float HealthPercent);

	// 체력 업데이트 + 데미지 누적
	UFUNCTION(BlueprintCallable, Category = "BossHealthBar")
	void UpdateHealth(float HealthPercent, float DamageDealt);

	// 누적 데미지 리셋
	UFUNCTION(BlueprintCallable, Category = "BossHealthBar")
	void ResetAccumulatedDamage();

protected:
	virtual void NativeConstruct() override;

	// UI 바인딩
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* BossNameText;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* AccumulatedDamageText;

	// 누적 데미지 리셋 시간 (초)
	UPROPERTY(EditDefaultsOnly, Category = "BossHealthBar")
	float DamageResetTime = 3.0f;

private:
	// 누적 데미지 (로컬 플레이어 전용)
	float AccumulatedDamage = 0.f;

	// 리셋 타이머 핸들
	FTimerHandle DamageResetTimerHandle;

	// 누적 데미지 텍스트 업데이트
	void UpdateAccumulatedDamageText();
};
