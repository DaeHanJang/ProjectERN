// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Player/ProjectERNCharacter.h"
#include "Engine/LocalPlayer.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
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
#include "Shop/Components/ERNShopComponent.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

AProjectERNCharacter::AProjectERNCharacter()
{
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

	// GAS 초기화는 부모 클래스에서 처리
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
}

void AProjectERNCharacter::Move(const FInputActionValue& Value)
{
	// 해당 태그가 있으면 움직이지 못함
	/*
	if (AbilitySystemComponent &&
		(AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking) ||
			AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Landing)))
	{
		return;
	}
	*/
	const bool bIsAttacking =
	AbilitySystemComponent &&
	AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking);

	const bool bIsLanding =
		AbilitySystemComponent &&
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Landing);

	if ((bIsAttacking && !bCanMoveWhileAttacking) || bIsLanding)
	{
		return;
	}

	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	// route the input
	DoMove(MovementVector.X, MovementVector.Y);
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
	if (AbilitySystemComponent)
	{
		// 구르기 태그를 가진 어빌리티 실행 시도
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Roll));
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
	if (AbilitySystemComponent)
	{
		// 점프 태그를 가진 어빌리티 실행 시도
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Movement_Jump));
	}
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
	bIsLockOn = !bIsLockOn;

	if (!Controller)
	{
		UpdateMovementSpeed();
		return;
	}

	if (bIsLockOn)
	{
		// 카메라/컨트롤러가 바라보는 Yaw로 캐릭터 방향을 맞춘다.
		const FRotator ControlRotation = Controller->GetControlRotation();
		const FRotator TargetYawRotation(0.f, ControlRotation.Yaw, 0.f);

		SetActorRotation(TargetYawRotation);

		// 락온 중에는 컨트롤러 Yaw가 캐릭터 회전을 직접 제어하게 한다.
		bUseControllerRotationYaw = true;

		// 이동 방향으로 자동 회전하는 기능은 꺼야 카메라 방향 유지가 쉽다.
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else
	{
		// 기존 이동 방식으로 복귀.
		bUseControllerRotationYaw = false;
		GetCharacterMovement()->bOrientRotationToMovement = true;
	}
	
	UpdateMovementSpeed();
}

void AProjectERNCharacter::UpdateMovementSpeed()
{
	if (!GetCharacterMovement())
	{
		return;
	}

	float NewSpeed = DefaultSpeed;

	if (bIsLockOn)
	{
		NewSpeed = TargetingSpeed;
	}

	if (AbilitySystemComponent && (AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking)))
	{
		NewSpeed = AttackingSpeed;
	}
	
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void AProjectERNCharacter::LightAttack()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// 이미 LightAttack 어빌리티가 활성화되어 있으면
	// 새로 활성화하지 않고 다음 콤보 입력만 전달한다.
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
				LightAttackAbility->CacheComboInput();
				return;
			}
		}
	}

	// 활성 중인 LightAttack이 없으면 첫 공격을 시작한다.
	AbilitySystemComponent->TryActivateAbilitiesByTag(
		FGameplayTagContainer(TAG_Ability_Attack_Light));
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
