// Fill out your copyright notice in the Description page of Project Settings.


#include "Fuzzer.h"


Fuzzer::Fuzzer()
{
}

Fuzzer::~Fuzzer()
{
}

std::map<FString, FProperty*> Fuzzer::getProperties(UObject* targetObject)
{
	std::fstream fs;
	fs.open("C:\\Users\\Q\\Desktop\\fuzzerProperties.txt", std::fstream::in | std::fstream::out | std::fstream::trunc);
	std::map<FString, FProperty*> local_property_map;
	for (TFieldIterator<UProperty> PropIt(targetObject->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Property = *PropIt;
		FString property_name = Property->GetNameCPP();
		local_property_map[property_name] = Property;

		FString property_type = Property->GetCPPType();
		fs << TCHAR_TO_UTF8(*property_name) << " " << TCHAR_TO_UTF8(*property_type) << '\n';

	}
	fs.close();
	return local_property_map;
}



void Fuzzer::myFuzzer(UObject* targetObject, std::map<FString, FProperty*> property_map) {
	


	GEngine->AddOnScreenDebugMessage(-1, 4, FColor::Green, "Pressed R");

	std::fstream fs;
	fs.open("C:\\Users\\Q\\Desktop\\fieldValues.json", std::fstream::in | std::fstream::out | std::fstream::app);

	std::string s;
	std::map<FString, FProperty*> global_property_map = getProperties(targetObject);

	while (fs >> s) {

		int pos = s.find_first_of(':');
		if (pos > -1) {
			s.erase(s.length() - 1, 1);
			std::string var_name = s.substr(1, pos - 2), var_range = s.substr(pos + 1);
			var_range = var_range.substr(1, var_range.length() - 2);
			int comma_pos = var_range.find_first_of(',');
			if (comma_pos > -1) {
				std::string range_min = var_range.substr(0, var_range.find_first_of(','));
				std::string range_max = var_range.substr(var_range.find_first_of(',') + 1);
				int first = stoi(range_min), second = stoi(range_max);
				FName property_name = UTF8_TO_TCHAR(var_name.c_str());
				FInt64Property* NumProperty = FindField<FInt64Property>(targetObject->GetClass(), property_name);
				int64 property_value = -1;
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> distr(first, second);

				if (global_property_map.find(property_name.ToString()) != global_property_map.end()) {
					NumProperty->SetPropertyValue_InContainer(targetObject, distr(gen));
					property_value = NumProperty->GetPropertyValue_InContainer(targetObject);
					GEngine->AddOnScreenDebugMessage(-1, 4, FColor::Green, property_name.ToString() + FString::Printf(TEXT(": %d"), property_value));
					FString string_property_name = property_name.ToString();
					property_map.erase(string_property_name);
				}
			}
		}
	}
	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> distr(1, 1000000);
	for (std::map<FString, FProperty*>::iterator it = property_map.begin(); it != property_map.end(); ++it) {
		FString int_type = "int64";
		if (it->second->GetCPPType() == int_type)
		{
			FName property_name(it->first);
			FInt64Property* NumProperty = FindField<FInt64Property>(targetObject->GetClass(), property_name);
			NumProperty->SetPropertyValue_InContainer(targetObject, distr(gen));
			int64 property_value = NumProperty->GetPropertyValue_InContainer(targetObject);
			GEngine->AddOnScreenDebugMessage(-1, 4, FColor::Green, property_name.ToString() + FString::Printf(TEXT(" (not in JSON): %d"), property_value));
					
		}
		
		//GEngine->AddOnScreenDebugMessage(-1, 4, FColor::Green, it->first);

	}


	GEngine->AddOnScreenDebugMessage(-1, 500, FColor::Green, " ");

	fs.close();


}

