// Fill out your copyright notice in the Description page of Project Settings.


#include "OverHeadWidget.h"

#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"

void UOverHeadWidget::SetDisplayText(FString TextToDisplay)
{
	if (DisplayText)
	{
		DisplayText->SetText(FText::FromString(TextToDisplay));
	}
}

void UOverHeadWidget::ShowPlayerNetRole(APawn* InPawn)
{
	ENetRole LocalNetRole = InPawn->GetLocalRole();
	ENetRole RemoteNetRole = InPawn->GetRemoteRole();
	FString Role;
	FString RemoteRole;
	switch (LocalNetRole)
	{
	case ENetRole::ROLE_Authority:
		Role = FString(TEXT("Authority"));
		break;
	case ENetRole::ROLE_AutonomousProxy:
		Role = FString(TEXT("AutonomousProxy"));
		break;
	case ENetRole::ROLE_SimulatedProxy:
		Role = FString(TEXT("SimulatedProxy"));
		break;
	case ENetRole::ROLE_None:
		Role = FString(TEXT("None"));
		break;
	}
	switch (RemoteNetRole)
	{
	case ENetRole::ROLE_Authority:
		RemoteRole = FString(TEXT("Authority"));
		break;
	case ENetRole::ROLE_AutonomousProxy:
		RemoteRole = FString(TEXT("AutonomousProxy"));
		break;
	case ENetRole::ROLE_SimulatedProxy:
		RemoteRole = FString(TEXT("SimulatedProxy"));
		break;
	case ENetRole::ROLE_None:
		RemoteRole = FString(TEXT("None"));
		break;
	}
	 
	FString LocalRoleString;
	FString RemoteRoleString;
	APlayerState* PlayerState = InPawn->GetPlayerState();
	if (PlayerState != nullptr)
	{
		FString PlayerName = PlayerState->GetPlayerName();
		LocalRoleString = FString::Printf(TEXT("%s, Local Role: %s"), *PlayerName, *Role);
		RemoteRoleString = FString::Printf(TEXT("%s, Remote Role: %s"), *PlayerName, *RemoteRole);
	}
	else
	{
		LocalRoleString = FString::Printf(TEXT("Local Role: %s"), *Role);
		RemoteRoleString = FString::Printf(TEXT("Remote Role: %s"), *RemoteRole);
	}
	 
	SetDisplayText(LocalRoleString + " : " + RemoteRoleString);
}

void UOverHeadWidget::OnLevelRemovedFromWorld(ULevel* InLevel, UWorld* InWorld)
{
	RemoveFromParent();
	Super::OnLevelRemovedFromWorld(InLevel, InWorld);
}
