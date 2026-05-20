// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "ERNWeaponBase.generated.h"

class UGameplayAbility;
class UEquipableItemDataAsset;
/**
 * ERNWeaponBase - 무기 베이스 클래스
 */
UCLASS(Abstract)
class PROJECTERN_API AERNWeaponBase : public AActor
{
	GENERATED_BODY()

public:
	AERNWeaponBase();
	
	// Getter/Setter
	FORCEINLINE const FItemRuntimeState& GetItemRuntimeState() const { return ItemRuntimeState; }
	FORCEINLINE void SetItemRuntimeState(const FItemRuntimeState& NewItemRuntimeState) { ItemRuntimeState = NewItemRuntimeState; }
	
	// Initialization
	void Init(const FItemRuntimeState& InItemRuntimeState, const UEquipableItemDataAsset* DA);

protected:
	virtual void BeginPlay() override;

public:
	// Static Mesh와 Skeletal Mesh를 모두 사용하기 위해 SceneRoot를 하나 설정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USceneComponent* SceneRoot;
	
	// 무기 메시(Static Mesh)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	UStaticMeshComponent* WeaponMesh;
	
	// 무기 메시(Skeletal Mesh)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon")
	USkeletalMeshComponent* SkeletalWeaponMesh;

	// 런타임 상태
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon States")
	FItemRuntimeState ItemRuntimeState;
	
	// 무기 공격력
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float LightAttackDamage = 10.0f;

	// 무기 강인도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	float LightAttackStaggerPower = 10.f;
	
	// 무기 스킬
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Stats")
	TSubclassOf<UGameplayAbility> WeaponSkill = nullptr;

	// 무기 전용 애니메이션 몽타주
	
	/* LightAttack은 일반 공격으로 GA에서 몽타주 받아서 처리.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Animations")
	UAnimMontage* LightAttackMontage;
	*/

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon Animations")
	TObjectPtr<UAnimMontage> HeavyAttackMontage;
};
