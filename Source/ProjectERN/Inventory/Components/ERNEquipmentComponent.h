// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "Components/ActorComponent.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "ERNEquipmentComponent.generated.h"

class AERNConsumableBase;
class UItemManagerSubsystem;
class UERNInventoryComponent;
class AERNWeaponBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipalbeSlotChanged, const FInventoryItemEntry&, Entry);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConsumableSlotChanged, const FInventoryItemEntry&, Entry);

/**
 * ERNEquipmentComponent - 장비 관리 (무기 장착/해제)
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTERN_API UERNEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UERNEquipmentComponent();

	FORCEINLINE const int32 GetCurrentConsumableQuantity() const { return CurrentConsumable.GetQuantity(); }
	FORCEINLINE const FName GetCurrentConsumableItemID() const { return CurrentConsumable.GetItemID(); }
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 초기 무기 장착
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Equipment")
	void Server_EquipWeapon(FName ItemID);

	// 런타임 상태(강화 수치 포함)로 무기 장착 — 스냅샷 복원용
	UFUNCTION(Server, Reliable, Category="Equipment")
	void Server_EquipWeaponFromState(const FItemRuntimeState& WeaponState);

	// 런타임 상태(종류+수량)로 소모품 장착 — 스냅샷 복원용
	UFUNCTION(Server, Reliable, Category="Equipment")
	void Server_EquipConsumableFromState(const FItemRuntimeState& ConsumableState);

	// 초기 무기 제거
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Equipment")
	void Server_UnequipWeapon();
	
	// 아이템 장착
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Equipment")
	void Server_EquipItem(const int32 SlotIndex);
	
	// 아이템 버리기 (소모품 전용)
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Equipment")
	void Server_UnequipItem(const int32 Quantity);
	
	// 장착한 소모품 개수 감소
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment")
	void Server_UseCurrentConsumableQuantity();
	
private:
	UItemManagerSubsystem* GetItemManagerSubsystem() const;
	UERNInventoryComponent* GetInventoryComponent() const;

	// RepNotify 함수
	// 장비 슬롯
	UFUNCTION()
	void OnRep_EquipableSlot();
	// 소모품 슬롯
	UFUNCTION()
	void OnRep_ConsumableSlot();

public:
	// 장착 갱신 이벤트
	// 무기
	FOnEquipalbeSlotChanged OnEquipmentSlotChanged;
	// 소모품
	FOnConsumableSlotChanged OnConsumableSlotChanged;

	// 현재 장착된 무기
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
	AERNWeaponBase* CurrentWeapon = nullptr;
	
	// 무기 슬롯
	UPROPERTY(ReplicatedUsing="OnRep_EquipableSlot", BlueprintReadOnly, Category = "Equipment")
	FInventoryItemEntry EquipableSlot;
	
	// 현재 장착된 소모품
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
	FItemRuntimeState CurrentConsumable;
	
	// 소모품 슬롯
	UPROPERTY(ReplicatedUsing="OnRep_ConsumableSlot", BlueprintReadOnly, Category = "Equipment")
	FInventoryItemEntry ConsumableSlot;
	
};
