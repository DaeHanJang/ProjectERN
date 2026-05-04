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
#include "InputActionValue.h"
#include "ProjectERN.h"
#include "AbilitySystemComponent.h"
#include "Character/Player/ERNPlayerState.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "GAS/ERNGameplayTags.h"
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
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
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
}

void AProjectERNCharacter::Move(const FInputActionValue& Value)
{
	// 해당 태그가 있으면 움직이지 못함
	if (AbilitySystemComponent &&
		(AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Combat_Attacking) ||
		AbilitySystemComponent->HasMatchingGameplayTag(TAG_State_Movement_Landing)))
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

void AProjectERNCharacter::Roll(const FInputActionValue& Value)
{
	if (AbilitySystemComponent)
	{
		// 구르기 태그를 가진 어빌리티 실행 시도
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Move_Roll));
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
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Move_Jump));
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

void AProjectERNCharacter::LightAttack(const FInputActionValue& Value)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Attack_Light));
	}
}

void AProjectERNCharacter::HeavyAttack(const FInputActionValue& Value)
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(TAG_Ability_Attack_Heavy));
	}
}
