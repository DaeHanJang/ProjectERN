// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNEntranceWidget.generated.h"

class UTextBlock;

/**
 * 지역 진입 안내 위젯.
 * 텍스트만 받아 TextBlock에 표시 (애니메이션/숨김은 BP에서 처리).
 */
UCLASS()
class PROJECTERN_API UERNEntranceWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 진입 문구를 TextBlock에 설정
	UFUNCTION(BlueprintCallable, Category = "Entrance")
	void SetEntranceText(const FText& InText);

protected:
	// WBP에서 동일 이름의 TextBlock을 배치하면 자동 바인딩
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> EntranceTextBlock;
};
