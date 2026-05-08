// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ERNWorldManagerSettings.generated.h"

/**
 * 
 */
UCLASS(Config = Game, DefaultConfig, meta=(DisplayName="WorldManager Settings"))
class PROJECTERN_API UERNWorldManagerSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="World")
	TSoftObjectPtr<UDataTable> WorldTable;
};
