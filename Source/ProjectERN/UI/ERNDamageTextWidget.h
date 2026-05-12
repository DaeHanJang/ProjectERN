// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNDamageTextWidget.generated.h"

class UTextBlock;

/**
 * UERNDamageTextWidget - 데미지 숫자 표시 위젯
 */
UCLASS()
class PROJECTERN_API UERNDamageTextWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 데미지 값과 색상 설정
	UFUNCTION(BlueprintCallable, Category = "DamageText")
	void SetDamageText(float Damage, FLinearColor Color);

protected:
	// 데미지 텍스트 (BP에서 바인딩)
	UPROPERTY(meta = (BindWidget))
	UTextBlock* DamageText;
};
