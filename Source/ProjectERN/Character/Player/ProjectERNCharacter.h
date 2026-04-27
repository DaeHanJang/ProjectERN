// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/ERNCharacterBase.h"
#include "Logging/LogMacros.h"
#include "GameplayTagContainer.h"
#include "ProjectERNCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;
enum class ECharacterType : uint8;
class UERNInventoryComponent;
class UERNEquipmentComponent;
class UERNShopComponent;

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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Character")
	ECharacterType CharacterType;

	/** Called when character is possessed by a controller */
	virtual void PossessedBy(AController* NewController) override;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;

	/** Mouse Look Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MouseLookAction;

	/** Light Attack Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LightAttackAction;

	/** Heavy Attack Input Action */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* HeavyAttackAction;

public:

	/** Constructor */
	AProjectERNCharacter();

protected:

	/** Called when the game starts or when spawned */
	virtual void BeginPlay() override;

	/** Initialize input action bindings */
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called for light attack input */
	void LightAttack(const FInputActionValue& Value);

	/** Called for heavy attack input */
	void HeavyAttack(const FInputActionValue& Value);

public:
	/** Handles move inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	/** Handles look inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoLook(float Yaw, float Pitch);

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	/** Handles jump pressed inputs from either controls or UI interfaces */
	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();
	
	UFUNCTION(BlueprintCallable, Category="Action")
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
};
