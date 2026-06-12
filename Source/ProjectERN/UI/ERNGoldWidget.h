// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GAS/ERNAttributeSet.h"
#include "UI/ERNUIManagerSubsystem.h"
#include "ERNGoldWidget.generated.h"

class UTextBlock;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNGoldWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// 현재 캐릭터에 이벤트 바인딩
	void RefreshFromCurrentCharacter();
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
private:
	// 골드 변경 핸들러
	void OnGoldChanged(const FOnAttributeChangeData& Data);
	
	// UI 상태 변경 핸들러
	UFUNCTION()
	void OnUIStateChanged(EERNUIType UIType);
	
	void UpdateGoldText();
	
private:
	// Gold
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> GoldTextBlock;
	
	// 현재 바인딩된 ASC
	UPROPERTY(Transient)
	TWeakObjectPtr<UAbilitySystemComponent> BoundAbilitySystemComponent;
	
	// 골드 변경 핸들
	FDelegateHandle GoldChangedHandle;
	
	// 골드 변경 효과 타이머
	FTimerHandle GoldChangedTimer;
	// 목표 골드
	int32 TargetGold = 0;
	// 현재 골드
	int32 CurrentGold = 0;
	// 현재 골드부터 목표 골드까지의 상승폭
	int32 GoldStep = 0;
	
};
