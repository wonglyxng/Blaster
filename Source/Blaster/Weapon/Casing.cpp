// Fill out your copyright notice in the Description page of Project Settings.


#include "Casing.h"
#include "kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundCue.h"

ACasing::ACasing()
{
	PrimaryActorTick.bCanEverTick = false;

	CasingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CasingMesh"));
	SetRootComponent(CasingMesh);
	
	// 忽略摄像机碰撞
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
	CasingMesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Ignore);
	// 开启物理模拟
	CasingMesh->SetSimulatePhysics(true);
	// 开启重力加速度
	CasingMesh->SetEnableGravity(true);
	// 开启生成 hit event，否则不会触发碰撞事件
	CasingMesh->SetNotifyRigidBodyCollision(true);

	ShellEjectionImpulse = 10.f;

}

void ACasing::BeginPlay()
{
	Super::BeginPlay();
	// 注册碰撞事件
	CasingMesh->OnComponentHit.AddDynamic(this, &ACasing::OnHit);

	// 计算随机方向
	const FRotator ShellRotator = GetActorRotation();
	SetActorRotation(FRotator(
		ShellRotator.Pitch + FMath::RandRange(-30., 30.),
		ShellRotator.Yaw + FMath::RandRange(-30., 30.),
		ShellRotator.Roll));
	const FVector ForwardVector = GetActorForwardVector();
	//UE_LOG(LogTemp, Warning, TEXT("ForwardVector.X: %f, ForwardVector.Y: %f, ForwardVector.Z: %f"), ForwardVector.X, ForwardVector.Y, ForwardVector.Z);
	// 给弹壳添加一个脉冲
	CasingMesh->AddImpulse(ForwardVector * ShellEjectionImpulse);
	
}

void ACasing::OnHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if (!bDestroyed)
	{
		//GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("ACasing::OnHit"));
		bDestroyed = true;
		if (ShellSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ShellSound, GetActorLocation());
		}
		// 从现在开始两秒后，销毁弹壳
		GetWorldTimerManager().SetTimer(DestroyShellTimerHandle, this, &ACasing::DestroyShell, 1.0f, false, 2.0f);
	}
}

void ACasing::DestroyShell()
{
	// 取消定时器
	GetWorldTimerManager().ClearTimer(DestroyShellTimerHandle);
	
	// 销毁弹壳
	Destroy();
}

