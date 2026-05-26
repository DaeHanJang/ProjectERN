// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_Normal_PaladinShield.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "DrawDebugHelpers.h"
#include "GAS/ERNAttributeSet.h"
#include "GAS/ERNGameplayTags.h"
#include "GameplayEffect.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

UERNGA_Normal_PaladinShield::UERNGA_Normal_PaladinShield()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UERNGA_Normal_PaladinShield::ApplyShieldFromNotify(USkeletalMeshComponent* MeshComp)
{
	AProjectERNCharacter* Caster = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo());
	if (!Caster)
	{
		return;
	}

	// Notify는 클라/서버 양쪽에서 호출될 수 있으므로 실제 GE 적용은 서버에서만 처리
	if (!Caster->HasAuthority())
	{
		return;
	}

	// 다른 캐릭터의 몽타주 Notify가 잘못 들어오는 것 방지
	if (MeshComp && MeshComp->GetOwner() != Caster)
	{
		return;
	}

	// 같은 몽타주 안에서 Notify가 여러 번 호출되어도 한 번만 적용
	if (bShieldAppliedThisActivation)
	{
		return;
	}

	ApplyShieldToAllies(Caster);
	bShieldAppliedThisActivation = true;
}

void UERNGA_Normal_PaladinShield::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                                  const FGameplayAbilityActorInfo* ActorInfo,
                                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                                  const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bShieldAppliedThisActivation = false;

	// 비용, 쿨다운, 활성 조건을 최종 확정
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AProjectERNCharacter* Caster = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo());
	if (!Caster)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	/* 몽타주에 Notify를 부여하여 스킬 시전으로 수정
	// 실제 실드 부여는 서버에서만 처리
	if (Caster->HasAuthority())
	{
		ApplyShieldToAllies(Caster);
	}
	*/

	// 몽타주가 있으면 재생 후 종료
	if (SkillMontage)
	{
		const float PlayRate = MontageSections.IsValidIndex(0)
			                       ? MontageSections[0].PlayRate
			                       : 1.0f;

		UAbilityTask_PlayMontageAndWait* MontageTask =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				NAME_None,
				SkillMontage,
				PlayRate,
				GetMontageSectionName(0),
				bStopMontageWhenAbilityEnds);

		if (MontageTask)
		{
			MontageTask->OnCompleted.AddDynamic(this, &UERNGA_Normal_PaladinShield::OnMontageCompleted);
			MontageTask->OnBlendOut.AddDynamic(this, &UERNGA_Normal_PaladinShield::OnMontageBlendOut);
			MontageTask->OnInterrupted.AddDynamic(this, &UERNGA_Normal_PaladinShield::OnMontageInterrupted);
			MontageTask->OnCancelled.AddDynamic(this, &UERNGA_Normal_PaladinShield::OnMontageCancelled);
			MontageTask->ReadyForActivation();
			return;
		}
	}

	// 몽타주가 없으면 실드만 적용하고 즉시 종료
	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UERNGA_Normal_PaladinShield::ApplyShieldToAllies(AProjectERNCharacter* Caster)
{
	// 서버에서 적용
	if (!Caster || !Caster->HasAuthority())
	{
		return;
	}

	// 적용 비율 계산 (0이면 return)
	const float CalculatedShieldAmount = CalculateShieldAmount(Caster);
	if (CalculatedShieldAmount <= 0.f)
	{
		return;
	}

	// 자기 자신 포함 옵션
	if (bIncludeSelf && IsValidShieldTarget(Caster, Caster))
	{
		ApplyShieldToTarget(Caster, Caster, CalculatedShieldAmount);
	}

	// 물리/충돌 검색을 위해 월드를 가져옴
	UWorld* World = Caster->GetWorld();
	if (!World || ShieldRadius <= 0.f)
	{
		return;
	}

	// 실드 적용 범위 Draw
	if (bDrawDebugShieldRadius)
	{
		DrawDebugSphere(
			World,
			Caster->GetActorLocation(),
			ShieldRadius,
			DebugShieldRadiusSegments,
			FColor::Cyan,
			false,
			DebugShieldRadiusDuration,
			0,
			DebugShieldRadiusThickness);
	}
	
	// 검색 결과를 담을 배열
	TArray<FOverlapResult> OverlapResults;

	// 검색 대상을 Pawn으로 제한
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	// 검색 대상에서 시전자 제외
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PaladinShieldOverlap), false);
	QueryParams.AddIgnoredActor(Caster);

	// 실제 범위 검색
	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		Caster->GetActorLocation(),
		FQuat::Identity, // 회전 없음
		ObjectQueryParams,
		FCollisionShape::MakeSphere(ShieldRadius),
		QueryParams);

	if (!bHasOverlap)
	{
		return;
	}

	// 한 캐릭터의 여러 컴포넌트가 감지되어도 한 번만 적용하기 위한 중복 방지
	TSet<AProjectERNCharacter*> AppliedTargets;

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AProjectERNCharacter* TargetCharacter = Cast<AProjectERNCharacter>(OverlapResult.GetActor());

		if (!IsValidShieldTarget(Caster, TargetCharacter))
		{
			continue;
		}

		if (AppliedTargets.Contains(TargetCharacter))
		{
			continue;
		}

		AppliedTargets.Add(TargetCharacter);
		ApplyShieldToTarget(Caster, TargetCharacter, CalculatedShieldAmount);
	}
}

void UERNGA_Normal_PaladinShield::ApplyShieldToTarget(AProjectERNCharacter* Caster,
                                                      AProjectERNCharacter* TargetCharacter,
                                                      float CalculatedShieldAmount)
{
	if (!Caster || !TargetCharacter)
	{
		return;
	}

	if (!ShieldAmountEffectClass || !ShieldStateEffectClass || CalculatedShieldAmount <= 0.f || ShieldDuration <= 0.f)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = Caster->GetAbilitySystemComponent();
	UAbilitySystemComponent* TargetASC = TargetCharacter->GetAbilitySystemComponent();
	if (!SourceASC || !TargetASC)
	{
		return;
	}

	if (bReplaceExistingShield)
	{
		FGameplayTagContainer ShieldTags;
		ShieldTags.AddTag(TAG_Buff_Shield);

		TargetASC->RemoveActiveEffectsWithGrantedTags(ShieldTags);
		TargetASC->SetNumericAttributeBase(UERNAttributeSet::GetShieldAttribute(), 0.f);
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	ContextHandle.AddInstigator(Caster, Caster);

	FGameplayEffectSpecHandle StateSpecHandle =
		SourceASC->MakeOutgoingSpec(ShieldStateEffectClass, GetAbilityLevel(), ContextHandle);

	FGameplayEffectSpecHandle AmountSpecHandle =
		SourceASC->MakeOutgoingSpec(ShieldAmountEffectClass, GetAbilityLevel(), ContextHandle);

	if (!StateSpecHandle.IsValid() || !AmountSpecHandle.IsValid())
	{
		return;
	}

	StateSpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Shield_Duration, ShieldDuration);
	AmountSpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Shield_Amount, CalculatedShieldAmount);

	FActiveGameplayEffectHandle StateEffectHandle =
		SourceASC->ApplyGameplayEffectSpecToTarget(*StateSpecHandle.Data.Get(), TargetASC);

	if (!StateEffectHandle.IsValid())
	{
		return;
	}

	if (FOnActiveGameplayEffectRemoved_Info* RemovedDelegate =
		TargetASC->OnGameplayEffectRemoved_InfoDelegate(StateEffectHandle))
	{
		RemovedDelegate->AddUObject(this, &UERNGA_Normal_PaladinShield::OnShieldStateEffectRemoved);
		ActiveShieldStateEffectTargets.Add(StateEffectHandle, TargetASC);
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*AmountSpecHandle.Data.Get(), TargetASC);
}

bool UERNGA_Normal_PaladinShield::IsValidShieldTarget(
	const AProjectERNCharacter* Caster,
	const AProjectERNCharacter* TargetCharacter) const
{
	if (!Caster || !TargetCharacter)
	{
		return false;
	}

	if (TargetCharacter->IsDead())
	{
		return false;
	}

	if (!bIncludeSelf && Caster == TargetCharacter)
	{
		return false;
	}

	return true;
}

float UERNGA_Normal_PaladinShield::CalculateShieldAmount(const AProjectERNCharacter* Caster) const
{
	if (!Caster || !Caster->GetAttributeSet())
	{
		return 0.f;
	}

	// 실드 적용량: 시전자의 최대 체력 * ShieldAmountRate
	const float CasterMaxHealth = Caster->GetAttributeSet()->GetMaxHealth();
	return FMath::Max(0.f, CasterMaxHealth * ShieldAmountRate);
}

void UERNGA_Normal_PaladinShield::OnShieldStateEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo)
{
	if (!RemovalInfo.ActiveEffect)
	{
		return;
	}

	const FActiveGameplayEffectHandle RemovedHandle = RemovalInfo.ActiveEffect->Handle;

	TWeakObjectPtr<UAbilitySystemComponent> TargetASCWeak;
	if (!ActiveShieldStateEffectTargets.RemoveAndCopyValue(RemovedHandle, TargetASCWeak))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = TargetASCWeak.Get();
	if (!TargetASC)
	{
		return;
	}

	TargetASC->SetNumericAttributeBase(UERNAttributeSet::GetShieldAttribute(), 0.f);
}

void UERNGA_Normal_PaladinShield::OnMontageCompleted()
{
	K2_EndAbility();
}

void UERNGA_Normal_PaladinShield::OnMontageBlendOut()
{
	K2_EndAbility();
}

void UERNGA_Normal_PaladinShield::OnMontageInterrupted()
{
	K2_EndAbility();
}

void UERNGA_Normal_PaladinShield::OnMontageCancelled()
{
	K2_EndAbility();
}
