// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNLevelUpWidget.h"

#include "Character/Player/ERNPlayerStatusTable.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/TextBlock.h"
#include "GAS/ERNAttributeSet.h"

UERNLevelUpWidget::UERNLevelUpWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UERNLevelUpWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (const AERNCharacterBase* PlayerCharacter = Cast<AERNCharacterBase>(PC->GetPawn()))
		{
			OnLevelChangedHandle = PlayerCharacter->GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetLevelAttribute()).AddUObject(this, &UERNLevelUpWidget::OnLevelChanged);
		}
	}
	
	RefreshTextByCurrentLevel();
}

void UERNLevelUpWidget::NativeDestruct()
{
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (const AERNCharacterBase* PlayerCharacter = Cast<AERNCharacterBase>(PC->GetPawn()))
		{
			PlayerCharacter->GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetLevelAttribute()).Remove(OnLevelChangedHandle);
		}
	}
	
	Super::NativeDestruct();
}

FReply UERNLevelUpWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::E || InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Top)
	{
		if (const APlayerController* PC = GetOwningPlayer())
		{
			if (AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(PC->GetPawn()))
			{
				PlayerCharacter->Server_LevelUp();
				
				return FReply::Handled();
			}
		}
	}
	
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UERNLevelUpWidget::OnLevelChanged(const FOnAttributeChangeData& Data)
{
	RefreshTextByCurrentLevel();

	if (AERNCharacterBase* PlayerCharacter = Cast<AERNCharacterBase>(GetOwningPlayerPawn()))
	{
		PlayerCharacter->Server_RequestEffectAndSound(Effect, PlayerCharacter->GetActorLocation(), Sound, PlayerCharacter->GetActorLocation());
	}
}

void UERNLevelUpWidget::RefreshTextByCurrentLevel()
{
	UDataTable* DT = nullptr;
	UERNAttributeSet* AS = nullptr;
	if (const APlayerController* PC = GetOwningPlayer())
	{
		if (const AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(PC->GetPawn()))
		{
			DT = PlayerCharacter->GetStatusCurveTable();
			AS = PlayerCharacter->GetAttributeSet();
		}
	}
	
	if (!DT || !AS)
	{
		return;
	}
	
	const FName CurrentLevel(FString::FromInt(static_cast<int32>(AS->GetLevel())));
	const FName NextLevel(FString::FromInt(static_cast<int32>(AS->GetLevel()) + 1));
	
	const FERNPlayerStatusTable* CurrentRow = DT->FindRow<FERNPlayerStatusTable>(CurrentLevel, TEXT("CurrentLevelRow"));
	const FERNPlayerStatusTable* NextRow = DT->FindRow<FERNPlayerStatusTable>(NextLevel, TEXT("CurrentLevelRow"));
	
	CurrentLevelTextBlock->SetText(FText::AsNumber(CurrentRow->Level));	
	CurrentHealthTextBlock->SetText(FText::AsNumber(CurrentRow->MaxHealth));	
	CurrentManaTextBlock->SetText(FText::AsNumber(CurrentRow->MaxMana));	
	CurrentStaminaTextBlock->SetText(FText::AsNumber(CurrentRow->MaxStamina));	
	CurrentStrengthTextBlock->SetText(FText::AsNumber(CurrentRow->AttackPower));	
	CurrentDefenceTextBlock->SetText(FText::AsNumber(CurrentRow->Defense));	
	CurrentStaggerTextBlock->SetText(FText::AsNumber(CurrentRow->StaggerResistance));
	
	if (!NextRow)
	{
		LevelUpCostTextBlock->SetText(FText::FromString("MAX"));
		NextLevelTextBlock->SetText(FText::FromString("MAX"));
		NextHealthTextBlock->SetText(FText::FromString("MAX"));
		NextManaTextBlock->SetText(FText::FromString("MAX"));
		NextStaminaTextBlock->SetText(FText::FromString("MAX"));
		NextStrengthTextBlock->SetText(FText::FromString("MAX"));
		NextDefenceTextBlock->SetText(FText::FromString("MAX"));
		NextStaggerTextBlock->SetText(FText::FromString("MAX"));
	}
	else
	{
		LevelUpCostTextBlock->SetText(FText::AsNumber(NextRow->Cost));
		NextLevelTextBlock->SetText(FText::AsNumber(NextRow->Level));
		NextHealthTextBlock->SetText(FText::AsNumber(NextRow->MaxHealth));
		NextManaTextBlock->SetText(FText::AsNumber(NextRow->MaxMana));
		NextStaminaTextBlock->SetText(FText::AsNumber(NextRow->MaxStamina));
		NextStrengthTextBlock->SetText(FText::AsNumber(NextRow->AttackPower));
		NextDefenceTextBlock->SetText(FText::AsNumber(NextRow->Defense));
		NextStaggerTextBlock->SetText(FText::AsNumber(NextRow->StaggerResistance));
	}
}
