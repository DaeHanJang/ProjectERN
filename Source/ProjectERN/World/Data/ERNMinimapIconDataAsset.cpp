// Fill out your copyright notice in the Description page of Project Settings.


#include "ERNMinimapIconDataAsset.h"

UTexture2D* UERNMinimapIconDataAsset::FindIconTexture(const EERNMinimapIconType Type) const
{
	if (const TObjectPtr<UTexture2D>* Found = IconTextures.Find(Type))
	{
		return Found->Get();
	}
	
	return nullptr;
}
