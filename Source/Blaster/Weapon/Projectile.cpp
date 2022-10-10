// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectile.h"

#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	// 开启子弹可复制
	bReplicates = true;

	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	SetRootComponent(CollisionBox);
	CollisionBox->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionBox->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
	CollisionBox->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	// 子弹旋转方向和速度保持一致
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	

}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (Tracer)
	{
		TracerComponent = UGameplayStatics::SpawnEmitterAttached(
			Tracer,
			CollisionBox,
			FName(),
			GetActorLocation(),
			GetActorRotation(),
			EAttachLocation::KeepWorldPosition
			);
	}
	
	if (HasAuthority() && CollisionBox)
	{
		// 只在服务端响应撞击事件
		CollisionBox->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}
	
}

void AProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AProjectile::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	// 调用销毁函数，当子弹被销毁以后会调用我们重写的Destroyed()函数
	//Destroy();

	// 从现在开始.001秒后，销毁子弹，如果直接调用Destroy发现向身边射击时不播放特效，原因？
	GetWorldTimerManager().SetTimer(DestroyProjectileTimerHandle, this, &AProjectile::DestroyProjectile, 1.0f, false, .001f);
}

void AProjectile::DestroyProjectile()
{
	// 取消定时器
	GetWorldTimerManager().ClearTimer(DestroyProjectileTimerHandle);
	// 销毁子弹
	Destroy();
}

void AProjectile::Destroyed()
{
	Super::Destroyed();

	// ENetRole LocalNetRole = GetLocalRole();
	// switch (LocalNetRole)
	// {
	// case ENetRole::ROLE_Authority:
	// 	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("AProjectile::Destroyed"));
	// 	break;
	// case ENetRole::ROLE_AutonomousProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("AProjectile::Destroyed"));
	// 	break;
	// case ENetRole::ROLE_SimulatedProxy:
	// 	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("AProjectile::Destroyed"));
	// 	break;
	// case ENetRole::ROLE_None:
	// 	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Blue, TEXT("AProjectile::Destroyed"));
	// 	break;
	// }

	// 播放撞击粒子特效
	if (ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, GetActorTransform());
	}
	// 播放撞击音效
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, GetActorLocation());
	}
}
