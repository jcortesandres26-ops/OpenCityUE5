#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "OpenCityTypes.h"
#include "OpenCityNetwork.generated.h"

UCLASS()
class OPENCITY_API AOpenCityPlayerState : public APlayerState
{
    GENERATED_BODY()
public:
    AOpenCityPlayerState();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(BlueprintReadOnly, Replicated) int32 Money = 2500;
    UPROPERTY(BlueprintReadOnly, Replicated) int32 WantedLevel = 0;
    UPROPERTY(BlueprintReadOnly, Replicated) FName Faction = TEXT("Independent");
    UPROPERTY(BlueprintReadOnly, Replicated) int32 Reputation = 0;
};

UCLASS()
class OPENCITY_API AOpenCityGameState : public AGameStateBase
{
    GENERATED_BODY()
public:
    AOpenCityGameState();
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(BlueprintReadOnly, Replicated) float WorldHour = 9.f;
    UPROPERTY(BlueprintReadOnly, Replicated) EOpenCityWeather Weather = EOpenCityWeather::Clear;
    UPROPERTY(BlueprintReadOnly, Replicated) int32 WorldSeed = 1337;
};

UCLASS()
class OPENCITY_API AOpenCityPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    AOpenCityPlayerController();
    virtual void SetupInputComponent() override;

    UPROPERTY(BlueprintReadOnly) bool bMapOpen = false;
    UPROPERTY(BlueprintReadOnly) bool bInventoryOpen = false;

    UFUNCTION(BlueprintCallable) void TogglePauseMenu();
    UFUNCTION(BlueprintCallable) void ToggleWorldMap();
    UFUNCTION(BlueprintCallable) void ToggleInventory();
    UFUNCTION(BlueprintCallable) void QuickSave();
    UFUNCTION(BlueprintCallable) void QuickLoad();
    UFUNCTION(Server, Reliable, BlueprintCallable) void ServerReportCrime(int32 Severity);
};
