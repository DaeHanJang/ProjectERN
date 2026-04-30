// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/ERNCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GAS/ERNAttributeSet.h"
#include "GAS/ERNGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"

AERNCharacterBase::AERNCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// GAS 컴포넌트 생성
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UERNAttributeSet>(TEXT("AttributeSet"));
}

void AERNCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void AERNCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystemActorInfo();

	// 서버에서만 어빌리티 부여
	if (HasAuthority())
	{
		GiveDefaultAbilities();
	}
}

void AERNCharacterBase::OnRep_Controller()
{
	Super::OnRep_Controller();
	InitializeAbilitySystemActorInfo();
}

void AERNCharacterBase::InitializeAbilitySystemActorInfo()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);
}

void AERNCharacterBase::GiveDefaultAbilities()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (AbilityClass)
		{
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
		}
	}
}

UAbilitySystemComponent* AERNCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

float AERNCharacterBase::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (bIsDead)
	{
		return 0.0f;
	}

	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (AttributeSet && ActualDamage > 0.0f)
	{
		// AttributeSet의 Health 감소
		const float NewHealth = FMath::Max(0.0f, AttributeSet->GetHealth() - ActualDamage);
		AttributeSet->SetHealth(NewHealth);

		UE_LOG(LogTemp, Log, TEXT("%s took %.2f damage. Health: %.2f"), *GetName(), ActualDamage, NewHealth);

		// 사망 체크
		if (NewHealth <= 0.0f)
		{
			OnDeath();
		}
	}

	return ActualDamage;
}

void AERNCharacterBase::TryApplyStagger(float IncomingStaggerPower)
{
	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	// 슈퍼아머 또는 무적 프레임이면 경직 무시
	if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_SuperArmor) ||
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_StaggerImmune))
	{
		return;
	}

	// StaggerPower가 저항력 이상일 때만 경직 적용
	if (IncomingStaggerPower < AttributeSet->GetStaggerResistance())
	{
		return;
	}

	// GE_Stagger 적용
	if (StaggerEffect)
	{
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		AbilitySystemComponent->ApplyGameplayEffectToSelf(
			StaggerEffect->GetDefaultObject<UGameplayEffect>(), 1.f, Context);
	}

	// 히트리액션 몽타주 재생 (Multicast로 모든 클라이언트에 동기화)
	if (HitReactionMontage)
	{
		Multicast_PlayHitReaction();
	}
}

void AERNCharacterBase::Multicast_PlayHitReaction_Implementation()
{
	if (!HitReactionMontage || !GetMesh()) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance)
	{
		AnimInstance->Montage_Play(HitReactionMontage);
	}
}

void AERNCharacterBase::OnDeath()
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;

	UE_LOG(LogTemp, Log, TEXT("%s has died."), *GetName());

	// 이동 비활성화
	GetCharacterMovement()->DisableMovement();

	// 충돌 비활성화
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 자식 클래스에서 추가 사망 처리 구현 (애니메이션, 이펙트 등)
}

void AERNCharacterBase::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);
	
	if (!AbilitySystemComponent)
	{
		return;
	}
	
	// 공중 상태 태그 On/Off
	if (GetCharacterMovement()->MovementMode == MOVE_Falling)
	{
		AbilitySystemComponent->AddLooseGameplayTag(TAG_State_Movement_Falling);
	}
	else
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(TAG_State_Movement_Falling);	
	}
}