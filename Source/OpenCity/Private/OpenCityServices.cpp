#include "OpenCityServices.h"
#include "AIController.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "OpenCityCore.h"
#include "Subsystems/SubsystemCollection.h"

void UOpenCityServiceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (Businesses.IsEmpty())
    {
        const TCHAR* BusinessKinds[] = { TEXT("Workshop"), TEXT("Restaurant"), TEXT("Club"), TEXT("Taxi"), TEXT("Marina"), TEXT("AirCargo") };
        for (int32 Index = 0; Index < 18; ++Index)
        {
            FOpenCityBusinessData Business;
            Business.Id = FName(*FString::Printf(TEXT("%s_%02d"), BusinessKinds[Index % 6], Index + 1));
            Business.Price = 18000 + Index * 8500;
            Business.DailyIncome = 350 + Index * 110;
            Businesses.Add(Business);
        }
    }
    if (Territories.IsEmpty())
    {
        const TCHAR* Districts[] = { TEXT("Downtown"), TEXT("Beach"), TEXT("Hills"), TEXT("Industrial"), TEXT("Desert"), TEXT("Harbor"), TEXT("Airport"), TEXT("OldTown") };
        for (int32 Index = 0; Index < 8; ++Index)
        {
            FOpenCityTerritoryData Territory;
            Territory.Id = Districts[Index];
            Territory.Center = FVector((Index % 4) * 7000.f - 10500.f, (Index / 4) * 10000.f - 5000.f, 0.f);
            Territories.Add(Territory);
        }
    }
    SetFactionRelation(TEXT("Police"), TEXT("Civilian"), 25);
    SetFactionRelation(TEXT("Northside"), TEXT("Southside"), -100);
    SetFactionRelation(TEXT("Police"), TEXT("Northside"), -60);
    SetFactionRelation(TEXT("Police"), TEXT("Southside"), -60);
}

FString UOpenCityServiceSubsystem::MakeRelationKey(FName First, FName Second) const
{
    const FString A = First.ToString();
    const FString B = Second.ToString();
    return A < B ? A + TEXT("|") + B : B + TEXT("|") + A;
}

void UOpenCityServiceSubsystem::SetFactionRelation(FName First, FName Second, int32 Relation)
{
    FactionRelations.Add(MakeRelationKey(First, Second), FMath::Clamp(Relation, -100, 100));
}

int32 UOpenCityServiceSubsystem::GetFactionRelation(FName First, FName Second) const
{
    if (First == Second) return 100;
    if (const int32* Relation = FactionRelations.Find(MakeRelationKey(First, Second))) return *Relation;
    return 0;
}

bool UOpenCityServiceSubsystem::BuyBusiness(FName BusinessId)
{
    UOpenCityGameSubsystem* Game = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>();
    if (!Game) return false;
    for (FOpenCityBusinessData& Business : Businesses)
    {
        if (Business.Id == BusinessId && !Business.bOwned && Game->SpendMoney(Business.Price))
        {
            Business.bOwned = true;
            return true;
        }
    }
    return false;
}

int32 UOpenCityServiceSubsystem::CollectBusinessIncome()
{
    int32 Income = 0;
    for (const FOpenCityBusinessData& Business : Businesses)
        if (Business.bOwned) Income += FMath::RoundToInt(Business.DailyIncome * Business.Condition / 100.f);
    if (UOpenCityGameSubsystem* Game = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>()) Game->AddMoney(Income);
    return Income;
}

bool UOpenCityServiceSubsystem::AddTerritoryInfluence(FName TerritoryId, FName Faction, float Amount)
{
    for (FOpenCityTerritoryData& Territory : Territories)
    {
        if (Territory.Id != TerritoryId) continue;
        if (Territory.OwnerFaction == Faction || Territory.OwnerFaction == TEXT("Neutral")) Territory.Influence += Amount;
        else Territory.Influence -= Amount;
        if (Territory.Influence >= 100.f)
        {
            Territory.OwnerFaction = Faction;
            Territory.Influence = 0.f;
            OnTerritoryChanged.Broadcast(Territory.Id, Territory.OwnerFaction);
        }
        return true;
    }
    return false;
}

AOpenCityMissionDirector::AOpenCityMissionDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickInterval(0.2f);
    bReplicates = true;
}

bool AOpenCityMissionDirector::BeginMission(FName MissionId)
{
    if (!HasAuthority() || !ActiveMission.IsNone()) return false;
    UOpenCityGameSubsystem* Game = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>();
    if (!Game || !Game->StartMission(MissionId)) return false;
    ActiveMission = MissionId;
    ObjectiveIndex = 0;
    for (const FOpenCityMission& Mission : Game->Missions)
        if (Mission.Id == MissionId) { ActiveTarget = Mission.Checkpoint; break; }
    OnMissionStarted.Broadcast(ActiveMission);
    return true;
}

void AOpenCityMissionDirector::AdvanceObjective(FVector NewTarget)
{
    if (!HasAuthority() || ActiveMission.IsNone()) return;
    ++ObjectiveIndex;
    ActiveTarget = NewTarget;
}

void AOpenCityMissionDirector::FailActiveMission()
{
    if (!HasAuthority() || ActiveMission.IsNone()) return;
    if (UOpenCityGameSubsystem* Game = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>()) Game->FailMission(ActiveMission);
    OnMissionFailed.Broadcast(ActiveMission);
    ActiveMission = NAME_None;
}

void AOpenCityMissionDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority() || ActiveMission.IsNone()) return;
    APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!Player || FVector::DistSquared(Player->GetActorLocation(), ActiveTarget) > FMath::Square(TargetRadius)) return;
    if (UOpenCityGameSubsystem* Game = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>())
    {
        const FName Completed = ActiveMission;
        if (Game->CompleteMission(ActiveMission)) OnMissionCompleted.Broadcast(Completed);
    }
    ActiveMission = NAME_None;
}

AOpenCityPoliceDirector::AOpenCityPoliceDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    SetActorTickInterval(1.f);
    bReplicates = true;
}

void AOpenCityPoliceDirector::BeginPlay()
{
    Super::BeginPlay();
    CurrentSuspect = UGameplayStatics::GetPlayerPawn(this, 0);
}

void AOpenCityPoliceDirector::ReportCrime(AActor* Suspect, int32 Severity)
{
    if (!HasAuthority()) return;
    CurrentSuspect = Suspect;
    if (UOpenCityGameSubsystem* Game = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>()) Game->AddWantedLevel(FMath::Max(1, Severity));
    DecayTimer = 0.f;
    UpdateDispatch();
}

void AOpenCityPoliceDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!HasAuthority()) return;
    UOpenCityGameSubsystem* Game = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>();
    if (!Game) return;
    Units.RemoveAll([](const AOpenCityPoliceNPC* Unit){ return !IsValid(Unit); });
    if (Game->WantedLevel <= 0) return;
    DecayTimer += GetActorTickInterval();
    if (DecayTimer >= WantedDecaySeconds)
    {
        DecayTimer = 0.f;
        Game->AddWantedLevel(-1);
    }
    UpdateDispatch();
}

void AOpenCityPoliceDirector::UpdateDispatch()
{
    UOpenCityGameSubsystem* Game = GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>();
    if (!Game || !CurrentSuspect) return;
    const int32 DesiredUnits = Game->WantedLevel * UnitsPerWantedLevel;
    while (Units.Num() < DesiredUnits)
    {
        const float Angle = FMath::RandRange(0.f, UE_TWO_PI);
        const FVector Offset(FMath::Cos(Angle) * DispatchRadius, FMath::Sin(Angle) * DispatchRadius, 180.f);
        AOpenCityPoliceNPC* Unit = GetWorld()->SpawnActor<AOpenCityPoliceNPC>(CurrentSuspect->GetActorLocation() + Offset, FRotator::ZeroRotator);
        if (!Unit) break;
        Units.Add(Unit);
        if (AAIController* Controller = Cast<AAIController>(Unit->GetController())) Controller->MoveToActor(CurrentSuspect, 180.f);
    }
}