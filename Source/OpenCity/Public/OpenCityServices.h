#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OpenCityTypes.h"
#include "OpenCityServices.generated.h"

class AOpenCityPoliceNPC;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOpenCityTerritoryChanged, FName, TerritoryId, FName, OwnerFaction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOpenCityMissionSignal, FName, MissionId);

UCLASS()
class OPENCITY_API UOpenCityServiceSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UPROPERTY(BlueprintReadOnly) TArray<FOpenCityBusinessData> Businesses;
    UPROPERTY(BlueprintReadOnly) TArray<FOpenCityTerritoryData> Territories;
    UPROPERTY(BlueprintAssignable) FOpenCityTerritoryChanged OnTerritoryChanged;

    UFUNCTION(BlueprintCallable) bool BuyBusiness(FName BusinessId);
    UFUNCTION(BlueprintCallable) int32 CollectBusinessIncome();
    UFUNCTION(BlueprintCallable) bool AddTerritoryInfluence(FName TerritoryId, FName Faction, float Amount);
    UFUNCTION(BlueprintCallable) void SetFactionRelation(FName First, FName Second, int32 Relation);
    UFUNCTION(BlueprintPure) int32 GetFactionRelation(FName First, FName Second) const;

private:
    TMap<FString, int32> FactionRelations;
    FString MakeRelationKey(FName First, FName Second) const;
};

UCLASS()
class OPENCITY_API AOpenCityMissionDirector : public AActor
{
    GENERATED_BODY()
public:
    AOpenCityMissionDirector();
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadOnly) FName ActiveMission;
    UPROPERTY(BlueprintReadOnly) FVector ActiveTarget = FVector::ZeroVector;
    UPROPERTY(BlueprintReadOnly) float TargetRadius = 250.f;
    UPROPERTY(BlueprintReadOnly) int32 ObjectiveIndex = 0;
    UPROPERTY(BlueprintAssignable) FOpenCityMissionSignal OnMissionStarted;
    UPROPERTY(BlueprintAssignable) FOpenCityMissionSignal OnMissionCompleted;
    UPROPERTY(BlueprintAssignable) FOpenCityMissionSignal OnMissionFailed;

    UFUNCTION(BlueprintCallable) bool BeginMission(FName MissionId);
    UFUNCTION(BlueprintCallable) void AdvanceObjective(FVector NewTarget);
    UFUNCTION(BlueprintCallable) void FailActiveMission();
};

UCLASS()
class OPENCITY_API AOpenCityPoliceDirector : public AActor
{
    GENERATED_BODY()
public:
    AOpenCityPoliceDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) float DispatchRadius = 3500.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float WantedDecaySeconds = 45.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 UnitsPerWantedLevel = 2;

    UFUNCTION(BlueprintCallable) void ReportCrime(AActor* Suspect, int32 Severity);

private:
    float DecayTimer = 0.f;
    UPROPERTY() TObjectPtr<AActor> CurrentSuspect;
    UPROPERTY() TArray<TObjectPtr<AOpenCityPoliceNPC>> Units;
    void UpdateDispatch();
};
