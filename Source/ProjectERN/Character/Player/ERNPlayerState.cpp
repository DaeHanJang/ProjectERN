// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Player/ERNPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "Core/ERNGameState.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "Core/ERNGameInstance.h"
#include "Inventory/Components/ERNInventoryComponent.h"

AERNPlayerState::AERNPlayerState()
{
	// 기본값 - 로비에서 설정됨
		CharacterType = ECharacterType::None;
	PlayerNickname = TEXT("Player");
	bIsReady = false;
}

// Replication 명단
void AERNPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AERNPlayerState, CharacterType);
	DOREPLIFETIME(AERNPlayerState, PlayerNickname);
	DOREPLIFETIME(AERNPlayerState, bIsReady);
}

void AERNPlayerState::SetNickname(const FString& Nickname)
{
	Server_SetNickname(Nickname);
}

void AERNPlayerState::Server_SetNickname_Implementation(const FString& Nickname)
{
	if (!Nickname.IsEmpty() && Nickname.Len() <= 20)
	{
		PlayerNickname = Nickname;
		UE_LOG(LogTemp, Log, TEXT("Player nickname set to: %s"), *PlayerNickname);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid nickname: %s"), *Nickname);
	}
}

void AERNPlayerState::Server_SetReady_Implementation(bool bReady)
{
	bIsReady = bReady;
	UE_LOG(LogTemp, Log, TEXT("Player %s ready state: %s"), *PlayerNickname, bIsReady ? TEXT("Ready") : TEXT("Not Ready"));

	// 서버에서도 델리게이트 브로드캐스트 (클라이언트는 OnRep_IsReady에서 자동 호출)
	OnReadyStateChanged.Broadcast(bIsReady);

	// GameState에 준비 상태 체크 요청
	if (AERNGameState* GameState = GetWorld()->GetGameState<AERNGameState>())
	{
		GameState->CheckAllPlayersReady();
	}
}

void AERNPlayerState::OnRep_IsReady()
{
	// 준비 상태가 리플리케이트되면 델리게이트 브로드캐스트
	UE_LOG(LogTemp, Log, TEXT("OnRep_IsReady: Player %s is now %s"), *PlayerNickname, bIsReady ? TEXT("Ready") : TEXT("Not Ready"));
	OnReadyStateChanged.Broadcast(bIsReady);
}

void AERNPlayerState::Server_ChangeCharacterClass_Implementation(ECharacterType NewClass)
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());
	if (!PC)
	{
		UE_LOG(LogTemp, Error, TEXT("Server_ChangeCharacterClass: PlayerController is null"));
		return;
	}

	// 현재 캐릭터 정보 저장
	APawn* OldPawn = PC->GetPawn();
	FVector Location = OldPawn ? OldPawn->GetActorLocation() : FVector::ZeroVector;
	FRotator Rotation = OldPawn ? OldPawn->GetActorRotation() : FRotator::ZeroRotator;

	// 새 캐릭터 클래스 선택
	TSubclassOf<AProjectERNCharacter> NewCharacterClass = nullptr;
	switch (NewClass)
	{
	case ECharacterType::Warrior:
		NewCharacterClass = WarriorClass;
		break;
	case ECharacterType::Mage:
		NewCharacterClass = MageClass;
		break;
	case ECharacterType::Support:
		NewCharacterClass = SupporterClass;
		break;
	}

	if (!NewCharacterClass)
	{
		UE_LOG(LogTemp, Error, TEXT("Character class not set for type %d"), static_cast<int32>(NewClass));
		return;
	}

	// 새 캐릭터 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = PC;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AProjectERNCharacter* NewCharacter = GetWorld()->SpawnActor<AProjectERNCharacter>(
		NewCharacterClass, Location, Rotation, SpawnParams);
	if (NewCharacter)
	{
		// Possess
		PC->Possess(NewCharacter);

		// CharacterType 업데이트
		CharacterType = NewClass;

		UE_LOG(LogTemp, Warning, TEXT("[Server_ChangeCharacterClass] Player %s changed CharacterType to %d"), *PlayerNickname, static_cast<int32>(NewClass));
	}
	
	if (AProjectERNCharacter* OldCharacter = Cast<AProjectERNCharacter>(OldPawn))
	{
		if (UERNInventoryComponent* OldInventory = OldCharacter->GetInventoryComponent())
		{
			if (UERNInventoryComponent* NewInventory = NewCharacter->GetInventoryComponent())
			{
				NewInventory->CopyInventoryFrom(OldInventory);
			}
		}
	}
	
	// 기존 캐릭터 파괴 (장착된 무기도 함께 정리)
	if (OldPawn)
	{
		// Equipment 컴포넌트가 있다면 무기 먼저 해제
		if (AProjectERNCharacter* OldCharacter = Cast<AProjectERNCharacter>(OldPawn))
		{
			if (UERNEquipmentComponent* EquipComp = OldCharacter->FindComponentByClass<UERNEquipmentComponent>())
			{
				// 장착된 무기가 있다면 먼저 해제
				EquipComp->Server_UnequipWeapon();
			}
		}
		OldPawn->Destroy();
	}
}

void AERNPlayerState::Server_SetCharacterType_Implementation(ECharacterType NewType)
{
	CharacterType = NewType;
	UE_LOG(LogTemp, Log, TEXT("[Server_SetCharacterType] Player %s CharacterType set to %d"), *PlayerNickname, static_cast<int32>(NewType));
}

// AttributeSet에서 값 가져오기
float AERNPlayerState::GetCurrentHealth() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetHealth();
			}
		}
	}
	return 0.f;
}

float AERNPlayerState::GetMaxHealth() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetMaxHealth();
			}
		}
	}
	return 100.f;
}

float AERNPlayerState::GetCurrentMana() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetMana();
			}
		}
	}
	return 0.f;
}

float AERNPlayerState::GetMaxMana() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetMaxMana();
			}
		}
	}
	return 100.f;
}

float AERNPlayerState::GetCurrentStamina() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetStamina();
			}
		}
	}
	return 0.f;
}

float AERNPlayerState::GetMaxStamina() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetMaxStamina();
			}
		}
	}
	return 100.f;
}

float AERNPlayerState::GetCurrentShield() const
{
	if (APawn* Pawn = GetPawn())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
		{
			if (const UERNAttributeSet* AS = Character->GetAttributeSet())
			{
				return AS->GetShield();
			}
		}
	}
	return 0.f;
}
