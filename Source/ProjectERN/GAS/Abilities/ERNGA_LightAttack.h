// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/ERNGA_AttackAbility.h"
#include "ERNGA_LightAttack.generated.h"

USTRUCT(BlueprintType)
struct FERNComboSectionCost
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	float StaminaCost = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Cost")
	float ManaCost = 0.f;
};

UCLASS()
class PROJECTERN_API UERNGA_LightAttack : public UERNA_AttackAbility
{
	GENERATED_BODY()
	
public:
	UERNGA_LightAttack();

	// BP의 K2_ActivateAbility 시작 지점에서 호출.
	// 콤보 상태를 1타 기준으로 초기화한다.
	UFUNCTION(BlueprintCallable, Category="Attack|Combo")
	void ResetComboState();

	// ProjectERNCharacter::LightAttack에서 공격 중 추가 입력이 들어오면 호출.
	void CacheComboInput(const FRotator& TargetRotation);

	// AnimNotify_CheckLightCombo에서 호출.
	// 입력이 캐시되어 있으면 다음 Montage Section으로 이동한다.
	UFUNCTION(BlueprintCallable, Category="Attack|Combo")
	void CheckCombo();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ERN|Cost")
	TArray<FERNComboSectionCost> ComboSectionCosts;
	
	// 0 = 1타 section, 1 = 2타 section ...
	int32 CurrentComboIndex = 0;

	// 공격 중 다음 LightAttack 입력이 들어왔는지 저장.
	bool bHasNextComboInput = false;

	FName GetComboSectionName(int32 ComboIndex) const;

	bool CanMoveToNextCombo() const;
	
	bool bHasCachedComboRotation = false;
	FRotator CachedComboRotation = FRotator::ZeroRotator;
};
