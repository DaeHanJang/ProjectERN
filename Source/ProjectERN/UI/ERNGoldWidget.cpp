// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNGoldWidget.h"

#include "AbilitySystemComponent.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/TextBlock.h"
#include "GAS/ERNAttributeSet.h"

void UERNGoldWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	RefreshFromCurrentCharacter();
}

void UERNGoldWidget::NativeDestruct()
{
	// 골드 갱신 타이머 중지
	if (GetWorld())
	{
		if (GetWorld()->GetTimerManager().IsTimerActive(GoldChangedTimer))
		{
			GetWorld()->GetTimerManager().ClearTimer(GoldChangedTimer);
		}
	}
	
	// ASC 바인딩 해제
	if (UAbilitySystemComponent* ASC = BoundAbilitySystemComponent.Get())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetGoldAttribute()).Remove(GoldChangedHandle);
	}
	
	BoundAbilitySystemComponent = nullptr;
	
	Super::NativeDestruct();
}

void UERNGoldWidget::RefreshFromCurrentCharacter()
{
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (const AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(PC->GetPawn()))
		{
			if (UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent())
			{
				// 바인딩 및 ASC 캐싱
				GoldChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetGoldAttribute()).AddUObject(this, &UERNGoldWidget::OnGoldChanged);
								
				BoundAbilitySystemComponent = ASC;
				
				// 바인딩 후 골드 동기화
				TargetGold = PlayerCharacter->GetAttributeSet()->GetGold();	
				CurrentGold = PlayerCharacter->GetAttributeSet()->GetGold();
				
				UpdateGoldText();
			}
		}
	}
}

void UERNGoldWidget::OnGoldChanged(const FOnAttributeChangeData& Data)
{
	if (!GetWorld())
	{
		return;
	}
	
	// 타이머 활성화 시 중지 및 현재 골드 설정
	if (GetWorld()->GetTimerManager().IsTimerActive(GoldChangedTimer))
	{
		GetWorld()->GetTimerManager().ClearTimer(GoldChangedTimer);
		
		if (!LexTryParseString(CurrentGold, *GoldTextBlock->GetText().ToString()))
		{
			return;
		}
	}
	else
	{
		CurrentGold = static_cast<int32>(Data.OldValue);
	}
	
	// 목표 골드, 골드 폭 설정
	TargetGold = static_cast<int32>(Data.NewValue);
	const int32 Delta = TargetGold - CurrentGold;
	GoldStep = Delta / 20;
	if (GoldStep == 0 && Delta != 0)
	{
		GoldStep = Delta > 0 ? 1 : -1;
	}
	
	// 골드 갱신 애니메이션 타이머 시작
	GetWorld()->GetTimerManager().SetTimer(GoldChangedTimer, this, &UERNGoldWidget::UpdateGoldText, 0.05f, true, 0.0f);
}

void UERNGoldWidget::UpdateGoldText()
{
	CurrentGold += GoldStep;
	if (GoldStep > 0)
	{
		CurrentGold = FMath::Min(CurrentGold, TargetGold);
	}
	else
	{
		CurrentGold = FMath::Max(CurrentGold, TargetGold);
	}
	GoldTextBlock->SetText(FText::AsNumber(CurrentGold));
	
	if (CurrentGold == TargetGold)
	{
		GetWorld()->GetTimerManager().ClearTimer(GoldChangedTimer);
	}
}
