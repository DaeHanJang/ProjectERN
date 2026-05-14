// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "ERNCharacterBase.generated.h"

class UAbilitySystemComponent;
class UERNAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UAnimMontage;

UCLASS(Abstract)
class PROJECTERN_API AERNCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AERNCharacterBase();
	
	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	// AttributeSet 가져오기
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	UERNAttributeSet* GetAttributeSet() const { return AttributeSet; }
	
	// 무기 스킬 어빌리티 설정
	void SetWeaponAbility(TSubclassOf<UGameplayAbility> NewWeaponAbility);
	
	// 소모품 어빌리티 설정
	void SetConsumableAbility(TSubclassOf<UGameplayAbility> NewConsumableAbility);

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;

	// 기본 어빌리티 목록 (블루프린트에서 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;
	
	// 무기 스킬 어빌리티 핸들
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ABilities")
	FGameplayAbilitySpecHandle WeaponAbilityHandle;
	
	// 소모품 어빌리티 핸들
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ABilities")
	FGameplayAbilitySpecHandle ConsumableAbilityHandle;

	void GiveDefaultAbilities();
	void InitializeAbilitySystemActorInfo();

	// GAS 컴포넌트들
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UERNAttributeSet> AttributeSet;

	// 사망 처리
	UFUNCTION(BlueprintCallable, Category = "Character")
	virtual void OnDeath();

	UPROPERTY(BlueprintReadOnly, Category = "Character")
	bool bIsDead = false;

public:
	// 데미지 처리
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Character")
	bool IsDead() const { return bIsDead; }

	// 경직 시스템
	UFUNCTION(BlueprintCallable, Category = "Combat")
	void TryApplyStagger(float IncomingStaggerPower);

	// 에디터에서 GE_Stagger 블루프린트 연결
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Stagger")
	TSubclassOf<UGameplayEffect> StaggerEffect;

	// 에디터에서 히트리액션 몽타주 연결
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Stagger")
	TObjectPtr<UAnimMontage> HitReactionMontage;

	// 히트리액션 몽타주 재생 (모든 클라이언트에 동기화)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayHitReaction();
	
protected:
	// 움직임 상태태그 변화를 위한 함수
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
};
