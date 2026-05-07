// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "Components/ActorComponent.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "ERNEquipmentComponent.generated.h"

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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 무기 장착
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment")
	void Server_EquipWeapon(TSubclassOf<AERNWeaponBase> WeaponClass);

	// 무기 해제
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment")
	void Server_UnequipWeapon();
	
	// 아이템 장착
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment")
	void Server_EquipItem(const int32 SlotIndex);
	
	// TODO: 소모품 클래스를 만든 후 작성
	// 아이템 제거 (소모품 전용)
	UFUNCTION(Server,Reliable, BlueprintCallable, Category = "Equipment")
	void Server_UnequipItem();
	
protected:
	virtual void BeginPlay() override;
	
private:
	UItemManagerSubsystem* GetItemManagerSubsystem() const;
	UERNInventoryComponent* GetInventoryComponent() const;

	UFUNCTION()
	void OnRep_EquipableSlot();
	UFUNCTION()
	void OnRep_ConsumableSlot();

public:
	// 장착 갱신 이벤트
	FOnEquipalbeSlotChanged OnEquipmentSlotChanged;
	FOnConsumableSlotChanged OnConsumableSlotChanged;
	
	// 현재 장착된 무기
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
	AERNWeaponBase* CurrentWeapon;
	
	// 무기 슬롯
	UPROPERTY(ReplicatedUsing="OnRep_EquipableSlot", BlueprintReadOnly, Category = "Equipment")
	FInventoryItemEntry EquipableSlot;
	
	// TODO: 현재 장착된 소모품
	// UPROPERTY(Replicated, BlueprintReadOnly, Category = "Equipment")
	// AERNConsumableBase* CurrentConsumable;
	
	// 소모품 슬롯
	UPROPERTY(ReplicatedUsing="OnRep_ConsumableSlot", BlueprintReadOnly, Category = "Equipment")
	FInventoryItemEntry ConsumableSlot;
	
};
