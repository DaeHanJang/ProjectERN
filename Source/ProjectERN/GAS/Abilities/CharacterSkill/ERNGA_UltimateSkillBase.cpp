// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_UltimateSkillBase.h"

#include "AbilitySystemComponent.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Combat/ERNSkillDamageLibrary.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "GAS/ERNAttributeSet.h"
#include "GAS/ERNGameplayTags.h"

UERNGA_UltimateSkillBase::UERNGA_UltimateSkillBase()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Skill_Ultimate);
	SetAssetTags(AssetTags);

	// 태그 자동 부여
	// 이 베이스를 상속한 모든 궁극기는 궁극기 슬롯 쿨다운을 공유한다.
	CooldownTags.AddTag(TAG_Cooldown_Skill_Ultimate);
}

void UERNGA_UltimateSkillBase::TriggerExplosionFromNotify(USkeletalMeshComponent* MeshComp)
{
	AProjectERNCharacter* Caster = nullptr;
	if (!CanTriggerExplosionFromNotify(MeshComp, Caster))
	{
		return;
	}

	// 실제 대미지/판정은 서버에서만 처리한다.
	// GameplayCue를 붙일 때는 이 함수 안에서 서버가 Cue를 실행하도록 확장하면 된다.
	if (!Caster->HasAuthority())
	{
		return;
	}

	if (ExplosionData.bApplyOnlyOncePerActivation)
	{
		bExplosionAppliedThisActivation = true;
	}

	const FVector Origin = GetExplosionOrigin(Caster);
	// 폭발 GameplayCue 실행
	ExecuteExplosionGameplayCue(Caster, Origin);
	// 대미지 적용
	ApplyExplosionDamage(Caster, Origin);
}

void UERNGA_UltimateSkillBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                               const FGameplayAbilityActorInfo* ActorInfo,
                                               const FGameplayAbilityActivationInfo ActivationInfo,
                                               const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 궁극기를 새로 발동할 때마다 Notify 폭발 적용 기록을 초기화한다.
	bExplosionAppliedThisActivation = false;
}

bool UERNGA_UltimateSkillBase::CanTriggerExplosionFromNotify(USkeletalMeshComponent* MeshComp,
                                                             AProjectERNCharacter*& OutCaster) const
{
	OutCaster = nullptr;

	// 실행중인 어빌리티인지 확인 || BP 사용 여부 설정 확인
	if (!IsActive() || !bUseExplosion)
	{
		return false;
	}

	if (ExplosionData.bApplyOnlyOncePerActivation && bExplosionAppliedThisActivation)
	{
		return false;
	}

	AProjectERNCharacter* Caster = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo());
	if (!Caster)
	{
		return false;
	}

	// 다른 캐릭터의 몽타주 Notify가 잘못 들어오는 상황 방지.
	if (MeshComp && MeshComp->GetOwner() != Caster)
	{
		return false;
	}

	OutCaster = Caster;
	return true;
}

FVector UERNGA_UltimateSkillBase::GetExplosionOrigin(const AProjectERNCharacter* Caster) const
{
	return Caster->GetActorLocation()
		+ Caster->GetActorForwardVector() * ExplosionData.OriginOffset.X
		+ Caster->GetActorRightVector() * ExplosionData.OriginOffset.Y
		+ Caster->GetActorUpVector() * ExplosionData.OriginOffset.Z;
}

void UERNGA_UltimateSkillBase::ApplyExplosionDamage(AProjectERNCharacter* Caster, const FVector& Origin) const
{
	UWorld* World = Caster ? Caster->GetWorld() : nullptr;
	if (!World || ExplosionData.DamageRadius <= 0.f)
	{
		return;
	}

	if (ExplosionData.bDrawDebug)
	{
		DrawDebugSphere(
			World,
			Origin,
			ExplosionData.DamageRadius,
			32,
			FColor::Red,
			false,
			ExplosionData.DebugDrawTime,
			0,
			2.f);
	}

	TArray<FOverlapResult> OverlapResults;

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(UltimateExplosion), false);
	QueryParams.AddIgnoredActor(Caster);

	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		Origin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(ExplosionData.DamageRadius),
		QueryParams);

	if (!bHasOverlap)
	{
		return;
	}

	TSet<TWeakObjectPtr<AActor>> DamagedActors;

	for (const FOverlapResult& Result : OverlapResults)
	{
		AActor* HitActor = Result.GetActor();
		if (!HitActor || HitActor == Caster || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		const EERNSkillHitResult HitResult = UERNSkillDamageLibrary::ApplySkillHit(
			HitActor,
			Caster,
			Caster->GetController(),
			ExplosionData.DamageData,
			Origin);

		if (HitResult != EERNSkillHitResult::None)
		{
			DamagedActors.Add(HitActor);
		}
	}
}

void UERNGA_UltimateSkillBase::ExecuteCastGameplayCue()
{
	AProjectERNCharacter* Caster = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo());
	if (!Caster)
	{
		return;
	}

	ExecuteUltimateGameplayCue(
		CastCueData,
		Caster,
		Caster->GetActorLocation(),
		Caster->GetActorRotation());
}

void UERNGA_UltimateSkillBase::ExecuteExplosionGameplayCue(AProjectERNCharacter* Caster, const FVector& ExplosionOrigin)
{
	if (!Caster)
	{
		return;
	}

	ExecuteUltimateGameplayCue(
		ExplosionData.ExplosionCueData,
		Caster,
		ExplosionOrigin,
		Caster->GetActorRotation());
}

void UERNGA_UltimateSkillBase::ExecuteUltimateGameplayCue(const FERNUltimateGameplayCueData& CueData,
                                                          AProjectERNCharacter* Caster,
                                                          const FVector& BaseLocation,
                                                          const FRotator& BaseRotation) const
{
	// Cue가 유효할 때만 실행
	if (!CueData.bUseCue || !CueData.CueTag.IsValid() || !Caster)
	{
		return;
	}

	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	// ASC가 유효할 때 실행
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	// 서버 또는 로컬 컨트롤 캐릭터면 Cue 실행
	if (!Caster->HasAuthority() && !Caster->IsLocallyControlled())
	{
		return;
	}

	// Caster기준 Offset 적용
	const FVector CueLocation =
		BaseLocation
		+ Caster->GetActorForwardVector() * CueData.LocationOffset.X
		+ Caster->GetActorRightVector() * CueData.LocationOffset.Y
		+ Caster->GetActorUpVector() * CueData.LocationOffset.Z;

	// Rotation 적용
	const FRotator CueRotation = BaseRotation + CueData.RotationOffset;

	// Cue Parameter에 모두 적용
	FGameplayCueParameters CueParams;
	CueParams.Instigator = Caster;
	CueParams.EffectCauser = Caster;
	CueParams.SourceObject = this;
	CueParams.Location = CueLocation;
	CueParams.Normal = CueRotation.Vector();
	CueParams.RawMagnitude = CueData.CueScale;

	ActorInfo->AbilitySystemComponent->ExecuteGameplayCue(CueData.CueTag, CueParams);
}
