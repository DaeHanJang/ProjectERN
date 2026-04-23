// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Enemy/ERNBossCharacter.h"
#include "Character/Enemy/AI/ERNBossAIController.h"
#include "GAS/ERNAttributeSet.h"
#include "GAS/ERNGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Net/UnrealNetwork.h"

AERNBossCharacter::AERNBossCharacter()
{
	// 보스용 AI Controller 클래스 설정
	AIControllerClass = AERNBossAIController::StaticClass();

	// 보스 기본 설정
	InitialStaggerResistance = 50.f;  // 보스는 경직 저항 높음
}

void AERNBossCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 첫 페이즈 설정
	if (Phases.Num() > 0)
	{
		CurrentPhaseIndex = 0;
	}
}

void AERNBossCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AERNBossCharacter, CurrentPhaseIndex);
	DOREPLIFETIME(AERNBossCharacter, bIsTransitioningPhase);
}

float AERNBossCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 페이즈 전환 중에는 데미지 무시 (선택적)
	if (bIsTransitioningPhase)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	// 페이즈 전환 체크
	if (ActualDamage > 0.0f && HasAuthority())
	{
		CheckPhaseTransition();
	}

	return ActualDamage;
}

int32 AERNBossCharacter::GetPhaseIndexForCurrentHealth() const
{
	if (!AttributeSet || Phases.Num() == 0)
	{
		return 0;
	}

	const float CurrentHealthRatio = AttributeSet->GetHealth() / AttributeSet->GetMaxHealth();

	// 체력 비율에 맞는 페이즈 찾기 (내림차순으로 정렬되어 있다고 가정)
	for (int32 i = Phases.Num() - 1; i >= 0; --i)
	{
		if (CurrentHealthRatio <= Phases[i].HealthThreshold)
		{
			return i;
		}
	}

	return 0;
}

void AERNBossCharacter::CheckPhaseTransition()
{
	const int32 NewPhaseIndex = GetPhaseIndexForCurrentHealth();

	if (NewPhaseIndex > CurrentPhaseIndex)
	{
		TransitionToPhase(NewPhaseIndex);
	}
}

void AERNBossCharacter::TransitionToPhase(int32 NewPhaseIndex)
{
	if (NewPhaseIndex < 0 || NewPhaseIndex >= Phases.Num() || NewPhaseIndex == CurrentPhaseIndex)
	{
		return;
	}

	bIsTransitioningPhase = true;
	CurrentPhaseIndex = NewPhaseIndex;

	const FBossPhaseInfo& NewPhase = Phases[CurrentPhaseIndex];

	// 페이즈 전환 중 슈퍼아머 적용
	if (NewPhase.bSuperArmorDuringTransition)
	{
		ApplySuperArmor();
	}

	// 페이즈 전환 몽타주 재생
	if (NewPhase.PhaseTransitionMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
		AnimInstance->Montage_Play(NewPhase.PhaseTransitionMontage);

		// 몽타주 종료 델리게이트
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AERNBossCharacter::OnPhaseTransitionMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, NewPhase.PhaseTransitionMontage);
	}
	else
	{
		// 몽타주 없으면 바로 완료 처리
		OnPhaseTransitionMontageEnded(nullptr, false);
	}

	// 새 비헤이비어 트리로 전환
	if (AERNBossAIController* BossAIC = Cast<AERNBossAIController>(GetController()))
	{
		BossAIC->SwitchBehaviorTree(NewPhase.PhaseBehaviorTree);
	}

	UE_LOG(LogTemp, Log, TEXT("[%s] Transitioned to Phase %d"), *GetName(), CurrentPhaseIndex);
}

void AERNBossCharacter::OnPhaseTransitionMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsTransitioningPhase = false;

	// 슈퍼아머 해제
	RemoveSuperArmor();
}

void AERNBossCharacter::PlayIntro()
{
	if (IntroMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		// 인트로 중 슈퍼아머
		ApplySuperArmor();

		GetMesh()->GetAnimInstance()->Montage_Play(IntroMontage);

		// 인트로 종료 후 슈퍼아머 해제
		FOnMontageEnded EndDelegate;
		EndDelegate.BindUObject(this, &AERNBossCharacter::OnPhaseTransitionMontageEnded);
		GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(EndDelegate, IntroMontage);
	}
}

void AERNBossCharacter::OnDeath()
{
	// 사망 몽타주 재생
	if (DeathMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Play(DeathMontage);
	}

	// 부모 사망 처리 (딜레이 후 Destroy 등)
	Super::OnDeath();
}

void AERNBossCharacter::ApplySuperArmor()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(TAG_State_SuperArmor);
	}
}

void AERNBossCharacter::RemoveSuperArmor()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(TAG_State_SuperArmor);
	}
}
