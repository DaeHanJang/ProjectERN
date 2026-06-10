// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MyStatusWidget.generated.h"

class UProgressBar;
class UTextBlock;
class AERNPlayerState;
class UERNBuffListWidget;

UCLASS()
class PROJECTERN_API UMyStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 명시적으로 대상 PlayerState를 지정 (파티원 위젯 등에서 재활용 시 사용)
	UFUNCTION(BlueprintCallable, Category = "Status")
	void SetTargetPlayerState(AERNPlayerState* NewPlayerState);

protected:
	// 위젯 바인딩
	UPROPERTY(meta = (BindWidget))
	UProgressBar* HealthBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* ManaBar;

	UPROPERTY(meta = (BindWidget))
	UProgressBar* StaminaBar;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* HealthText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* ManaText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* StaminaText;

	// 캐시된 PlayerState
	UPROPERTY()
	AERNPlayerState* CachedPlayerState;

	void UpdateHealth();
	void UpdateMana();
	void UpdateStamina();

	// 버프 아이콘들을 담을 리스트 위젯
	UPROPERTY(meta = (BindWidgetOptional))
	UERNBuffListWidget* BuffList;

	// ASC 초기화 여부
	bool bIsBuffListInitialized = false;
};
