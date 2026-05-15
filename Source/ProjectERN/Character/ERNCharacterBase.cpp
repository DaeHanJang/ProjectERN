// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/ERNCharacterBase.h"
#include "AbilitySystemComponent.h"
#include "GAS/ERNAttributeSet.h"
#include "GAS/ERNGameplayTags.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/ERNCutsceneSubsystem.h"

AERNCharacterBase::AERNCharacterBase()
{
	PrimaryActorTick.bCanEverTick = true;

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

void AERNCharacterBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AERNCharacterBase, CutsceneSpeed);
}

void AERNCharacterBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 서버에서만 컷신 Speed 계산
	if (!HasAuthority())
	{
		return;
	}

	// 컷신 재생 중인지 확인
	UERNCutsceneSubsystem* CutsceneSubsystem = nullptr;
	if (UGameInstance* GI = Cast<UGameInstance>(GetWorld()->GetGameInstance()))
	{
		CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>();
	}

	if (!CutsceneSubsystem || !CutsceneSubsystem->IsPlayingCutscene())
	{
		// 컷신 아니면 리셋
		bCutscenePrevLocationValid = false;
		CutsceneSpeed = 0.f;
		return;
	}

	// 컷신 중 Speed 계산
	FVector CurrentLocation = GetActorLocation();

	if (bCutscenePrevLocationValid && DeltaTime > 0.f)
	{
		FVector PositionDelta = CurrentLocation - CutscenePrevLocation;
		float CalculatedSpeed = PositionDelta.Size() / DeltaTime;

		// 텔레포트 감지
		constexpr float MaxReasonableSpeed = 2000.f;
		if (CalculatedSpeed > MaxReasonableSpeed)
		{
			CalculatedSpeed = 0.f;
		}

		// 미세 떨림 방지
		constexpr float SpeedThreshold = 20.f;
		if (CalculatedSpeed < SpeedThreshold)
		{
			CalculatedSpeed = 0.f;
		}

		CutsceneSpeed = CalculatedSpeed;
	}
	else
	{
		bCutscenePrevLocationValid = true;
	}

	CutscenePrevLocation = CurrentLocation;
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

void AERNCharacterBase::SetWeaponAbility(TSubclassOf<UGameplayAbility> NewWeaponAbility)
{
	if (!NewWeaponAbility)
	{
		return;
	}
	
	if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return;
	}
	
	if (WeaponAbilityHandle.IsValid())
	{
		if (FGameplayAbilitySpec* OldSpec = AbilitySystemComponent->FindAbilitySpecFromHandle(WeaponAbilityHandle))
		{
			AbilitySystemComponent->CancelAbilityHandle(WeaponAbilityHandle);
			
			AbilitySystemComponent->ClearAbility(WeaponAbilityHandle);
		}
		
		WeaponAbilityHandle = FGameplayAbilitySpecHandle();
	}
	
	FGameplayAbilitySpec NewSpec(NewWeaponAbility, 1, INDEX_NONE, this);
	WeaponAbilityHandle = AbilitySystemComponent->GiveAbility(NewSpec);
}

void AERNCharacterBase::SetConsumableAbility(TSubclassOf<UGameplayAbility> NewConsumableAbility)
{
	if (!NewConsumableAbility)
	{
		return;
	}
	
	if (!AbilitySystemComponent || !AbilitySystemComponent->IsOwnerActorAuthoritative())
	{
		return;
	}
	
	if (ConsumableAbilityHandle.IsValid())
	{
		if (FGameplayAbilitySpec* OldSpec = AbilitySystemComponent->FindAbilitySpecFromHandle(ConsumableAbilityHandle))
		{
			AbilitySystemComponent->CancelAbilityHandle(ConsumableAbilityHandle);
			
			AbilitySystemComponent->ClearAbility(ConsumableAbilityHandle);
		}
		
		ConsumableAbilityHandle = FGameplayAbilitySpecHandle();
	}
	
	FGameplayAbilitySpec NewSpec(NewConsumableAbility, 1, INDEX_NONE, this);
	ConsumableAbilityHandle = AbilitySystemComponent->GiveAbility(NewSpec);
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

	// 대미지 면역 상태
	if (AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Immunity_Damage))
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

void AERNCharacterBase::PlayCutsceneMontage(UAnimMontage* Montage)
{
	if (!Montage || !GetMesh())
	{
		return;
	}

	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->Montage_Play(Montage);
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
		AbilitySystemComponent->SetLooseGameplayTagCount(TAG_State_Movement_Falling, 1);
	}
	else
	{
		// 공중 상태 태그 Off
		AbilitySystemComponent->SetLooseGameplayTagCount(TAG_State_Movement_Falling, 0);
		// 벽 점프 사용상태 Off
		AbilitySystemComponent->SetLooseGameplayTagCount(TAG_State_Movement_WallJumpUsed, 0);
	}
}
