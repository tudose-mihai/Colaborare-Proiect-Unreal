// Copyright Epic Games, Inc. All Rights Reserved.

#include "BltBPLibrary.h"

#include "Kismet/GameplayStatics.h"
#include "PythonBridge.h"

DEFINE_LOG_CATEGORY(LogBlt);


bool UBltBPLibrary::ParseJson(const FString& FilePath, TSharedPtr<FJsonObject>& OutObject)
{
	FString AbsoluteFilePath;
	if (!GetAbsolutePath(FilePath, AbsoluteFilePath))
		return false;
	
	FString JsonRaw;
	FFileHelper::LoadFileToString(JsonRaw, *AbsoluteFilePath);
	if (!FJsonSerializer::Deserialize<TCHAR>(TJsonReaderFactory<TCHAR>::Create(JsonRaw), OutObject))
	{
		UE_LOG(LogBlt, Error, TEXT("Could not deserialize %s [check if file is JSON]"), *AbsoluteFilePath);
		return false;
	}

	return true;
}

bool UBltBPLibrary::GetAbsolutePath(const FString& FilePath, FString& AbsoluteFilePath)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	
	AbsoluteFilePath = FilePath;
	if (PlatformFile.FileExists(*AbsoluteFilePath))
		return true;

	AbsoluteFilePath = FPaths::ProjectContentDir() + FilePath;
	if (PlatformFile.FileExists(*AbsoluteFilePath))
		return true;

	UE_LOG(LogBlt, Error, TEXT("File %s not found [relative path starts from /Content/]"), *AbsoluteFilePath);
	return false;
}

UClass* UBltBPLibrary::FindClass(const FString& ClassName)
{
	check(*ClassName);
	
	if (UClass* const ClassType = FindObject<UClass>(ANY_PACKAGE, *ClassName))
		return ClassType;

	if (const UObjectRedirector* const RenamedClassRedirector = FindObject<UObjectRedirector>(ANY_PACKAGE, *ClassName))
		return CastChecked<UClass>(RenamedClassRedirector->DestinationObject);

	return nullptr;
}

TArray<AActor*> UBltBPLibrary::GetAllActorsOfClass(
	const UObject* const WorldContextObject,
	const FString& ActorClassName
)
{
	const TSubclassOf<AActor> ActorClass = FindClass(ActorClassName);
	if (!ActorClass)
	{
		UE_LOG(LogBlt, Warning, TEXT("Class %s could not be found!"), *ActorClassName);
	}

	TArray<AActor*> OutActors;
	UGameplayStatics::GetAllActorsOfClass(WorldContextObject->GetWorld(), ActorClass, OutActors);
	return OutActors;
}

void UBltBPLibrary::ApplyFuzzing(
	const UObject* const WorldContextObject,
	const FString& FilePath,
	const TArray<AActor*>& AffectedActors,
	const bool bUseArray
)
{
	TSharedPtr<FJsonObject> JsonParsed;
	if (!ParseJson(FilePath, JsonParsed))
		return;

	const TMap<FString, TSharedPtr<FJsonValue>> JsonClasses = JsonParsed.Get()->Values;
	for (const TTuple<FString, TSharedPtr<FJsonValue>>& JsonClass : JsonClasses)
	{
		const FString& ActorClassName = JsonClass.Key;
		const TSharedPtr<FJsonObject>* ActorClassObject;
		if (!JsonClass.Value->TryGetObject(ActorClassObject))
		{
			UE_LOG(LogBlt, Error, TEXT("Entry %s must have an Object type value!"), *ActorClassName);
			continue;
		}

		RandomiseProperties(
			ActorClassObject,
			bUseArray ? AffectedActors : GetAllActorsOfClass(WorldContextObject, ActorClassName)
		);
	}
}

void UBltBPLibrary::K2ApplyFuzzing(
	const UObject* const WorldContextObject,
	const FString& FilePath,
	const TArray<AActor*>& AffectedActors,
	const bool bUseArray
)
{
	ApplyFuzzing(WorldContextObject, FilePath, AffectedActors, bUseArray);
}

void UBltBPLibrary::RandomiseProperties(
	const TSharedPtr<FJsonObject>* ActorClassObject,
	const TArray<AActor*>& Actors
)
{
	const TMap<FString, TSharedPtr<FJsonValue>> ActorClassProperties = ActorClassObject->Get()->Values;
	
	for (AActor* const Actor : Actors)
	{

		if (!Actor)
			continue;

		FString currentProperties = FPaths::ProjectContentDir() + "Data\\currentProperties.txt";
		FString baseProperties = FPaths::ProjectContentDir() + "Data\\baseProperties.txt";
		if(GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 4, FColor::Green, baseProperties);

		TMap<FString, FProperty*> property_map = LogCurrentProperties(Actor, currentProperties);
		LogDefinedProperties(Actor, baseProperties, property_map);

		for (TFieldIterator<FProperty> Iterator(Actor->GetClass()); Iterator; ++Iterator)
		{
			const FProperty* const Property = *Iterator;
			const FString& PropertyName = Property->GetNameCPP();
			if (!ActorClassProperties.Contains(PropertyName))
			{
				if (property_map.Contains(PropertyName)) {
					RandomiseNumericProperty(Property, nullptr, Actor);
				}
				continue;
			}

			const FJsonValue* const PropertyValue = ActorClassProperties.Find(PropertyName)->Get();
			switch (PropertyValue->Type)
			{
			case EJson::Array:
				RandomiseNumericProperty(Property, PropertyValue, Actor);
				break;

			case EJson::String:
				RandomiseStringProperty(Property, PropertyValue, Actor);
				break;
				
			default:
				break;
			}
		}
	}
}

void UBltBPLibrary::RandomiseNumericProperty(
	const FProperty* const Property,
	const FJsonValue* const PropertyValue,
	AActor* const Actor
)
{
	const FNumericProperty* const NumericProperty = CastField<const FNumericProperty>(Property);
	if (!NumericProperty)
		return;
	 float IntervalMin;
	float IntervalMax;
	if (PropertyValue) {
		const TArray<TSharedPtr<FJsonValue>>& Interval = PropertyValue->AsArray();
		IntervalMin = Interval[0u].Get()->AsNumber();
		IntervalMax = Interval[1u].Get()->AsNumber();
	}
	else {
		IntervalMin = 0;
		IntervalMax = 1000000;
	}
	
	float RandomValue = FMath::RandRange(IntervalMin, IntervalMax);
							
	NumericProperty->SetNumericPropertyValueFromString(
		NumericProperty->ContainerPtrToValuePtr<float>(Actor),
		*FString::Printf(TEXT("%f"), RandomValue)
	);
	GEngine->AddOnScreenDebugMessage(-1, 500, FColor::Green, *Property->GetNameCPP());
	GEngine->AddOnScreenDebugMessage(-1, 500, FColor::Green, *FString::Printf(TEXT("%f"), RandomValue));


}

void UBltBPLibrary::RandomiseStringProperty(
	const FProperty* const Property,
	const FJsonValue* const PropertyValue,
	AActor* const Actor
)
{
	const UPythonBridge* const PythonBridge = UPythonBridge::Get();
	if (!PythonBridge)
	{
		UE_LOG(LogBlt, Error, TEXT("Python bridge could not be instantiated!"));
		return;
	}
	
	const FString& RandomString = PythonBridge->GenerateStringFromRegex(PropertyValue->AsString());

	const FStrProperty* const StringProperty = CastField<const FStrProperty>(Property);
	if (StringProperty)
	{
		StringProperty->SetPropertyValue_InContainer(Actor, RandomString);
		return;
	}
	
	const FNameProperty* const NameProperty = CastField<const FNameProperty>(Property);
	if (NameProperty)
	{
		NameProperty->SetPropertyValue_InContainer(Actor, FName(RandomString));
		return;
	}

	const FTextProperty* const TextProperty = CastField<const FTextProperty>(Property);
	if (TextProperty)
	{
		TextProperty->SetPropertyValue_InContainer(Actor, FText::FromString(RandomString));
		return;
	}

	UE_LOG(LogBlt, Fatal, TEXT("%s is not FString, FName or FText!"), *Property->GetFullName());
}

//////////

TMap<FString, FProperty*> UBltBPLibrary::LogCurrentProperties(UObject* targetObject,  FString& currentProperties) {
	std::fstream fsCurrent;
	std::string strFilePath = std::string(TCHAR_TO_UTF8(*currentProperties));
	fsCurrent.open(strFilePath, std::fstream::in | std::fstream::out | std::fstream::trunc);

	TMap<FString, FProperty*> property_map;
	for (TFieldIterator<FProperty> PropIt(targetObject->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		FString property_name = Property->GetNameCPP();
		property_map.Add(property_name, Property);

		FString property_type = Property->GetCPPType();
		fsCurrent << TCHAR_TO_UTF8(*property_name) << '\n';
	}

	fsCurrent.close();
	return property_map;

}


void UBltBPLibrary::LogNewProperties(UObject* targetObject,  FString& oldProperties,  FString& currentProperties) {
	std::fstream fsOld, fsCurrent;
	
	std::string strFilePath0 = std::string(TCHAR_TO_UTF8(*oldProperties));
	std::string strFilePath1 = std::string(TCHAR_TO_UTF8(*currentProperties));
	LogCurrentProperties(targetObject, currentProperties);
	TArray<FString> comparison;

	std::string line;

	fsCurrent.open(strFilePath1, std::fstream::in | std::fstream::out | std::fstream::app);
	while (std::getline(fsCurrent, line))
	{
		FString p_name = UTF8_TO_TCHAR(line.c_str());
		comparison.Add(p_name);
	}
	fsCurrent.close();

	fsOld.open(strFilePath0, std::fstream::in | std::fstream::out | std::fstream::app);
	while (std::getline(fsOld, line))
	{
		FString p_name = UTF8_TO_TCHAR(line.c_str());
		comparison.Remove(p_name);
	}
	fsOld.close();

	if (comparison.Num() > 0)
		GEngine->AddOnScreenDebugMessage(-1, 500, FColor::Red, "No new properties");
	else {
		GEngine->AddOnScreenDebugMessage(-1, 500, FColor::Red, "New properties:");

		for (const FString& elem : comparison) {
			GEngine->AddOnScreenDebugMessage(-1, 4, FColor::Red, elem);
		}
	}

	fsOld.open(strFilePath0, std::fstream::in | std::fstream::out | std::fstream::trunc);
	fsCurrent.open(strFilePath1, std::fstream::in | std::fstream::out | std::fstream::app);
	while (std::getline(fsCurrent, line))
	{
		fsOld << line << '\n';
	}
	fsOld.close();
	fsCurrent.close();


}

// take obj, baseprops filePath, map
// eliminates baseprops from map
// edits map to only contain user defined properties

// LogDefinedProperties(Actor, "Data\\baseProperties.txt", property_map);
void UBltBPLibrary::LogDefinedProperties(UObject* targetObject,  FString& baseProperties, TMap<FString, FProperty*>& property_map) {

	std::fstream fs;
	std::string strFilePath0 = std::string(TCHAR_TO_UTF8(*baseProperties));
	fs.open(strFilePath0, std::fstream::in | std::fstream::out | std::fstream::app);
	std::string line;

	while (std::getline(fs, line))
	{
		FString p_name = UTF8_TO_TCHAR(line.c_str());
		property_map.Remove(p_name);
	}
	fs.close();

}
