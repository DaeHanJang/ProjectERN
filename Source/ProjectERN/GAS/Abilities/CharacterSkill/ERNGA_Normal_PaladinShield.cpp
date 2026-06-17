// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_Normal_PaladinShield.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SkeletalMeshComponent.h"
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
	TryApplyShield(MeshComp);
}

void UERNGA_Normal_PaladinShield::OnApplyShieldEventReceived(FGameplayEventData Payload)
{
	if (!IsApplyShieldEventFromAvatar(Payload))
	{
		return;
	}

	TryApplyShield(GetAvatarMeshFromActorInfo());
}

bool UERNGA_Normal_PaladinShield::TryApplyShield(USkeletalMeshComponent* MeshComp)
{
	AProjectERNCharacter* Caster = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo());
	if (!Caster)
	{
		return false;
	}

	if (!Caster->HasAuthority())
	{
		return false;
	}

	if (MeshComp && MeshComp->GetOwner() != Caster)
	{
		return false;
	}

	if (bShieldAppliedThisActivation)
	{
		return false;
	}

	ApplyShieldToAllies(Caster);
	bShieldAppliedThisActivation = true;

	return true;
}

bool UERNGA_Normal_PaladinShield::IsApplyShieldEventFromAvatar(const FGameplayEventData& Payload) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return false;
	}

	const AActor* EventInstigator = Payload.Instigator.Get();
	const AActor* EventTarget = Payload.Target.Get();

	return (!EventInstigator || EventInstigator == AvatarActor) &&
		(!EventTarget || EventTarget == AvatarActor);
}

USkeletalMeshComponent* UERNGA_Normal_PaladinShield::GetAvatarMeshFromActorInfo() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (ActorInfo && ActorInfo->SkeletalMeshComponent.IsValid())
	{
		return ActorInfo->SkeletalMeshComponent.Get();
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? AvatarActor->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
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

	UAbilityTask_WaitGameplayEvent* ApplyShieldEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			TAG_Event_Skill_Normal_PaladinShield_Apply);

	ApplyShieldEventTask->EventReceived.AddDynamic(
		this,
		&UERNGA_Normal_PaladinShield::OnApplyShieldEventReceived);
	ApplyShieldEventTask->ReadyForActivation();

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

	// 전과: 이 배리어로 막아낸 데미지를 시전 성기사에게 귀속하기 위해 시전자 기록
	TargetCharacter->SetShieldInstigator(Caster);

	if (FOnActiveGameplayEffectRemoved_Info* RemovedDelegate =
		TargetASC->OnGameplayEffectRemoved_InfoDelegate(StateEffectHandle))
	{
		RemovedDelegate->AddUObject(this, &UERNGA_Normal_PaladinShield::OnShieldStateEffectRemoved);
		ActiveShieldStateEffectTargets.Add(StateEffectHandle, { TargetASC, CalculatedShieldAmount });
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

	if (!Caster->IsAlive() || !TargetCharacter->IsAlive())
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

	FShieldStateTracking Tracking;
	if (!ActiveShieldStateEffectTargets.RemoveAndCopyValue(RemovedHandle, Tracking))
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = Tracking.TargetASC.Get();
	if (!TargetASC || !TargetASC->IsOwnerActorAuthoritative())
	{
		return;
	}

	// 실드가 막아준 데미지(= 깎인 실드량 = 부여량 − 남은 실드)만큼 체력 회복
	const float RemainingShield = TargetASC->GetNumericAttribute(UERNAttributeSet::GetShieldAttribute());
	const float ConsumedShield = FMath::Max(0.f, Tracking.GrantedAmount - RemainingShield);

	// 살아있는 대상만 회복 (사망 시 부활 방지)
	const AProjectERNCharacter* TargetChar = Cast<AProjectERNCharacter>(TargetASC->GetAvatarActor());
	if (ConsumedShield > 0.f && TargetChar && TargetChar->IsAlive())
	{
		const float CurrentHealth = TargetASC->GetNumericAttribute(UERNAttributeSet::GetHealthAttribute());
		const float MaxHealth = TargetASC->GetNumericAttribute(UERNAttributeSet::GetMaxHealthAttribute());
		const float NewHealth = FMath::Clamp(CurrentHealth + ConsumedShield, 0.f, MaxHealth);
		TargetASC->SetNumericAttributeBase(UERNAttributeSet::GetHealthAttribute(), NewHealth);
	}

	// 남은 실드 정리
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
