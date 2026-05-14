// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Player/ProjectERNCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameplayAbilitySpec.h"
#include "InputActionValue.h"
#include "ProjectERN.h"
#include "AbilitySystemComponent.h"
#include "Character/Player/ERNPlayerState.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "GAS/ERNGameplayTags.h"
#include "GAS/Abilities/ERNGA_LightAttack.h"
#include "Input/ERNInputComponent.h"
#include "Net/UnrealNetwork.h"
#include "Shop/Components/ERNShopComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AProjectERNCharacter::AProjectERNCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);

	// Note: For faster iteration times these variables, and many more, can be tweaked in the Character Blueprint
	// instead of recompiling to adjust them
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultSpeed;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->SocketOffset = FVector(0.f, 0.f, 100.f);
	CameraBoom->bUsePawnControlRotation = true;

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	// Create Inventory Component
	InventoryComponent = CreateDefaultSubobject<UERNInventoryComponent>(TEXT("InventoryComponent"));

	// Create Equipment Component
	EquipmentComponent = CreateDefaultSubobject<UERNEquipmentComponent>(TEXT("EquipmentComponent"));

	// Create Shop Component
	ShopComponent = CreateDefaultSubobject<UERNShopComponent>(TEXT("ShopComponent"));

	// GAS 컴포넌트는 부모 클래스(ERNCharacterBase)에서 생성됨

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character)
	// are set in the derived blueprint asset named ThirdPersonCharacter (to avoid direct content references in C++)

	// 기본값 설정
	CharacterType = ECharacterType::Warrior;
}

void AProjectERNCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AProjectERNCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 서버에서만 실행됨
	if (AERNPlayerState* PS = GetPlayerState<AERNPlayerState>())
	{
		// PlayerState의 CharacterType을 캐릭터에 적용 (맵 이동 시 유지)
		// PlayerState가 None이 아니면 그대로 사용, None이면 캐릭터 기본값 사용
		if (PS->CharacterType != ECharacterType::None)
		{
			CharacterType = PS->CharacterType;
		}
		else
		{
			// PlayerState가 None이면 캐릭터의 기본값을 PlayerState에 설정
			PS->CharacterType = CharacterType;
		}
	}

	if (HasAuthority())
	{
		// 재생 GE적용
		ApplyPlayerRegenEffects();
	}

	// GAS 초기화는 부모 클래스에서 처리
}

void AProjectERNCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const bool bIsSprinting =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Sprinting);

	const bool bIsAttacking =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking);

	// Sprint 중에는 LockOn 강제 회전을 끈다.
	// const bool bShouldRotateForLockOn = bIsLockOn && !bIsSprinting && !bIsAttacking;

	if (bIsLockOn && !bIsSprinting && !bIsAttacking)
	{
		if (IsLocallyControlled() && Controller)
		{
			const float ControlYaw = Controller->GetControlRotation().Yaw;
			DesiredLockOnYaw = ControlYaw;

			if (!HasAuthority())
			{
				Server_UpdateLockOnYaw(ControlYaw);
			}
		}

		DesiredActorRotation = GetLockOnDesiredRotation();

		if (HasAuthority() || IsLocallyControlled())
		{
			InterpActorRotation(DeltaSeconds);
		}

		return;
	}

	// 공격/콤보 회전: 로컬 + 서버
	if (bHasPendingActorRotation)
	{
		if (!HasAuthority() && !IsLocallyControlled())
		{
			return;
		}

		InterpActorRotation(DeltaSeconds);
	}
}

void AProjectERNCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UERNInputComponent* InputComp = Cast<UERNInputComponent>(PlayerInputComponent);
	if (!InputComp)
	{
		UE_LOG(LogProjectERN, Error, TEXT("%s failed to find ERNInputComponent."), *GetNameSafe(this));
		return;
	}

	if (!InputConfig)
	{
		UE_LOG(LogProjectERN, Warning, TEXT("%s has no InputConfig."), *GetNameSafe(this));
		return;
	}

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Move,
		ETriggerEvent::Triggered,
		this,
		&AProjectERNCharacter::Move);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Move,
		ETriggerEvent::Completed,
		this,
		&AProjectERNCharacter::MoveEnd);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Look,
		ETriggerEvent::Triggered,
		this,
		&AProjectERNCharacter::Look);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_MouseLook,
		ETriggerEvent::Triggered,
		this,
		&AProjectERNCharacter::Look);

	// Jump
	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Jump,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::DoJumpStart);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Jump,
		ETriggerEvent::Completed,
		this,
		&AProjectERNCharacter::DoJumpEnd);

	// Tap Shift -> Roll
	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Roll,
		ETriggerEvent::Triggered,
		this,
		&AProjectERNCharacter::Roll);

	// Attack
	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_LightAttack,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::LightAttack);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_HeavyAttack,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::HeavyAttack);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_LockOn,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::LockOn);

	InputComp->BindNativeInputAction(
		InputConfig,
		TAG_Input_Sprint,
		ETriggerEvent::Started,
		this,
		&AProjectERNCharacter::ToggleSprint);
}

void AProjectERNCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// 입력 값 갱신(Sprint에 사용)
	CachedMoveInput = MovementVector;

	// 해당 태그가 있으면 움직이지 못함
	const bool bIsAttacking =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking);

	const bool bIsLanding =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Landing);
	
	const bool bIsGetHit =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Stagger);

	if ((bIsAttacking && !bCanMoveWhileAttacking) || bIsLanding || bIsGetHit)
	{
		return;
	}

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
}

void AProjectERNCharacter::MoveEnd()
{
	CachedMoveInput = FVector2D::ZeroVector;
	StopSprint();
}

void AProjectERNCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();

	// route the inputS
	DoLook(LookAxisVector.X, LookAxisVector.Y);
}

void AProjectERNCharacter::Roll()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// 방향 계산
	const FVector RollDirection = GetRollWorldDirection();

	if (HasAuthority())
	{
		// 구르기 방향 결정
		PendingRollDirection = RollDirection;
		
		// 구르기 태그를 가진 어빌리티 실행 시도
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Roll));
	}
	else
	{
		Server_RequestRoll(RollDirection);
	}
}

void AProjectERNCharacter::DoMove(float Right, float Forward)
{
	if (GetController() != nullptr)
	{
		// find out which way is forward
		const FRotator Rotation = GetController()->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

		// get right vector
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// add movement
		AddMovementInput(ForwardDirection, Forward);
		AddMovementInput(RightDirection, Right);
	}
}

void AProjectERNCharacter::DoLook(float Yaw, float Pitch)
{
	if (GetController() != nullptr)
	{
		// add yaw and pitch input to controller
		AddControllerYawInput(Yaw);
		AddControllerPitchInput(Pitch);
	}
}

void AProjectERNCharacter::DoJumpStart()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// 공중 상태라면 벽 점프 실행
	if (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Falling))
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_WallJump));
		return;
	}

	// 점프 실행
	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Jump));
}

void AProjectERNCharacter::DoJumpEnd()
{
	StopJumping();
}

void AProjectERNCharacter::ExecuteJumpLaunch()
{
	Jump();
}

void AProjectERNCharacter::ToggleTemporaryLockOn()
{
	const bool bNewLockOn = !bIsLockOn;
	const FRotator ControlRotation = Controller ? Controller->GetControlRotation() : GetActorRotation();

	ApplyLockOnState(bNewLockOn, ControlRotation);

	if (!HasAuthority())
	{
		Server_SetLockOn(bNewLockOn, ControlRotation);
	}
}

void AProjectERNCharacter::Server_SetLockOn_Implementation(bool bNewLockOn, FRotator TargetRotation)
{
	ApplyLockOnState(bNewLockOn, TargetRotation);
}

void AProjectERNCharacter::ApplyLockOnState(bool bNewLockOn, const FRotator& TargetRotation)
{
	bIsLockOn = bNewLockOn;

	if (bIsLockOn)
	{
		DesiredLockOnYaw = TargetRotation.Yaw;
		DesiredActorRotation = FRotator(0.f, TargetRotation.Yaw, 0.f);
	}

	UpdateRotationMode();
	UpdateMovementSpeed();
}

void AProjectERNCharacter::Server_UpdateLockOnYaw_Implementation(float NewYaw)
{
	DesiredLockOnYaw = NewYaw;
}

void AProjectERNCharacter::OnRep_IsLockOn()
{
	UpdateRotationMode();
	UpdateMovementSpeed();
}

void AProjectERNCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProjectERNCharacter, bIsLockOn);
}

void AProjectERNCharacter::UpdateMovementSpeed()
{
	if (!GetCharacterMovement())
	{
		return;
	}

	const bool bIsSprinting = AbilitySystemComponent && AbilitySystemComponent->HasMatchingGameplayTag(
		TAG_State_Movement_Sprinting);

	float NewSpeed = DefaultSpeed;

	if (bIsSprinting)
	{
		NewSpeed = SprintSpeed;
	}
	else if (bIsLockOn)
	{
		NewSpeed = TargetingSpeed;
	}

	if (AbilitySystemComponent && (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking)))
	{
		NewSpeed = AttackingSpeed;
	}

	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void AProjectERNCharacter::UpdateRotationMode()
{
	if (!GetCharacterMovement())
	{
		return;
	}

	const bool bIsSprinting =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Sprinting);

	const bool bIsAttacking =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking);

	if (bIsAttacking)
	{
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = false;
		return;
	}

	// LockOn상태와 관련 없이 회전이 가능하게 함
	const bool bShouldFaceMovement = !bIsLockOn || bIsSprinting;

	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = bShouldFaceMovement;
}

void AProjectERNCharacter::LightAttack()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	const FRotator TargetRotation = GetAttackDesiredRotation();

	if (CacheActiveLightAttackComboInput(TargetRotation))
	{
		if (!HasAuthority())
		{
			Server_CacheLightAttackComboInput(TargetRotation);
		}

		return;
	}

	SetPendingAttackRotation(TargetRotation);

	if (!HasAuthority())
	{
		Server_SetPendingAttackRotation(TargetRotation);
	}

	// 활성 중인 LightAttack이 없으면 첫 공격을 시작한다.
	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Attack_Light));
}

void AProjectERNCharacter::HeavyAttack()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Attack_Heavy));
	}
}

void AProjectERNCharacter::LockOn()
{
	ToggleTemporaryLockOn();
}

void AProjectERNCharacter::ToggleSprint()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	const bool bIsSprinting = AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Sprinting);

	if (bIsSprinting)
	{
		StopSprint();
		return;
	}

	if (!HasMoveInput())
	{
		return;
	}

	// 전력질주 실행
	AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Sprint));
}

void AProjectERNCharacter::StopSprint()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	FGameplayTagContainer SprintTags;
	SprintTags.AddTag(TAG_Ability_Movement_Sprint);

	AbilitySystemComponent->CancelAbilities(&SprintTags);
}

bool AProjectERNCharacter::HasMoveInput() const
{
	return !CachedMoveInput.IsNearlyZero();
}

FRotator AProjectERNCharacter::GetLockOnDesiredRotation() const
{
	return FRotator(0.f, DesiredLockOnYaw, 0.f);
}

FRotator AProjectERNCharacter::GetAttackDesiredRotation() const
{
	FRotator TargetRotation = GetActorRotation();

	// LockOn상태 일 때
	if (bIsLockOn && Controller)
	{
		// 카메라 방향과 일치하는 방향으로 회전
		const FRotator ControlRotation = Controller->GetControlRotation();
		return FRotator(0.f, ControlRotation.Yaw, 0.f);
	}

	// 입력이 없다면
	if (!CachedMoveInput.IsNearlyZero() && Controller)
	{
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		const FVector DesiredDirection =
			ForwardDirection * CachedMoveInput.Y +
			RightDirection * CachedMoveInput.X;

		if (!DesiredDirection.IsNearlyZero())
		{
			TargetRotation = DesiredDirection.Rotation();
		}
	}

	return FRotator(0.f, TargetRotation.Yaw, 0.f);
}

void AProjectERNCharacter::InterpActorRotation(float DeltaSeconds)
{
	const FRotator CurrentRotation = GetActorRotation();
	const FRotator TargetRotation(0.f, DesiredActorRotation.Yaw, 0.f);

	const FRotator NewRotation = FMath::RInterpTo(
		CurrentRotation,
		TargetRotation,
		DeltaSeconds,
		RotationInterpSpeed);

	SetActorRotation(NewRotation);

	if (bHasPendingActorRotation && NewRotation.Equals(TargetRotation, 1.f))
	{
		bHasPendingActorRotation = false;
	}
}

void AProjectERNCharacter::SetPendingAttackRotation(const FRotator& TargetRotation)
{
	DesiredActorRotation = FRotator(0.f, TargetRotation.Yaw, 0.f);
	bHasPendingActorRotation = true;
}

void AProjectERNCharacter::Server_SetPendingAttackRotation_Implementation(FRotator TargetRotation)
{
	SetPendingAttackRotation(TargetRotation);
}

void AProjectERNCharacter::Server_CacheLightAttackComboInput_Implementation(FRotator TargetRotation)
{
	CacheActiveLightAttackComboInput(TargetRotation);
}

bool AProjectERNCharacter::CacheActiveLightAttackComboInput(const FRotator& TargetRotation)
{
	if (!AbilitySystemComponent)
	{
		return false;
	}

	const FGameplayTagContainer LightAttackTags(TAG_Ability_Attack_Light);

	for (const FGameplayAbilitySpec& AbilitySpec : AbilitySystemComponent->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive() || !AbilitySpec.Ability ||
			!AbilitySpec.Ability->GetAssetTags().HasAll(LightAttackTags))
		{
			continue;
		}

		for (UGameplayAbility* AbilityInstance : AbilitySpec.GetAbilityInstances())
		{
			if (UERNGA_LightAttack* LightAttackAbility = Cast<UERNGA_LightAttack>(AbilityInstance))
			{
				LightAttackAbility->CacheComboInput(TargetRotation);
				return true;
			}
		}
	}

	return false;
}

void AProjectERNCharacter::ApplyPlayerRegenEffects()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	FGameplayEffectContextHandle Context =
		AbilitySystemComponent->MakeEffectContext();

	Context.AddSourceObject(this);

	// 스태미나 재생
	if (StaminaRegenEffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle =
			AbilitySystemComponent->MakeOutgoingSpec(
				StaminaRegenEffectClass,
				1.f,
				Context);

		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
				*SpecHandle.Data.Get());
		}
	}

	// 마나 재생
	if (ManaRegenEffectClass)
	{
		FGameplayEffectSpecHandle SpecHandle =
			AbilitySystemComponent->MakeOutgoingSpec(
				ManaRegenEffectClass,
				1.f,
				Context);

		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(
				*SpecHandle.Data.Get());
		}
	}
}

FVector AProjectERNCharacter::GetRollWorldDirection() const
{
	if (!CachedMoveInput.IsNearlyZero() && Controller)
	{
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector Right = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		return (Forward * CachedMoveInput.Y + Right * CachedMoveInput.X).GetSafeNormal();
	}

	return GetActorForwardVector();
}

void AProjectERNCharacter::Server_RequestRoll_Implementation(FVector_NetQuantizeNormal RollDirection)
{
	PendingRollDirection = RollDirection.GetSafeNormal();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Roll));
	}
}
