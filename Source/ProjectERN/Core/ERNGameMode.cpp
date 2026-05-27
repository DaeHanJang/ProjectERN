// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ERNGameMode.h"
#include "Character/Player/ERNPlayerState.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GameFramework/PlayerController.h"
#include "Core/ERNGameInstance.h"

AERNGameMode::AERNGameMode()
{
}

UClass* AERNGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	// PlayerController에서 PlayerState 가져오기
	if (APlayerController* PC = Cast<APlayerController>(InController))
	{
		if (AERNPlayerState* PS = PC->GetPlayerState<AERNPlayerState>())
		{
			// PlayerState의 CharacterType이 None인 경우, GameInstance에서 복원 시도
			if (PS->CharacterType == ECharacterType::None)
			{
				if (UERNGameInstance* GI = Cast<UERNGameInstance>(GetGameInstance()))
				{
					ECharacterType SavedType = GI->GetPlayerCharacterType();
					if (SavedType != ECharacterType::None)
					{
						PS->CharacterType = SavedType;
						UE_LOG(LogTemp, Warning, TEXT("[GetDefaultPawnClassForController] Restored CharacterType from GameInstance: %d"), static_cast<int32>(SavedType));
					}
				}
			}

			UE_LOG(LogTemp, Warning, TEXT("[GetDefaultPawnClassForController] Player %s has CharacterType: %d"), *PS->PlayerNickname, static_cast<int32>(PS->CharacterType));

			// PlayerState의 CharacterType에 따라 적절한 캐릭터 클래스 반환
			UClass* SelectedClass = nullptr;

			switch (PS->CharacterType)
			{
			case ECharacterType::Warrior:
				SelectedClass = PS->WarriorClass.Get();
				if (SelectedClass)
				{
					UE_LOG(LogTemp, Log, TEXT("Spawning Warrior for %s"), *PS->PlayerNickname);
					return SelectedClass;
				}
				break;
			case ECharacterType::Mage:
				SelectedClass = PS->MageClass.Get();
				if (SelectedClass)
				{
					UE_LOG(LogTemp, Log, TEXT("Spawning Mage for %s"), *PS->PlayerNickname);
					return SelectedClass;
				}
				break;
			case ECharacterType::Support:
				SelectedClass = PS->SupporterClass.Get();
				if (SelectedClass)
				{
					UE_LOG(LogTemp, Log, TEXT("Spawning Supporter for %s"), *PS->PlayerNickname);
					return SelectedClass;
				}
				break;
			case ECharacterType::None:
				// 캐릭터를 선택하지 않은 경우 기본값(Warrior) 설정
				UE_LOG(LogTemp, Warning, TEXT("CharacterType is None for %s, setting to Warrior"), *PS->PlayerNickname);
				PS->CharacterType = ECharacterType::Warrior;
				SelectedClass = PS->WarriorClass.Get();
				if (SelectedClass)
				{
					return SelectedClass;
				}
				break;
			default:
				UE_LOG(LogTemp, Warning, TEXT("CharacterType is invalid for %s, using default"), *PS->PlayerNickname);
				break;
			}
		}
	}

	// 기본 Pawn 클래스 반환 (블루프린트에서 설정된 DefaultPawnClass)
	return Super::GetDefaultPawnClassForController_Implementation(InController);
}

void AERNGameMode::PreLogin(const FString& Options, const FString& Address,
	const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	// LAN/Steam 토글 시 엔진 기본 OSS와 클라 UniqueId 타입이 미스매치되면
	// 엔진 기본 PreLogin이 "incompatible_unique_net_id"로 거부함.
	// 사적 LAN/친선 Steam 환경이므로 이 검증만 핀포인트로 무시.
	// 정원 초과 등 다른 거부 사유는 그대로 작동.
	if (ErrorMessage == TEXT("incompatible_unique_net_id"))
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[ERNGameMode] OSS UniqueId 타입 미스매치 무시 - LAN/Steam 토글 대응"));
		ErrorMessage.Empty();
	}
}
