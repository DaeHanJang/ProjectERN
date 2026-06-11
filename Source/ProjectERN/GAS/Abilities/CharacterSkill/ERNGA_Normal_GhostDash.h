// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_NormalSkillBase.h"
#include "GAS/Abilities/CharacterSkill/ERNSkillNiagaraTypes.h"
#include "Engine/EngineTypes.h"
#include "ERNGA_Normal_GhostDash.generated.h"

class UMaterialInstanceDynamic;
class UMeshComponent;
class AProjectERNCharacter;

UCLASS()
class PROJECTERN_API UERNGA_Normal_GhostDash : public UERNGA_NormalSkillBase
{
	GENERATED_BODY()

public:
	UERNGA_Normal_GhostDash();

protected:
	// 스킬이 유지되는 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill", meta=(ClampMin="0.0"))
	float SkillDuration = 1.f;
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
	                        const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo,
	                        bool bReplicateEndAbility,
	                        bool bWasCancelled) override;
	
	// 투명화 비율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill")
	float GhostOpacity = 0.3f;
	
private:
	UFUNCTION()
	void FinishSkill();
	
	// 스킬 사용하는 동안 캐릭터를 투명하게 만들기 위함

	UPROPERTY()
	TArray<TObjectPtr<UMaterialInstanceDynamic>> GhostDashMaterials;

	void ApplyGhostDashVisualToMesh(UMeshComponent* MeshComp);
	void ApplyGhostDashVisual(AProjectERNCharacter* Character);
	void ClearGhostDashVisual();
	
protected:
	// 스킬 사용하는 동안 Pawn과의 충돌을 없앰
	// (투사체 채널은 무시하지 않음 — 프리즈 투사체 등이 물리적으로 명중해야 하고, 일반 투사체 데미지는 무적 태그가 0 처리)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill")
	TEnumAsByte<ECollisionChannel> GhostPassThroughChannel = ECC_Pawn;

private:
	// 이전 충돌 채널
	ECollisionResponse PreviousCapsuleResponse = ECR_Block;

	// 충돌 제거 적용
	void ApplyGhostDashCollision(AProjectERNCharacter* Character);
	// 충돌 제거 복구
	void ClearGhostDashCollision(AProjectERNCharacter* Character);
	
protected:
	// 스킬 지속 중 유지할 나이아가라 이펙트 배열
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill|Niagara")
	TArray<FERNSkillAttachedNiagaraEffect> GhostDashNiagaraEffects;

private:
	void StartGhostDashNiagaraEffects(AProjectERNCharacter* Character);
	void StopGhostDashNiagaraEffects(AProjectERNCharacter* Character);
};
