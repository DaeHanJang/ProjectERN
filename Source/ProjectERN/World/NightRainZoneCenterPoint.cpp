// Fill out your copyright notice in the Description page of Project Settings.


#include "World/NightRainZoneCenterPoint.h"

#include "MiddleBossSpawner.h"

void ANightRainZoneCenterPoint::HandleZoneShrinkFinished()
{
	if (MiddleBossSpawner == nullptr)
	{
		return;
	}
	
	MiddleBossSpawner->SpawnMob();
}
