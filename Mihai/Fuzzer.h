// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "UObject/UnrealType.h"
#include <Runtime/CoreUObject/Public/UObject/ObjectMacros.h>
#include "Math/UnrealMathUtility.h"
#include "UObject/Class.h"
#include <string>
#include <random>
#include <fstream>
#include <map>

/**
 * 
 */
class MYPROJECT6_API Fuzzer
{
public:
	Fuzzer();
	~Fuzzer();

	UFUNCTION(BlueprintCallable)
	void myFuzzer(UObject* targetObject, std::map<FString, FProperty*> property_map);

private:
	std::map<FString, FProperty*> getProperties(UObject* targetObject);


};
