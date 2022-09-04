#pragma once

UENUM(BlueprintType)
enum class ETurningInSpaceState: uint8
{
	ETIS_Left UMETA(DisplayName="Turning Left"),
	ETIS_Right UMETA(DisplayName="Turning Right"),
	ETIS_NotTurning UMETA(DisplayName="Not Turning"),

	ETIS_MAX UMETA(DisplayName="DefaultMAX")
};
