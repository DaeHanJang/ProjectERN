// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/AoE/ERNAoE_Heal.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Character/Player/ERNPlayerState.h"
#include "Combat/ERNSkillDamageLibrary.h"
#include "GAS/ERNAttributeSet.h"
#include "GAS/ERNGameplayTags.h"
#include "GameFramework/Pawn.h"

bool AERNAoE_Heal::IsValidAoETarget(AActor* TargetActor, UPrimitiveComponent* OverlappedComponent) const
{
	if (!Super::IsValidAoETarget(TargetActor, OverlappedComponent))
	{
		return false;
	}

	// 살아있는 적 캐릭터는 데미지 대상으로 본다. (컴포넌트 종류 무관 — Actor 단위 중복 적용은 베이스에서 방지)
	if (bDamageEnemiesPerTick)
	{
		if (AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(TargetActor))
		{
			return !Enemy->IsDead() && GetASCFromActor(SourceActor) != nullptr;
		}
	}

	// Sanctuary 힐 장판은 죽지 않은 현재 플레이어 캐릭터만 회복 대상으로 본다.
	AProjectERNCharacter* TargetCharacter = Cast<AProjectERNCharacter>(TargetActor);
	if (!TargetCharacter || !TargetCharacter->IsAlive())
	{
		return false;
	}

	// RootComponent가 Overlap됐을 때만 적용
	if (OverlappedComponent != TargetCharacter->GetRootComponent())
	{
		return false;
	}
	
	if (!HealEffectClass || HealPercentPerTick <= 0.f)
	{
		return false;
	}
	
	if (!GetASCFromActor(TargetCharacter))
	{
		return false;
	}
	
	return true;
}

void AERNAoE_Heal::ApplyAoEToTarget(AActor* TargetActor)
{
	// 적이면 시전자 스탯 비례 데미지(+경직) 적용
	if (bDamageEnemiesPerTick)
	{
		if (AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(TargetActor))
		{
			AController* InstigatorController = nullptr;
			if (APawn* SourcePawn = Cast<APawn>(SourceActor))
			{
				InstigatorController = SourcePawn->GetController();
			}

			UERNSkillDamageLibrary::ApplySkillHit(
				Enemy,
				SourceActor,
				InstigatorController,
				DamagePerTick,
				GetActorLocation());
			return;
		}
	}

	AProjectERNCharacter* TargetCharacter = Cast<AProjectERNCharacter>(TargetActor);
	if (!TargetCharacter)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetASCFromActor(SourceActor);
	UAbilitySystemComponent* TargetASC = GetASCFromActor(TargetCharacter);
	if (!SourceASC || !TargetASC || !HealEffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	ContextHandle.AddInstigator(SourceActor, this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(HealEffectClass, 1.f, ContextHandle);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 시전자 최대 체력 비례로 Tick당 회복량 산출
	const float SourceMaxHealth = SourceASC->GetNumericAttribute(UERNAttributeSet::GetMaxHealthAttribute());
	const float HealAmount = SourceMaxHealth * HealPercentPerTick;

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Heal_Amount, HealAmount);

	// 실제 회복량(오버힐 제외)을 측정하기 위해 적용 전후 체력 비교
	const float OldHealth = TargetASC->GetNumericAttribute(UERNAttributeSet::GetHealthAttribute());
	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	const float NewHealth = TargetASC->GetNumericAttribute(UERNAttributeSet::GetHealthAttribute());

	// 전과: 시전자(성기사)에게 실제 회복량 누적
	const float ActualHealed = NewHealth - OldHealth;
	if (ActualHealed > 0.f)
	{
		if (AProjectERNCharacter* CasterCharacter = Cast<AProjectERNCharacter>(SourceActor))
		{
			if (AERNPlayerState* CasterPS = CasterCharacter->GetPlayerState<AERNPlayerState>())
			{
				CasterPS->TotalUltimateHealed += ActualHealed;
			}
		}
	}
}

UAbilitySystemComponent* AERNAoE_Heal::GetASCFromActor(AActor* Actor) const
{
	if (!Actor)
	{
		return nullptr;
	}

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Actor);
	if (!AbilitySystemInterface)
	{
		return nullptr;
	}

	return AbilitySystemInterface->GetAbilitySystemComponent();
}
