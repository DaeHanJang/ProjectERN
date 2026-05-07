// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/ERNCharacterBase.h"
#include "Logging/LogMacros.h"
#include "ProjectERNCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
enum class ECharacterType : uint8;
class UERNInventoryComponent;
class UERNEquipmentComponent;
class UERNShopComponent;
class UERNInputConfig;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera
 */
UCLASS(abstract)
class AProjectERNCharacter : public AERNCharacterBase
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FollowCamera;

	/** Inventory Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UERNInventoryComponent* InventoryComponent;

	/** Equipment Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UERNEquipmentComponent* EquipmentComponent;

	/** Shop Component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UERNShopComponent* ShopComponent;

protected:

	/** Character Type - 블루프린트에서 설정 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Character")
	ECharacterType CharacterType;

	/** Called when character is possessed by a controller */
	virtual void PossessedBy(AController* NewController) override;
	
	// 태그 기반 입력을 위한 InputConfig 부여
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Input")
	TObjectPtr<UERNInputConfig> InputConfig;
	
public:
	/** Constructor */
	AProjectERNCharacter();

protected:

	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	
	/** Called for movement input */
	void Move(const FInputActionValue& Value);
	
	/** Called for movement input end*/
	void MoveEnd();

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for roll input */
	void Roll();
	
	/** Called for light attack input */
	void LightAttack();

	/** Called for heavy attack input */
	void HeavyAttack();
	
	/** Called for lock on input */
	void LockOn();
	
	/** Called for sprint on input */
	void ToggleSprint();

public:
	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="ERN|Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="ERN|Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="ERN|Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="ERN|Input")
	virtual void DoJumpEnd();
	
	UFUNCTION(BlueprintCallable, Category="ERN|Action")
	void ExecuteJumpLaunch();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }

	/** Returns InventoryComponent **/
	FORCEINLINE class UERNInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** Returns EquipmentComponent **/
	FORCEINLINE class UERNEquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }

	/** Returns ShopComponent **/
	FORCEINLINE class UERNShopComponent* GetShopComponent() const { return ShopComponent; }
	
	// ************** 임시 락온 기능 구현 **************
public:
	UFUNCTION(BlueprintCallable, Category="ERN|LockOn")
	void ToggleTemporaryLockOn();

	bool IsLockOn() const { return bIsLockOn; }
	
protected:
	UPROPERTY(BlueprintReadOnly, Category="ERN|LockOn")
	bool bIsLockOn = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|LockOn")
	bool bUseCameraYawOnLockOn = true;
	// ************** 임시 락온 기능 구현 **************
	
protected:
	// 공격 중 움직일 수 있게 하기 위함
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Combat")
	bool bCanMoveWhileAttacking = false;
	
	// 상태 별 속도
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float DefaultSpeed = 600;	// 기본 속도
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float TargetingSpeed = 300;	// 타겟팅 중 속도
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float TargetingRunSpeed = 500;	// 타겟팅 중 달리기 속도
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float AttackingSpeed = 200;	// 공격 중 속도
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Movement")
	float SprintSpeed = 1000;	// 전력질주 속도
	
public:
	// 움직임 속도 변화 함수
	UFUNCTION(BlueprintCallable, Category="ERN|Movement")
	void UpdateMovementSpeed();
	
protected:
	// Sprint 관련 변수/함수
	FVector2D CachedMoveInput = FVector2D::ZeroVector;
	
	// Sprint 멈추기 함수
	void StopSprint();
	
	// 움직임 입력 여부 확인(Sprint 비활성 확인용)
	bool HasMoveInput() const;
};
