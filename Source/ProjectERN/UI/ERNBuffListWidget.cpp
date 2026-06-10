// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ERNBuffListWidget.h"
#include "UI/ERNBuffIconWidget.h"
#include "Character/Player/ERNPlayerState.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Inventory/Item/Data/ConsumableItemDataAsset.h"
#include "Components/HorizontalBox.h"

void UERNBuffListWidget::InitializeWithPlayerState(AERNPlayerState* PS)
{
	if (!PS || WeakPS.Get() == PS)
	{
		return;
	}

	// 기존 바인딩 해제 및 아이콘 정리
	if (WeakPS.IsValid())
	{
		WeakPS->OnConsumableBuffAdded.RemoveDynamic(this, &UERNBuffListWidget::OnConsumableBuffAdded);
	}

	if (BuffContainerBox)
	{
		BuffContainerBox->ClearChildren();
	}
	ActiveBuffWidgets.Empty();

	WeakPS = PS;

	// 새 PlayerState 이벤트 바인딩
	WeakPS->OnConsumableBuffAdded.AddDynamic(this, &UERNBuffListWidget::OnConsumableBuffAdded);
	
	UE_LOG(LogTemp, Warning, TEXT("[BuffUI] ERNBuffListWidget bound to PlayerState: %s"), *WeakPS->GetName());
}

void UERNBuffListWidget::NativeDestruct()
{
	if (WeakPS.IsValid())
	{
		WeakPS->OnConsumableBuffAdded.RemoveDynamic(this, &UERNBuffListWidget::OnConsumableBuffAdded);
	}

	Super::NativeDestruct();
}

void UERNBuffListWidget::OnConsumableBuffAdded(const FName& ItemID, float Duration)
{
	UE_LOG(LogTemp, Warning, TEXT("[BuffUI] OnConsumableBuffAdded called. ItemID: %s, Duration: %f"), *ItemID.ToString(), Duration);

	if (!BuffContainerBox)
	{
		UE_LOG(LogTemp, Error, TEXT("[BuffUI] BuffContainerBox is NULL!"));
		return;
	}
	if (!BuffIconWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[BuffUI] BuffIconWidgetClass is NULL! Did you assign it in the Blueprint Details?"));
		return;
	}
	if (!GetWorld() || !GetWorld()->GetGameInstance()) return;

	UItemManagerSubsystem* ItemManager = GetWorld()->GetGameInstance()->GetSubsystem<UItemManagerSubsystem>();
	if (!ItemManager) return;

	const UConsumableItemDataAsset* DA = Cast<UConsumableItemDataAsset>(ItemManager->LoadItemDataAssetSync(ItemID, EItemAssetLoadFlags::UI));
	if (!DA)
	{
		UE_LOG(LogTemp, Error, TEXT("[BuffUI] Failed to load ConsumableItemDataAsset for ItemID: %s"), *ItemID.ToString());
		return;
	}
	
	if (Duration <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BuffUI] Duration is 0 or less! Icon will not show. Duration: %f"), Duration);
		// 임시로 디버깅을 위해 5초로 강제 설정 (원인을 찾기 전까지 아이콘이 보이게 함)
		Duration = 5.0f;
	}

	// 이미 리스트에 있으면 리프레시 (지속시간 갱신)
	if (UERNBuffIconWidget** ExistingWidget = ActiveBuffWidgets.Find(ItemID))
	{
		if (IsValid(*ExistingWidget))
		{
			(*ExistingWidget)->InitBuff(DA, Duration);
			// 위젯이 시간이 다 되어 부모에서 지워졌던 상태라면 다시 추가
			if (!(*ExistingWidget)->GetParent())
			{
				BuffContainerBox->AddChildToHorizontalBox(*ExistingWidget);
			}
			return;
		}
	}

	// 새로 생성
	UERNBuffIconWidget* NewIconWidget = CreateWidget<UERNBuffIconWidget>(this, BuffIconWidgetClass);
	if (NewIconWidget)
	{
		NewIconWidget->InitBuff(DA, Duration);
		BuffContainerBox->AddChildToHorizontalBox(NewIconWidget);
		ActiveBuffWidgets.Add(ItemID, NewIconWidget);
	}

	// ActiveBuffWidgets에서 지워진 위젯들(시간 만료로 자가 삭제된 위젯들) 정리
	for (auto It = ActiveBuffWidgets.CreateIterator(); It; ++It)
	{
		if (!IsValid(It.Value()) || !It.Value()->GetParent())
		{
			It.RemoveCurrent();
		}
	}
}
