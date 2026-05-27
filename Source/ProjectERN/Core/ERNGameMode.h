// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ERNGameMode.generated.h"

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AERNGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:

	/** Constructor */
	AERNGameMode();

	// 플레이어의 CharacterType에 따라 적절한 Pawn 클래스 반환
	virtual UClass* GetDefaultPawnClassForController_Implementation(AController* InController) override;

	// OSS 미스매치(incompatible_unique_net_id) 거부만 핀포인트로 무시 - LAN/Steam 토글 양쪽 대응
	virtual void PreLogin(const FString& Options, const FString& Address,
		const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
};
