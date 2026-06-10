// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNBuffIconWidget.generated.h"

class UImage;
class UProgressBar;
class UItemDataAssetBase;

/**
 * 단일 버프 아이콘 위젯
 */
UCLASS()
class PROJECTERN_API UERNBuffIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 위젯 초기화 (아이콘 세팅 및 쿨다운 시작)
	void InitBuff(const UItemDataAssetBase* BuffData, float InDuration);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 버프 기본 아이콘
	UPROPERTY(meta = (BindWidget))
	UImage* IconImage;

	// 버프 남은 시간을 위에서 아래로 보여줄 프로그레스 바 (FillType: Top to Bottom 권장)
	UPROPERTY(meta = (BindWidget))
	UProgressBar* CooldownBar;

private:
	float TotalDuration = 0.f;
	float TimeRemaining = 0.f;
	bool bIsCooldownActive = false;
};
