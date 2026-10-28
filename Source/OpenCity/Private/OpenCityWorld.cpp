#include "OpenCityWorld.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "OpenCityComponents.h"
#include "OpenCityCore.h"
#include "OpenCityNetwork.h"
#include "OpenCityVehicles.h"

AOpenCityWorldGenerator::AOpenCityWorldGenerator()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    CylinderMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
}

void AOpenCityWorldGenerator::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    if (bGenerateOnConstruction && GetWorld() && GetWorld()->IsGameWorld()) GenerateWorld();
}

AActor* AOpenCityWorldGenerator::SpawnBlockPart(const FVector& Location, const FVector& Scale, const FString& Label, bool bCollision)
{
    if (!GetWorld() || !CubeMesh) return nullptr;
    AStaticMeshActor* Part = GetWorld()->SpawnActor<AStaticMeshActor>(Location, FRotator::ZeroRotator);
    if (!Part) return nullptr;
#if WITH_EDITOR
    Part->SetActorLabel(Label);
#else
    Part->Tags.Add(FName(*Label));
#endif
    Part->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
    Part->SetReplicates(true);
    Part->SetReplicateMovement(false);
    Part->GetStaticMeshComponent()->SetMobility(EComponentMobility::Static);
    Part->GetStaticMeshComponent()->SetCollisionEnabled(bCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
    Part->SetActorScale3D(Scale);
    GeneratedActors.Add(Part);
    return Part;
}

void AOpenCityWorldGenerator::ClearWorld()
{
    for (AActor* Actor : GeneratedActors) if (IsValid(Actor)) Actor->Destroy();
    GeneratedActors.Empty();
}

void AOpenCityWorldGenerator::GenerateWorld()
{
    ClearWorld();
    FRandomStream Random(Seed);
    const float HalfWorld = (GridRadius + 1) * BlockSize;
    SpawnBlockPart(FVector(0.f, 0.f, -110.f), FVector(HalfWorld / 50.f, HalfWorld / 50.f, 1.f), TEXT("WorldGround"));

    for (int32 X = -GridRadius; X <= GridRadius; ++X)
    {
        for (int32 Y = -GridRadius; Y <= GridRadius; ++Y)
        {
            const FVector BlockCenter(X * BlockSize, Y * BlockSize, 0.f);
            SpawnBlockPart(BlockCenter + FVector(0.f, 0.f, -35.f), FVector(18.f, 18.f, 0.3f), TEXT("Sidewalk"));

            for (int32 LotX = -1; LotX <= 1; ++LotX)
            {
                for (int32 LotY = -1; LotY <= 1; ++LotY)
                {
                    if (FMath::Abs(LotX) + FMath::Abs(LotY) == 0 && Random.FRand() < 0.35f) continue;
                    const float Height = Random.FRandRange(500.f, X * X + Y * Y < 8 ? 2600.f : 1400.f);
                    const FVector Location = BlockCenter + FVector(LotX * 520.f, LotY * 520.f, Height * 0.5f);
                    SpawnBlockPart(Location, FVector(4.2f, 4.2f, Height / 100.f), TEXT("Building"));
                }
            }
        }
    }

    for (int32 Lane = -GridRadius; Lane <= GridRadius + 1; ++Lane)
    {
        const float Coordinate = (Lane - 0.5f) * BlockSize;
        SpawnBlockPart(FVector(Coordinate, 0.f, -22.f), FVector(3.f, HalfWorld / 50.f, 0.12f), TEXT("RoadNS"));
        SpawnBlockPart(FVector(0.f, Coordinate, -20.f), FVector(HalfWorld / 50.f, 3.f, 0.12f), TEXT("RoadEW"));
    }

    const FVector Airport(-HalfWorld * 0.72f, HalfWorld * 0.72f, 0.f);
    SpawnBlockPart(Airport, FVector(42.f, 8.f, 0.25f), TEXT("AirportRunway"));
    SpawnBlockPart(Airport + FVector(-1800.f, -900.f, 420.f), FVector(16.f, 6.f, 4.f), TEXT("AirportTerminal"));

    const FVector Port(HalfWorld * 0.78f, -HalfWorld * 0.72f, 0.f);
    SpawnBlockPart(Port, FVector(35.f, 22.f, 0.35f), TEXT("PortDock"));
    for (int32 Index = 0; Index < 8; ++Index)
        SpawnBlockPart(Port + FVector((Index % 4) * 340.f, (Index / 4) * 420.f, 140.f), FVector(3.f, 6.f, 1.4f), TEXT("Container"));

    for (int32 Index = 0; Index < 22; ++Index)
    {
        const float Angle = Random.FRandRange(0.f, UE_TWO_PI);
        const float Radius = Random.FRandRange(HalfWorld * 0.88f, HalfWorld * 1.18f);
        const float Height = Random.FRandRange(800.f, 3200.f);
        SpawnBlockPart(FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Height * 0.42f),
            FVector(Random.FRandRange(8.f, 24.f), Random.FRandRange(8.f, 24.f), Height / 100.f), TEXT("Mountain"));
    }
}

AOpenCityWorldDirector::AOpenCityWorldDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(false);
}

void AOpenCityWorldDirector::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority())
    {
        if (UOpenCityGameSubsystem* Systems = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>()) WorldHour = Systems->WorldTime;
    }
}

void AOpenCityWorldDirector::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AOpenCityWorldDirector, Weather);
    DOREPLIFETIME(AOpenCityWorldDirector, WorldHour);
}

void AOpenCityWorldDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority()) return;
    WorldHour = FMath::Fmod(WorldHour + DeltaSeconds * 24.f / FMath::Max(60.f, MinutesPerDay * 60.f), 24.f);
    WeatherTimer += DeltaSeconds;
    EventTimer += DeltaSeconds;
    IncomeTimer += DeltaSeconds;

    if (WeatherTimer >= WeatherChangeMinutes * 60.f)
    {
        WeatherTimer = 0.f;
        SetWeather(static_cast<EOpenCityWeather>(FMath::RandRange(0, static_cast<int32>(EOpenCityWeather::Sandstorm))));
    }
    if (EventTimer >= 90.f && ActiveEvents.Num() < 3)
    {
        EventTimer = 0.f;
        SpawnRandomEvent();
    }
    for (int32 Index = ActiveEvents.Num() - 1; Index >= 0; --Index)
    {
        ActiveEvents[Index].TimeRemaining -= DeltaSeconds;
        if (ActiveEvents[Index].TimeRemaining <= 0.f) ActiveEvents.RemoveAt(Index);
    }
    if (AOpenCityGameState* State = GetWorld()->GetGameState<AOpenCityGameState>())
    {
        State->WorldHour = WorldHour;
        State->Weather = Weather;
    }
    if (UOpenCityGameSubsystem* Systems = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>())
    {
        Systems->WorldTime = WorldHour;
        if (IncomeTimer >= MinutesPerDay * 60.f)
        {
            IncomeTimer = 0.f;
            for (const FPropertyRecord& Property : Systems->Properties)
                if (Property.bOwned) Systems->AddMoney(Property.DailyIncome);
        }
    }
}

void AOpenCityWorldDirector::SetWeather(EOpenCityWeather NewWeather)
{
    if (!HasAuthority()) return;
    Weather = NewWeather;
    OnWeatherChanged.Broadcast(Weather);
}

FOpenCityWorldEvent AOpenCityWorldDirector::SpawnRandomEvent()
{
    FOpenCityWorldEvent Event;
    Event.Id = FName(*FString::Printf(TEXT("Event_%08X"), FMath::Rand()));
    Event.Type = static_cast<EOpenCityEventType>(FMath::RandRange(1, static_cast<int32>(EOpenCityEventType::Rescue)));
    Event.Location = GetActorLocation() + FVector(FMath::RandRange(-8000.f, 8000.f), FMath::RandRange(-8000.f, 8000.f), 100.f);
    Event.Reward = FMath::RandRange(250, 2500);
    Event.TimeRemaining = FMath::RandRange(120.f, 420.f);
    ActiveEvents.Add(Event);
    return Event;
}

bool AOpenCityWorldDirector::ResolveEvent(FName EventId, bool bSuccess)
{
    for (int32 Index = 0; Index < ActiveEvents.Num(); ++Index)
    {
        if (ActiveEvents[Index].Id == EventId)
        {
            if (bSuccess)
                if (UOpenCityGameSubsystem* Systems = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>()) Systems->AddMoney(ActiveEvents[Index].Reward);
            ActiveEvents.RemoveAt(Index);
            return true;
        }
    }
    return false;
}

void AOpenCityWorldDirector::OnRep_Weather()
{
    OnWeatherChanged.Broadcast(Weather);
}

AOpenCityCheckpoint::AOpenCityCheckpoint()
{
    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetSphereRadius(240.f);
    Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AOpenCityCheckpoint::HandleOverlap);
    Marker = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Marker"));
    Marker->SetupAttachment(Trigger);
    Marker->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Marker->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    Marker->SetRelativeScale3D(FVector(2.4f, 2.4f, 0.25f));
}

void AOpenCityCheckpoint::HandleOverlap(UPrimitiveComponent* Overlapped, AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (!OtherActor || (!OtherActor->IsA<AOpenCityCharacter>() && !OtherActor->IsA<AOpenCityVehicle>())) return;
    OnReached.Broadcast(CheckpointIndex);
    if (!MissionId.IsNone())
        if (UOpenCityGameSubsystem* Systems = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>()) Systems->CompleteMission(MissionId);
}

AOpenCityPropertyActor::AOpenCityPropertyActor()
{
    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(220.f));
    Trigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    Sign = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Sign"));
    Sign->SetupAttachment(Trigger);
    Sign->SetText(FText::FromString(TEXT("PROPERTY")));
    Sign->SetHorizontalAlignment(EHTA_Center);
    Sign->SetRelativeLocation(FVector(0.f, 0.f, 260.f));
}

bool AOpenCityPropertyActor::Purchase(AActor* Buyer)
{
    if (!Buyer || !Buyer->IsOverlappingActor(this)) return false;
    if (UOpenCityGameSubsystem* Systems = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>()) return Systems->BuyProperty(PropertyId);
    return false;
}

AOpenCityGarage::AOpenCityGarage()
{
    ServiceArea = CreateDefaultSubobject<UBoxComponent>(TEXT("ServiceArea"));
    SetRootComponent(ServiceArea);
    ServiceArea->SetBoxExtent(FVector(500.f, 700.f, 260.f));
    ServiceArea->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
}

bool AOpenCityGarage::RepairVehicle(AActor* VehicleActor)
{
    if (!VehicleActor || !ServiceArea->IsOverlappingActor(VehicleActor)) return false;
    UOpenCityGameSubsystem* Systems = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>();
    UOpenCityHealthComponent* Health = VehicleActor->FindComponentByClass<UOpenCityHealthComponent>();
    if (!Systems || !Health || !Systems->SpendMoney(RepairPrice)) return false;
    Health->Heal(Health->MaxHealth);
    return true;
}

bool AOpenCityGarage::RepaintVehicle(AActor* VehicleActor, FLinearColor Color)
{
    if (!VehicleActor || !ServiceArea->IsOverlappingActor(VehicleActor)) return false;
    UStaticMeshComponent* Mesh = VehicleActor->FindComponentByClass<UStaticMeshComponent>();
    return Mesh != nullptr;
}

AOpenCityTrafficManager::AOpenCityTrafficManager()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickInterval(2.f);
}

void AOpenCityTrafficManager::BeginPlay()
{
    Super::BeginPlay();
    if (HasAuthority()) MaintainPopulation();
}

void AOpenCityTrafficManager::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority()) MaintainPopulation();
}

void AOpenCityTrafficManager::MaintainPopulation()
{
    TrafficActors.RemoveAll([](const AActor* Actor){ return !IsValid(Actor); });
    Pedestrians.RemoveAll([](const AActor* Actor){ return !IsValid(Actor); });
    const FVector Origin = UGameplayStatics::GetPlayerPawn(this, 0) ? UGameplayStatics::GetPlayerPawn(this, 0)->GetActorLocation() : FVector::ZeroVector;

    while (TrafficActors.Num() < TargetVehicleCount)
    {
        const FVector Position = Origin + FVector(FMath::RandRange(-SpawnRadius, SpawnRadius), FMath::RandRange(-SpawnRadius, SpawnRadius), 120.f);
        UClass* VehicleClass = AOpenCityCar::StaticClass();
        const int32 Kind = FMath::RandRange(0, 9);
        if (Kind == 0) VehicleClass = AOpenCityTruck::StaticClass();
        else if (Kind <= 2) VehicleClass = AOpenCityMotorcycle::StaticClass();
        if (AOpenCityVehicle* Vehicle = GetWorld()->SpawnActor<AOpenCityVehicle>(VehicleClass, Position, FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f))) TrafficActors.Add(Vehicle);
        else break;
    }
    while (Pedestrians.Num() < TargetPedestrianCount)
    {
        const FVector Position = Origin + FVector(FMath::RandRange(-SpawnRadius, SpawnRadius), FMath::RandRange(-SpawnRadius, SpawnRadius), 120.f);
        if (AOpenCityNPC* NPC = GetWorld()->SpawnActor<AOpenCityNPC>(Position, FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f))) Pedestrians.Add(NPC);
        else break;
    }
}
