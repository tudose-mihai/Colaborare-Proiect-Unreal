// Fill out your copyright notice in the Description page of Project Settings.


#include "MyCharacter.h"
#include "Math/UnrealMathUtility.h"
#include "UObject/Class.h"
#include "Fuzzer.h"
#include <string>
#include <random>
#include <fstream>
#include <map>

std::map<FString, FProperty*> property_map;
std::map<FString, FProperty*>::iterator it;

// Sets default values
AMyCharacter::AMyCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	test_WalkSpeed = 200;
	test_RunSpeed = 1000;
	test_JumpHeight = 1000;
}

// Called when the game starts or when spawned
void AMyCharacter::BeginPlay()
{
	MyPropertyLogger();
	LogNewProperties();
	Super::BeginPlay();
	
}

// Called every frame
void AMyCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

// Called to bind functionality to input
void AMyCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

void AMyCharacter::MyPropertyLogger() {
	std::fstream fs;
	fs.open("C:\\Users\\Q\\Desktop\\currentProperties.txt", std::fstream::in | std::fstream::out | std::fstream::trunc);

	for (TFieldIterator<UProperty> PropIt(GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		FString property_name = Property->GetNameCPP();
		property_map[property_name] = Property;

		FString property_type = Property->GetCPPType();
		fs << TCHAR_TO_UTF8(*property_name);
		//fs << " " << TCHAR_TO_UTF8(*property_type);
		fs << '\n';

	}
	fs.close();

	

}

void AMyCharacter::LogNewProperties() {
	std::fstream fs;
	std::map<FString, FProperty*> comparison_map = property_map;
	fs.open("C:\\Users\\Q\\Desktop\\oldProperties.txt", std::fstream::in | std::fstream::out | std::fstream::app);
	std::string line;
	while (std::getline(fs, line))
	{
		FString p_name = UTF8_TO_TCHAR(line.c_str());
		comparison_map.erase(p_name);
	}
	fs.close();
	if (comparison_map.empty())
		GEngine->AddOnScreenDebugMessage(-1, 500, FColor::Red, "No new properties");
	else {
		GEngine->AddOnScreenDebugMessage(-1, 500, FColor::Red, "New properties:");

		for (it = comparison_map.begin(); it != comparison_map.end(); ++it) {
			GEngine->AddOnScreenDebugMessage(-1, 4, FColor::Red, it->first);
		}
	}
	
	std::fstream fs1;
	fs.open("C:\\Users\\Q\\Desktop\\oldProperties.txt", std::fstream::in | std::fstream::out | std::fstream::trunc);
	fs1.open("C:\\Users\\Q\\Desktop\\currentProperties.txt", std::fstream::in | std::fstream::out | std::fstream::app);
	while (std::getline(fs1, line))
	{
		fs << line << '\n';
	}
	fs.close();
	fs1.close();


}


void AMyCharacter::MyFuzzer() {
	GEngine->AddOnScreenDebugMessage(-1, 4, FColor::Green, "Pressed R");

	std::fstream fs;
	fs.open("C:\\Users\\Q\\Desktop\\fieldValues.json", std::fstream::in | std::fstream::out | std::fstream::app);

	std::string s;

	while (fs >> s) {
		int pos = s.find_first_of(':');
		if (pos > -1) {

			s.erase(s.length() - 1, 1);
			std::string var_name = s.substr(1, pos-2), var_range = s.substr(pos + 1);
			var_range = var_range.substr(1, var_range.length() - 2);
			int comma_pos = var_range.find_first_of(',');
			if (comma_pos > -1) {
				int first = stoi(var_range.substr(0, var_range.find_first_of(',')));
				int second = stoi(var_range.substr(var_range.find_first_of(',') + 1));
				FName property_name = UTF8_TO_TCHAR(var_name.c_str());
				FInt64Property* NumProperty = FindField<FInt64Property>(this->GetClass(), property_name);
				int64 property_value = -1;
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> distr(first, second);
				
				if (property_map.find(property_name.ToString()) != property_map.end()) {
					NumProperty->SetPropertyValue_InContainer(this, distr(gen));
					property_value = NumProperty->GetPropertyValue_InContainer(this);
					GEngine->AddOnScreenDebugMessage(-1, 4, FColor::Green, property_name.ToString() + FString::Printf(TEXT(": %d"), property_value));
				
				}
			}
		}
	}
	GEngine->AddOnScreenDebugMessage(-1, 500, FColor::Green, " ");

	fs.close();

}

void AMyCharacter::InheritedFuzzer() {

	// current_properties - base_properties

	std::fstream fs;
	fs.open("C:\\Users\\Q\\Desktop\\baseProperties.txt", std::fstream::in | std::fstream::out | std::fstream::app);
	std::string line;
	while (std::getline(fs, line))
	{
		FString p_name = UTF8_TO_TCHAR(line.c_str());
		property_map.erase(p_name);
	}
	fs.close();

	Fuzzer fuzzer;
	fuzzer.myFuzzer(this,property_map);

	

}