// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"
#include "DumpStaticMeshCollisionStateCommandlet.generated.h"

UCLASS()
class PROJECTERN_API UDumpStaticMeshCollisionStateCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UDumpStaticMeshCollisionStateCommandlet();

	virtual int32 Main(const FString& Params) override;
};
