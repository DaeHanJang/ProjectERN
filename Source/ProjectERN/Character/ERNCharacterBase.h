// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpecHandle.h"
#include "ERNCharacterBase.generated.h"

class UNiagaraSystem;
class UAbilitySystemComponent;
class UERNAttributeSet;
class UGameplayAbility;
class UGameplayEffect;
class UAnimMontage;

// 히트 리액션 방향 (수평면 4사분면)
UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Front	UMETA(DisplayName = "Front"),
	Back	UMETA(DisplayName = "Back"),
	Left	UMETA(DisplayName = "Left"),
	Right	UMETA(DisplayName = "Right")
};

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
	
	// 골드 추가
	UFUNCTION(BlueprintCallable, Category = "Attributes")
	void AddGold(const int32 Amount) const;

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

	// 막타 크레딧용 — 마지막으로 유효타를 넣은 컨트롤러 (서버)
	TWeakObjectPtr<AController> LastHitInstigator;

public:
	// 데미지 처리
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	UFUNCTION(BlueprintPure, Category = "Character")
	bool IsDead() const { return bIsDead; }

	// 타겟팅 가능 여부 (락온/유도 검색에서 사용) — 죽었거나 무형 상태면 불가
	UFUNCTION(BlueprintPure, Category = "Character")
	bool IsTargetable() const { return !bIsDead && !bIsIntangible; }

	// 무형(사라짐) 상태 설정 — 충돌 끄기 + 추적 중인 투사체 유도 해제
	// (서버에서 호출, 클라는 OnRep로 동기화)
	UFUNCTION(BlueprintCallable, Category = "Character")
	void SetIntangible(bool bNewIntangible);

protected:
	// 무형 상태 (충돌/락온/유도 전부 무시되는 "그 자리에 없는" 상태)
	UPROPERTY(ReplicatedUsing = OnRep_Intangible, BlueprintReadOnly, Category = "Character")
	bool bIsIntangible = false;

	UFUNCTION()
	void OnRep_Intangible();

	// 무형 상태 적용 (충돌 토글 + 유도 해제) — 서버/클라 양쪽에서 실행
	void ApplyIntangible();

public:

	// 경직 시스템 — HitOrigin은 공격자/투사체/폭발 중심 등 데미지 발원 위치 (ZeroVector면 방향 계산 스킵하고 Front fallback)
	UFUNCTION(BlueprintCallable, Category = "Combat")
	virtual void TryApplyStagger(float IncomingStaggerPower, const FVector& HitOrigin = FVector::ZeroVector);

	// 에디터에서 GE_Stagger 블루프린트 연결
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Stagger")
	TSubclassOf<UGameplayEffect> StaggerEffect;

	// 기본 히트리액션 몽타주 (4방향 몽타주가 비어 있을 때 fallback)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Stagger")
	TObjectPtr<UAnimMontage> HitReactionMontage;

	// 방향별 히트리액션 몽타주 (앞/뒤/좌/우 — 비어 있으면 HitReactionMontage로 fallback)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Stagger")
	TObjectPtr<UAnimMontage> HitReactionMontage_Front;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Stagger")
	TObjectPtr<UAnimMontage> HitReactionMontage_Back;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Stagger")
	TObjectPtr<UAnimMontage> HitReactionMontage_Left;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Stagger")
	TObjectPtr<UAnimMontage> HitReactionMontage_Right;

	// 다운 몽타주 (StaggerPower >= DownResistance 일 때 재생)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Combat|Stagger")
	TObjectPtr<UAnimMontage> DownMontage;

	// 사망 몽타주 (적/플레이어 공통)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat")
	TObjectPtr<UAnimMontage> DeathMontage;

	// 히트리액션 몽타주 재생 (모든 클라이언트에 동기화) — 재생할 몽타주를 인자로 전달
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayHitReaction(UAnimMontage* Montage);
	
	// VFX, SFX 재생 요청
	UFUNCTION(Server, Reliable)
	void Server_RequestEffectAndSound(UNiagaraSystem* Effect, FVector EffectLocation, USoundBase* Sound, FVector SoundLocation);
	
	// VFX, SFX 재생
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable)
	void Multicast_PlayEffectAndSound(UNiagaraSystem* Effect, FVector EffectLocation, USoundBase* Sound, FVector SoundLocation);

protected:
	// 공격자(HitOrigin) → 내 캐릭터 기준 4방향 판별 (Z 무시, 수평면)
	EHitDirection ComputeHitDirection(const FVector& HitOrigin) const;

public:

	// 사망 몽타주 재생 (모든 클라이언트에 동기화)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayDeathMontage();

	// 컷신용 몽타주 재생 (시퀀서 Event Track에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Animation|Cutscene")
	void PlayCutsceneMontage(UAnimMontage* Montage);

	// 컷신용 Speed (서버에서 계산 → 리플리케이트)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Animation|Cutscene")
	float CutsceneSpeed = 0.f;

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// 컷신 Speed 계산용
	FVector CutscenePrevLocation = FVector::ZeroVector;
	bool bCutscenePrevLocationValid = false;

protected:
	// 움직임 상태태그 변화를 위한 함수
	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;
};
