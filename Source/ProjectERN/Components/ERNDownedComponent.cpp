// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/ERNDownedComponent.h"

#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
UERNDownedComponent::UERNDownedComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UERNDownedComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(UERNDownedComponent, DownedHealth);
	DOREPLIFETIME(UERNDownedComponent, MaxDownedHealth);
	DOREPLIFETIME(UERNDownedComponent, PenaltyStacks);
}

void UERNDownedComponent::EnterDownedState(int32 DeathPenaltyStacks)
{
	// 서버에서만 적용
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 기절 상태 체력 적용
	PenaltyStacks = FMath::Clamp(DeathPenaltyStacks, 0, MaxPenaltyStacks);
	MaxDownedHealth = BaseDownedHealth + DownedHealthPerPenaltyStack * PenaltyStacks;
	DownedHealth = MaxDownedHealth;

	// 게이지 변화 적용
	BroadcastGaugeChanged();
	GetOwner()->ForceNetUpdate();
}

void UERNDownedComponent::ApplyReviveHit(AController* Reviver, float ReviveHitScale)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || DownedHealth <= 0.f)
	{
		return;
	}

	// 아군 여부 검증은 무기 타격 처리 연결 단계에서 추가한다.
	(void)Reviver;

	// 음수 방어
	DownedHealth = FMath::Max(0.f, DownedHealth - (ReviveHitAmount * FMath::Max(0.f, ReviveHitScale)));

	BroadcastGaugeChanged();
	GetOwner()->ForceNetUpdate();

	// 기절 상태 종료 알림
	if (DownedHealth <= 0.f)
	{
		OnReviveGaugeDepleted.Broadcast();
	}
}

void UERNDownedComponent::ExitDownedState()
{
	// 서버에서만 처리
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 기절 상태 종료
	DownedHealth = 0.f;
	MaxDownedHealth = 0.f;
	
	BroadcastGaugeChanged();
	GetOwner()->ForceNetUpdate();
}

float UERNDownedComponent::GetDownedHealthPercent() const
{
	// 기절 상태 HP 비율 계산
	return MaxDownedHealth > 0.f ? DownedHealth / MaxDownedHealth : 0.f;
}

void UERNDownedComponent::OnRep_DownedHealth()
{
	BroadcastGaugeChanged();
}

void UERNDownedComponent::OnRep_MaxDownedHealth()
{
	BroadcastGaugeChanged();
}

void UERNDownedComponent::OnRep_PenaltyStacks()
{
	BroadcastGaugeChanged();
}

void UERNDownedComponent::BroadcastGaugeChanged()
{
	OnDownedGaugeChanged.Broadcast(DownedHealth, MaxDownedHealth, PenaltyStacks);
}

float UERNDownedComponent::GetMaxPossibleDownedHealth() const
{
	return BaseDownedHealth + DownedHealthPerPenaltyStack * MaxPenaltyStacks;
}

float UERNDownedComponent::GetDownedHealthGlobalPercent() const
{
	const float MaxPossibleHealth = GetMaxPossibleDownedHealth();
	return MaxPossibleHealth > 0.f ? DownedHealth / MaxPossibleHealth : 0.f;
}

float UERNDownedComponent::GetActiveDownedMaxGlobalPercent() const
{
	const float MaxPossibleHealth = GetMaxPossibleDownedHealth();
	return MaxPossibleHealth > 0.f ? MaxDownedHealth / MaxPossibleHealth : 0.f;
}

