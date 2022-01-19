// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once


class FBltModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
