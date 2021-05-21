// Fill out your copyright notice in the Description page of Project Settings.


#include "SomeRPGSaveGame.h"

USomeRPGSaveGame::USomeRPGSaveGame()
{
	PlayerName = TEXT("Default");
	PlayerIndex = 0;

	CharacterStats.WeaponName = TEXT("");
	CharacterStats.LevelName = TEXT("");
}