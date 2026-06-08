// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/ERNSkillDamageTypes.h"
#include "GAS/Abilities/CharacterSkill/ERNSkillNiagaraTypes.h"
#include "GAS/Abilities/ERNGA_WeaponSkill.h"
#include "GAS/Abilities/WeaponSkill/ERNWeaponSkillTypes.h"
#include "ERNGA_WeaponSkill_Channeling.generated.h"

USTRUCT(BlueprintType)
struct FERNWeaponSkillChannelOriginData
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Origin")
	EWeaponSkillAreaOriginMode OriginMode = EWeaponSkillAreaOriginMode::WeaponHitbox;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Origin",
		meta=(EditCondition="OriginMode == EWeaponSkillAreaOriginMode::MeshSocket", EditConditionHides))
	FName MeshSocketName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Origin")
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Origin")
	FRotator RotationOffset = FRotator::ZeroRotator;
};

USTRUCT(BlueprintType)
struct FERNWeaponSkillChannelingData
{
	GENERATED_BODY()

	FERNWeaponSkillChannelingData() : DamageData(0.f, 1.f, 1.f, 0.f, 1.f)
	{
	}

	// 대미지 데이터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Channeling|Damage")
	FERNSkillDamageData DamageData;
	
	// 피해 적용, 비용 처리 간격
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Channeling")
	float TickInterval = 0.25f;

	// 최대 유지 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Channeling")
	float MaxChannelDuration = 0.f;

	// TickInterval마다 소모될 마나
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Channeling|Cost")
	float ManaCostPerTick = 0.f;
	
	// 판정 길이(판정은 원기둥)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Channeling|Damage")
	float DamageLength = 500.f;

	// 판정 두께
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Channeling|Damage")
	float DamageRadius = 80.f;

	// 스킬 적용 위치
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Channeling|Origin")
	FERNWeaponSkillChannelOriginData OriginData;

	// 몽타주 Loop구간 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Channeling|Montage")
	FName LoopSectionName = TEXT("Loop");

	// 몽타주 End구간 섹션 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Channeling|Montage")
	FName EndSectionName = TEXT("End");
	
	// 나이아가라 효과 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Channeling|Effect")
	TArray<FERNSkillAttachedNiagaraEffect> ChannelingNiagaraEffects;
	
	// 디버그용 드로우 활성화 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Channeling|Debug")
	bool bDrawDebug = false;
};

UCLASS()
class PROJECTERN_API UERNGA_WeaponSkill_Channeling : public UERNGA_WeaponSkill
{
	GENERATED_BODY()
	
public:
	UERNGA_WeaponSkill_Channeling();
	
	// Notify에서 채널링 시작
	UFUNCTION(BlueprintCallable, Category="WeaponSkill|Channeling")
	void StartChannelingFromNotify(USkeletalMeshComponent* MeshComp);

	// 채널링 중지
	UFUNCTION(BlueprintCallable, Category="WeaponSkill|Channeling")
	void StopChanneling();

	// 채널링 종료 요청
	UFUNCTION(BlueprintCallable, Category="WeaponSkill|Channeling")
	void RequestEndChanneling();
	
	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WeaponSkill|Channeling")
	FERNWeaponSkillChannelingData ChannelingData;

private:
	// 채널링 대미지 틱을 반복 실행하기 위한 타이머 핸들
	FTimerHandle ChannelTickTimerHandle;
	
	// 채널링 시작 Notify가 발생한 MeshComp를 저장
	// 이후 틱마다 이 Mesh 기준으로 발사 위치와 방향을 계산
	TWeakObjectPtr<USkeletalMeshComponent> CachedMeshComp;
	
	// 실제 타이머에 사용 중인 틱 간격
	float ActiveTickInterval = 0.f;
	
	// 채널링이 시작된 뒤 누적된 시간
	float ChannelElapsedTime = 0.f;
	
	// 채널링 지속 여부
	bool bIsChanneling = false;

	// TickInterval마다 호출되는 채널링 처리 함수
	void TickChanneling();
	
	// 현재 채널링 원점 기준으로 원기둥형 범위를 검사하고, 적에게 대미지 + 경직 부여
	void ApplyChannelDamage(USkeletalMeshComponent* MeshComp);
	
	// 채널링 시작 시 지속 이펙트 소환
	void StartChannelingNiagaraEffects();
	
	// 채널링 종료 시 지속 이펙트 제거
	void StopChannelingNiagaraEffects();
	
	// OriginData 설정에 따라 채널링의 위치, 회전, 부착 컴포넌트를 계산
	bool GetChannelOriginTransform(
	USkeletalMeshComponent* MeshComp,
	FTransform& OutTransform,
	USceneComponent*& OutAttachComponent) const;
};
