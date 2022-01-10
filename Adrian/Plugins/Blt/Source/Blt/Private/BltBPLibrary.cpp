// Copyright Epic Games, Inc. All Rights Reserved.

#include "BltBPLibrary.h"
#include "Kismet/GameplayStatics.h"

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
		
		for (TFieldIterator<FProperty> Iterator(Actor->GetClass()); Iterator; ++Iterator)
		{
			const FProperty* const Property = *Iterator;
			const FString& PropertyName = Property->GetNameCPP();
			if (!ActorClassProperties.Contains(PropertyName))
				continue;

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
	const TArray<TSharedPtr<FJsonValue>>& Interval = PropertyValue->AsArray();
	const float IntervalMin = Interval[0].Get()->AsNumber();
	const float IntervalMax = Interval[1].Get()->AsNumber();
							
	const FNumericProperty* const NumericProperty = CastField<const FNumericProperty>(Property);
	NumericProperty->SetNumericPropertyValueFromString(
		NumericProperty->ContainerPtrToValuePtr<float>(Actor),
		*FString::Printf(TEXT("%f"), FMath::RandRange(IntervalMin, IntervalMax))
	);
}

void UBltBPLibrary::RandomiseStringProperty(
	const FProperty* const Property,
	const FJsonValue* const PropertyValue,
	AActor* const Actor
)
{
	// TODO...
	
	const FStrProperty* const StringProperty = CastField<const FStrProperty>(Property);
	StringProperty->SetPropertyValue_InContainer(Actor, PropertyValue->AsString());
}