#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SaveGame.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "OpenCityTypes.h"
#include "OpenCityCore.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UStaticMeshComponent;
class USphereComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class ADirectionalLight;
class UOpenCityHealthComponent;
class UOpenCityInventoryComponent;
class UOpenCityWeaponComponent;
class UOpenCityDialogueComponent;
class UOpenCityRoutineComponent;
class UOpenCityMotionComponent;

UENUM(BlueprintType)
enum class EMissionState : uint8 { Locked, Available, Active, Completed, Failed };

UENUM(BlueprintType)
enum class EFactionAttitude : uint8 { Friendly, Neutral, Hostile };

USTRUCT(BlueprintType)
struct FOpenCityMission
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FText Title;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FText Description;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Reward = 500;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) EMissionState State = EMissionState::Available;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FVector Checkpoint = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct FInventoryItem
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Quantity = 1;
};

USTRUCT(BlueprintType)
struct FPropertyRecord
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) FName Id;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) bool bOwned = false;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 Price = 10000;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame) int32 DailyIncome = 0;
};

UCLASS()
class OPENCITY_API UOpenCitySaveGame : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY(SaveGame) FVector PlayerLocation = FVector::ZeroVector;
    UPROPERTY(SaveGame) FRotator PlayerRotation = FRotator::ZeroRotator;
    UPROPERTY(SaveGame) int32 Money = 2500;
    UPROPERTY(SaveGame) float Health = 100.f;
    UPROPERTY(SaveGame) float Armor = 0.f;
    UPROPERTY(SaveGame) int32 WantedLevel = 0;
    UPROPERTY(SaveGame) int32 CurrentWeaponIndex = 0;
    UPROPERTY(SaveGame) int32 MagazineAmmo = 0;
    UPROPERTY(SaveGame) int32 ReserveAmmo = 0;
    UPROPERTY(SaveGame) float WorldTime = 12.f;
    UPROPERTY(SaveGame) TArray<FOpenCityMission> Missions;
    UPROPERTY(SaveGame) TArray<FInventoryItem> Inventory;
    UPROPERTY(SaveGame) TArray<FPropertyRecord> Properties;
    UPROPERTY(SaveGame) TMap<FName, int32> FactionReputation;
    UPROPERTY(SaveGame) TMap<FName, int32> TerritoryOwners;
    UPROPERTY(SaveGame) TArray<FOpenCityBusinessData> Businesses;
    UPROPERTY(SaveGame) TArray<FOpenCityTerritoryData> Territories;
};

UCLASS()
class OPENCITY_API UOpenCityGameSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    UPROPERTY(BlueprintReadOnly) int32 Money = 2500;
    UPROPERTY(BlueprintReadOnly) float Health = 100.f;
    UPROPERTY(BlueprintReadOnly) float Armor = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 WantedLevel = 0;
    UPROPERTY(BlueprintReadOnly) float WorldTime = 12.f;
    UPROPERTY(BlueprintReadOnly) TArray<FOpenCityMission> Missions;
    UPROPERTY(BlueprintReadOnly) TArray<FInventoryItem> Inventory;
    UPROPERTY(BlueprintReadOnly) TArray<FPropertyRecord> Properties;
    UPROPERTY(BlueprintReadOnly) TMap<FName, int32> FactionReputation;
    UPROPERTY(BlueprintReadOnly) TMap<FName, int32> TerritoryOwners;

    UFUNCTION(BlueprintCallable) bool StartMission(FName MissionId);
    UFUNCTION(BlueprintCallable) bool CompleteMission(FName MissionId);
    UFUNCTION(BlueprintCallable) void FailMission(FName MissionId);
    UFUNCTION(BlueprintCallable) void AddMoney(int32 Amount);
    UFUNCTION(BlueprintCallable) bool SpendMoney(int32 Amount);
    UFUNCTION(BlueprintCallable) void AddWantedLevel(int32 Amount);
    UFUNCTION(BlueprintCallable) void ClearWantedLevel();
    UFUNCTION(BlueprintCallable) void AddInventoryItem(FName ItemId, int32 Quantity);
    UFUNCTION(BlueprintCallable) bool BuyProperty(FName PropertyId);
    UFUNCTION(BlueprintCallable) bool SaveGameToSlot(FString SlotName = TEXT("OpenCitySave"));
    UFUNCTION(BlueprintCallable) bool LoadGameFromSlot(FString SlotName = TEXT("OpenCitySave"));
private:
    void GenerateMissionCatalog();
};

UCLASS()
class OPENCITY_API AOpenCityCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AOpenCityCharacter();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> FollowCamera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> BodyVisual;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USphereComponent> InteractionRange;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UOpenCityHealthComponent> HealthComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UOpenCityInventoryComponent> InventoryComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UOpenCityWeaponComponent> WeaponComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UOpenCityMotionComponent> MotionComponent;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float WalkSpeed = 360.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float SprintSpeed = 680.f;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<class AOpenCityVehicle> NearbyVehicle;

    UFUNCTION(BlueprintCallable) void SetNearbyVehicle(AOpenCityVehicle* Vehicle);
    UFUNCTION(BlueprintCallable) void EnterNearbyVehicle();

private:
    void MoveForward(float Value);
    void MoveRight(float Value);
    void StartSprint();
    void StopSprint();
    void ToggleCrouch();
    void FireWeapon();
    void ReloadWeapon();
    void NextWeapon();
    void FindNearbyVehicle();
};

UCLASS()
class OPENCITY_API AOpenCityVehicle : public APawn
{
    GENERATED_BODY()
public:
    AOpenCityVehicle();
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> VehicleBody;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UFloatingPawnMovement> Movement;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<USpringArmComponent> CameraBoom;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UOpenCityHealthComponent> HealthComponent;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated) EOpenCityVehicleClass VehicleClass = EOpenCityVehicleClass::Car;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated) float Fuel = 100.f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float FuelBurnPerSecond = 0.35f;
    UPROPERTY(BlueprintReadOnly, Replicated) TObjectPtr<AOpenCityCharacter> Driver;
    UFUNCTION(BlueprintCallable) void EnterVehicle(AOpenCityCharacter* NewDriver);
    UFUNCTION(BlueprintCallable) void ExitVehicle();
    UFUNCTION(BlueprintCallable) void Refuel(float Amount);
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    float ThrottleInput = 0.f;
    void ApplyThrottle(float Value);
    void ApplySteering(float Value);
};

UCLASS()
class OPENCITY_API AOpenCityAIController : public AAIController
{
    GENERATED_BODY()
public:
    AOpenCityAIController();
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UAIPerceptionComponent> Perception;
protected:
    UFUNCTION() void HandlePerception(const TArray<AActor*>& UpdatedActors);
private:
    UPROPERTY() TObjectPtr<UAISenseConfig_Sight> SightConfig;
};

UCLASS()
class OPENCITY_API AOpenCityNPC : public ACharacter
{
    GENERATED_BODY()
public:
    AOpenCityNPC();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> BodyVisual;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UOpenCityHealthComponent> HealthComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UOpenCityDialogueComponent> DialogueComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UOpenCityRoutineComponent> RoutineComponent;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UOpenCityMotionComponent> MotionComponent;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) FName Faction = TEXT("Civilian");
    UPROPERTY(EditAnywhere, BlueprintReadWrite) EFactionAttitude Attitude = EFactionAttitude::Neutral;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TArray<FText> DialogueLines;
};

UCLASS()
class OPENCITY_API AOpenCityPoliceNPC : public AOpenCityNPC
{
    GENERATED_BODY()
public:
    AOpenCityPoliceNPC();
};

UCLASS()
class OPENCITY_API AOpenCityDayNightController : public AActor
{
    GENERATED_BODY()
public:
    AOpenCityDayNightController();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) TObjectPtr<ADirectionalLight> Sun;
    UPROPERTY(EditAnywhere, BlueprintReadWrite) float DayLengthMinutes = 30.f;
};

UCLASS()
class OPENCITY_API AOpenCityHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
};

UCLASS()
class OPENCITY_API AOpenCityGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AOpenCityGameMode();
    virtual void BeginPlay() override;
};
