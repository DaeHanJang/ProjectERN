// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/ERNTrainingDummy.h"

#include "GAS/ERNAttributeSet.h"
#include "TimerManager.h"

AERNTrainingDummy::AERNTrainingDummy()
{
	// AIControllerClass는 더미 BP에서 순찰 전용 BT를 가진 컨트롤러로 지정

	// 전과(킬수/총데미지) 집계 제외 — 연습용
	bExcludeFromCombatStats = true;
}

float AERNTrainingDummy::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// Mob의 어그로/동료 전파를 건너뛰기 위해 부모(EnemyCharacter)를 직접 호출
	// 데미지 적용 + 체력바/데미지텍스트는 유지, 추적/알림만 제외
	const float ActualDamage = AERNEnemyCharacter::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (HasAuthority() && ActualDamage > 0.f)
	{
		// 진행 중 회복 중단 + 회복 시작 딜레이 리셋 (맞을 때마다 갱신)
		GetWorldTimerManager().ClearTimer(RegenTickTimerHandle);
		GetWorldTimerManager().SetTimer(RegenDelayTimerHandle, this, &AERNTrainingDummy::StartRegen, RegenDelay, false);
	}

	return ActualDamage;
}

void AERNTrainingDummy::OnDeath()
{
	// 무적
}

void AERNTrainingDummy::StartRegen()
{
	if (!HasAuthority())
	{
		return;
	}

	GetWorldTimerManager().SetTimer(RegenTickTimerHandle, this, &AERNTrainingDummy::RegenTick, RegenTickInterval, true);
}

void AERNTrainingDummy::RegenTick()
{
	UERNAttributeSet* AttrSet = GetAttributeSet();
	if (AttrSet == nullptr)
	{
		GetWorldTimerManager().ClearTimer(RegenTickTimerHandle);
		return;
	}

	const float MaxHealth = AttrSet->GetMaxHealth();
	const float CurrentHealth = AttrSet->GetHealth();

	if (CurrentHealth >= MaxHealth)
	{
		GetWorldTimerManager().ClearTimer(RegenTickTimerHandle);
		return;
	}

	const float NewHealth = FMath::Min(MaxHealth, CurrentHealth + RegenPerSecond * RegenTickInterval);
	AttrSet->SetHealth(NewHealth);
}
