// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_SkillBase.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GAS/ERNGameplayTags.h"

UERNGA_SkillBase::UERNGA_SkillBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UERNGA_SkillBase::CheckCooldown(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCooldown(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return false;
	}

	if (!IsSkillCooldownConfigured())
	{
		return true;
	}

	// ASC에서 CooldownTags 태그확인
	const bool bOnCooldown = ActorInfo->AbilitySystemComponent->HasAnyMatchingGameplayTags(CooldownTags);

	if (bOnCooldown && OptionalRelevantTags)
	{
		OptionalRelevantTags->AppendTags(CooldownTags);
	}

	// 태그가 존재한다면 false return
	return !bOnCooldown;
}

void UERNGA_SkillBase::ApplyCooldown(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo) const
{
	// Actor가 ASC를 보유했을때만
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	// 스킬 쿨타임 설정 모두 부여했을 때
	if (!IsSkillCooldownConfigured())
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	// 기본 정보 세팅
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	// 실제로 적용할 Effect 명세서(Spec)를 만듦
	FGameplayEffectSpecHandle SpecHandle =
		ASC->MakeOutgoingSpec(
			CooldownEffectClass,
			GetAbilityLevel(Handle, ActorInfo),
			Context);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	// GE의 Duration을 BP에서 설정한 변수(CooldownDuration) 값으로 적용
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Cooldown, CooldownDuration);
	// 실제 쿨다운 상태 태그 부여
	SpecHandle.Data->DynamicGrantedTags.AppendTags(CooldownTags);

	// GE 적용
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

FName UERNGA_SkillBase::GetMontageSectionName(int32 Index) const
{
	return MontageSections.IsValidIndex(Index) ? MontageSections[Index].SectionName : NAME_None;
}

UAbilityTask_PlayMontageAndWait* UERNGA_SkillBase::PlayConfiguredMontage(int32 SectionIndex)
{
	if (!SkillMontage)
	{
		return nullptr;
	}

	const float PlayRate = MontageSections.IsValidIndex(SectionIndex) ? MontageSections[SectionIndex].PlayRate : 1.0f;

	UAbilityTask_PlayMontageAndWait* MontageTask =
		UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			SkillMontage,
			PlayRate,
			GetMontageSectionName(SectionIndex),
			bStopMontageWhenAbilityEnds);

	MontageTask->ReadyForActivation();
	return MontageTask;
}

const FGameplayTagContainer* UERNGA_SkillBase::GetCooldownTags() const
{
	// 쿨다운 태그가 존재하지 않는다면 null
	return CooldownTags.IsEmpty() ? nullptr : &CooldownTags;
}

bool UERNGA_SkillBase::IsSkillCooldownConfigured() const
{
	// 쿨다운 GE 존재, 쿨다운 기간 부여, 쿨다운 태그 존재
	return CooldownEffectClass && CooldownDuration > 0.f && !CooldownTags.IsEmpty();
}
