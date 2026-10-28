#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OpenCityTypes.h"
#include "OpenCityWorld.generated.h"

class UBoxComponent;
class USphereComponent;
class UPointLightComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTextRenderComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOpenCityCheckpointReached, int32, CheckpointIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOpenCityWeatherChanged, EOpenCityWeather, Weather);

UCLASS()
class OPENCITY_API AOpenCityWorldGenerator : public AActor
{
    GENERATED_BODY()
public:
    AOpenCityWorldGenerator();
    virtual void OnConstruction(const FTransform& Transform) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") int32 GridRadius = 4;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") float BlockSize = 2400.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") int32 Seed = 1337;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="World") bool bGenerateOnConstruction = true;

    UFUNCTION(BlueprintCallable, CallInEditor) void GenerateWorld();
    UFUNCTION(BlueprintCallable, CallInEditor) void ClearWorld();

private:
    UPROPERTY() TArray<TObjectPtr<AActor>> GeneratedActors;
    UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
    UPROPERTY() TObjectPtr<UStaticMesh> CylinderMesh;
    AActor* SpawnBlockPart(const FVector& Location, const FVector& Scale, const FString& Label, bool bCollision = true);
};

UCLASS()
class OPENCITY_API AOpenCityWorldDirector : public AActor
{
    GENERATED_BODY()
public:
    AOpenCityWorldDirector();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, ReplicatedUsing=OnRep_Weather) EOpenCityWeather Weather = EOpenCityWeather::Clear;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated) float WorldHour = 9.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float MinutesPerDay = 36.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float WeatherChangeMinutes = 8.f;
    UPROPERTY(BlueprintAssignable) FOpenCityWeatherChanged OnWeatherChanged;
    UPROPERTY(BlueprintReadOnly) TArray<FOpenCityWorldEvent> ActiveEvents;

    UFUNCTION(BlueprintCallable) void SetWeather(EOpenCityWeather NewWeather);
    UFUNCTION(BlueprintCallable) FOpenCityWorldEvent SpawnRandomEvent();
    UFUNCTION(BlueprintCallable) bool ResolveEvent(FName EventId, bool bSuccess);

private:
    float WeatherTimer = 0.f;
    float EventTimer = 0.f;
    float IncomeTimer = 0.f;
    UFUNCTION() void OnRep_Weather();
};

UCLASS()
class OPENCITY_API AOpenCityCheckpoint : public AActor
{
    GENERATED_BODY()
public:
    AOpenCityCheckpoint();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USphereComponent> Trigger;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Marker;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 CheckpointIndex = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName MissionId;
    UPROPERTY(BlueprintAssignable) FOpenCityCheckpointReached OnReached;

private:
    UFUNCTION() void HandleOverlap(UPrimitiveComponent* Overlapped, AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};

UCLASS()
class OPENCITY_API AOpenCityPropertyActor : public AActor
{
    GENERATED_BODY()
public:
    AOpenCityPropertyActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> Trigger;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UTextRenderComponent> Sign;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName PropertyId = TEXT("Property_01");
    UFUNCTION(BlueprintCallable) bool Purchase(AActor* Buyer);
};

UCLASS()
class OPENCITY_API AOpenCityGarage : public AActor
{
    GENERATED_BODY()
public:
    AOpenCityGarage();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UBoxComponent> ServiceArea;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 RepairPrice = 500;
    UFUNCTION(BlueprintCallable) bool RepairVehicle(AActor* VehicleActor);
    UFUNCTION(BlueprintCallable) bool RepaintVehicle(AActor* VehicleActor, FLinearColor Color);
};

UCLASS()
class OPENCITY_API AOpenCityTrafficManager : public AActor
{
    GENERATED_BODY()
public:
    AOpenCityTrafficManager();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TargetVehicleCount = 18;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) int32 TargetPedestrianCount = 26;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float SpawnRadius = 9000.f;

private:
    float SpawnTimer = 0.f;
    UPROPERTY() TArray<TObjectPtr<AActor>> TrafficActors;
    UPROPERTY() TArray<TObjectPtr<AActor>> Pedestrians;
    void MaintainPopulation();
};
