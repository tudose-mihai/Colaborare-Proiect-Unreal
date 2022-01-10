// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UObject/UnrealType.h"
#include <Runtime/CoreUObject/Public/UObject/ObjectMacros.h>
#include "MyCharacter.generated.h"



UCLASS()
class MYPROJECT6_API AMyCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AMyCharacter();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "test")
		int64 test_WalkSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "test")
		int64 test_RunSpeed;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "test")
		int64 test_JumpHeight;


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UFUNCTION(BlueprintCallable)
		void MyPropertyLogger();
	UFUNCTION(BlueprintCallable)
		void LogNewProperties();
	UFUNCTION(BlueprintCallable)
		void MyFuzzer();
	UFUNCTION(BlueprintCallable)
		void InheritedFuzzer();
};
