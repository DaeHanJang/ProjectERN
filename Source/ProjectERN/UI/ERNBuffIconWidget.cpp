// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ERNBuffIconWidget.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"

void UERNBuffIconWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UERNBuffIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsCooldownActive && TotalDuration > 0.f)
	{
		TimeRemaining -= InDeltaTime;

		if (TimeRemaining <= 0.f)
		{
			TimeRemaining = 0.f;
			bIsCooldownActive = false;
			RemoveFromParent(); // 쿨다운 종료 시 리스트에서 자동 제거
		}

		if (CooldownBar)
		{
			// 남은 시간이 줄어들수록 0에서 1로 차오르도록 반전 (1.0 - 비율)
			CooldownBar->SetPercent(1.0f - (TimeRemaining / TotalDuration));
		}
	}
}

void UERNBuffIconWidget::InitBuff(const UItemDataAssetBase* BuffData, float InDuration)
{
	if (IconImage && BuffData && !BuffData->Icon.IsNull())
	{
		UTexture2D* Tex = BuffData->Icon.IsValid() ? BuffData->Icon.Get() : BuffData->Icon.LoadSynchronous();
		if (Tex)
		{
			IconImage->SetBrushFromTexture(Tex);
		}
	}

	TotalDuration = InDuration;
	TimeRemaining = TotalDuration;
	
	if (TotalDuration > 0.f)
	{
		bIsCooldownActive = true;
		if (CooldownBar)
		{
			// 처음에 시작할 때는 완전히 빈 상태(0)에서 시작
			CooldownBar->SetPercent(0.0f);
		}
	}
	else
	{
		// 지속시간이 0(무제한 등)일 경우 프로그레스 바 숨김
		bIsCooldownActive = false;
		if (CooldownBar)
		{
			CooldownBar->SetPercent(0.0f);
		}
	}
}
