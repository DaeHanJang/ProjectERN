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
	
	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			UIManager->OnUIStateChanged.AddDynamic(this, &UERNGoldWidget::OnUIStateChanged);
			OnUIStateChanged(UIManager->GetActiveUIType());
		}
	}
}

void UERNGoldWidget::NativeDestruct()
{
	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			UIManager->OnUIStateChanged.RemoveDynamic(this, &UERNGoldWidget::OnUIStateChanged);
		}
	}

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
			UAbilitySystemComponent* NewASC = PlayerCharacter->GetAbilitySystemComponent();
			UERNAttributeSet* AttributeSet = PlayerCharacter->GetAttributeSet();
			if (!IsValid(NewASC) || !IsValid(AttributeSet))
			{
				return;
			}
			
			if (UAbilitySystemComponent* OldASC = BoundAbilitySystemComponent.Get())
			{
				if (OldASC != NewASC && GoldChangedHandle.IsValid())
				{
					OldASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetGoldAttribute()).Remove(GoldChangedHandle);
					GoldChangedHandle.Reset();
				}
			}
			
			if (BoundAbilitySystemComponent.Get() != NewASC || !GoldChangedHandle.IsValid())
			{
				// 바인딩 및 ASC 캐싱
				GoldChangedHandle = NewASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetGoldAttribute()).AddUObject(this, &UERNGoldWidget::OnGoldChanged);
								
				BoundAbilitySystemComponent = NewASC;
			}
			
			// 바인딩 후 골드 동기화
			TargetGold = FMath::FloorToInt(AttributeSet->GetGold());
			CurrentGold = TargetGold;
			GoldStep = 0;
			
			if (GetWorld())
			{
				GetWorld()->GetTimerManager().ClearTimer(GoldChangedTimer);
			}
			
			UpdateGoldText();
		}
	}
}

void UERNGoldWidget::OnUIStateChanged(EERNUIType UIType)
{
	if (UIType == EERNUIType::None || UIType == EERNUIType::Inventory)
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
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
	}
	else
	{
		CurrentGold = FMath::FloorToInt(Data.OldValue);
	}
	
	// 목표 골드, 골드 폭 설정
	TargetGold = FMath::FloorToInt(Data.NewValue);
	const int32 Delta = TargetGold - CurrentGold;
	if (Delta == 0)
	{
		UpdateGoldText();
		return;
	}
	
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
