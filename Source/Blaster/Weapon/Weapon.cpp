// Fill out your copyright notice in the Description page of Project Settings.


#include "Weapon.h"

#include "Casing.h"
#include "Blaster/Character/BlasterCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/SkeletalMeshSocket.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;

	WeaponMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("WeaponMesh"));
	SetRootComponent(WeaponMesh);

	WeaponMesh->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	WeaponMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	AreaSphere = CreateDefaultSubobject<USphereComponent>(TEXT("AreaSphere"));
	AreaSphere->SetupAttachment(WeaponMesh);
	AreaSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	PickupWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("PickupWidget"));
	PickupWidget->SetupAttachment(WeaponMesh);
}

void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWeapon, WeaponState);
}

void AWeapon::BeginPlay()
{
	Super::BeginPlay();

	// ENetRole LocalNetRole = GetLocalRole();
	// switch (LocalNetRole)
	// {
	// case ENetRole::ROLE_Authority:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("AWeapon::BeginPlay"));
	// 	break;
	// case ENetRole::ROLE_AutonomousProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Green, TEXT("AWeapon::BeginPlay"));
	// 	break;
	// case ENetRole::ROLE_SimulatedProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Yellow, TEXT("AWeapon::BeginPlay"));
	// 	break;
	// case ENetRole::ROLE_None:
	// 	GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Blue, TEXT("AWeapon::BeginPlay"));
	// 	break;
	// }
	if (HasAuthority())
	{
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		AreaSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		AreaSphere->OnComponentBeginOverlap.AddDynamic(this, &AWeapon::OnSphereBeginOverlap);
		AreaSphere->OnComponentEndOverlap.AddDynamic(this, &AWeapon::OnSphereEndOverlap);
	}
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(false);
	}
}

void AWeapon::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                   UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
                                   const FHitResult& SweepResult)
{
	//GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("AWeapon::OnSphereBeginOverlap"));
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(this);
	}
}

void AWeapon::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                 UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	//GEngine->AddOnScreenDebugMessage(-1, 60.f, FColor::Red, TEXT("AWeapon::OnSphereEndOverlap"));
	ABlasterCharacter* BlasterCharacter = Cast<ABlasterCharacter>(OtherActor);
	if (BlasterCharacter)
	{
		BlasterCharacter->SetOverlappingWeapon(nullptr);
	}
}

void AWeapon::OnRep_WeaponState()
{
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		break;
	}
}

void AWeapon::SetWeaponState(EWeaponState State)
{
	WeaponState = State;
	switch (WeaponState)
	{
	case EWeaponState::EWS_Equipped:
		ShowPickupWidget(false);
		AreaSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		break;
	}
}

void AWeapon::ShowPickupWidget(bool bShowWidget)
{
	if (PickupWidget)
	{
		PickupWidget->SetVisibility(bShowWidget);
	}
}

void AWeapon::Fire(const FVector& HitTarget)
{
	// 通过WeaponMesh播放武器射击动画
	if (FireAnimation && WeaponMesh)
	{
		// 循环播放参数设置为false
		WeaponMesh->PlayAnimation(FireAnimation, false);

		// 弹出子弹壳
		if (CasingClass)
		{
			// 获取AmmoEject插槽对象
			const USkeletalMeshSocket* AmmoEjectSocket = WeaponMesh->GetSocketByName(FName("AmmoEject"));
			if (AmmoEjectSocket)
			{
				// 获取弹壳插槽位置的转换，即在这个位置生成弹壳
				FTransform SocketTransform = AmmoEjectSocket->GetSocketTransform(WeaponMesh);
				UWorld* World = GetWorld();
				if (World)
				{
					World->SpawnActor<ACasing>(
						CasingClass, // 生成的弹壳类
						SocketTransform.GetLocation(), // 生成弹壳位置
						SocketTransform.GetRotation().Rotator() // 生成弹壳的旋转
					);
				}
			}
		}
	}
}
