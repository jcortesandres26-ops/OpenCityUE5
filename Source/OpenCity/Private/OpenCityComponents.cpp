#include "OpenCityComponents.h"
#include "AIController.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "OpenCityCore.h"

UOpenCityHealthComponent::UOpenCityHealthComponent()
{
    SetIsReplicatedByDefault(true);
}

void UOpenCityHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UOpenCityHealthComponent, MaxHealth);
    DOREPLIFETIME(UOpenCityHealthComponent, Health);
    DOREPLIFETIME(UOpenCityHealthComponent, Armor);
}

float UOpenCityHealthComponent::ApplyOpenCityDamage(float Amount, EOpenCityDamageKind Kind, AActor* InstigatorActor)
{
    if (Amount <= 0.f || Health <= 0.f || !GetOwner() || !GetOwner()->HasAuthority()) return 0.f;
    const float OldHealth = Health;
    const float ArmorFactor = Kind == EOpenCityDamageKind::Drowning ? 0.f : 0.65f;
    const float Absorbed = FMath::Min(Armor, Amount * ArmorFactor);
    Armor -= Absorbed;
    Health = FMath::Clamp(Health - (Amount - Absorbed), 0.f, MaxHealth);
    OnHealthChanged.Broadcast(Health, Armor);
    if (Health <= 0.f && OldHealth > 0.f) OnDeath.Broadcast(GetOwner());
    return OldHealth - Health;
}

void UOpenCityHealthComponent::Heal(float Amount)
{
    if (Amount <= 0.f || !GetOwner() || !GetOwner()->HasAuthority()) return;
    Health = FMath::Clamp(Health + Amount, 0.f, MaxHealth);
    OnHealthChanged.Broadcast(Health, Armor);
}

void UOpenCityHealthComponent::AddArmor(float Amount)
{
    if (Amount <= 0.f || !GetOwner() || !GetOwner()->HasAuthority()) return;
    Armor = FMath::Clamp(Armor + Amount, 0.f, 100.f);
    OnHealthChanged.Broadcast(Health, Armor);
}

void UOpenCityHealthComponent::OnRep_Vitals()
{
    OnHealthChanged.Broadcast(Health, Armor);
}

UOpenCityInventoryComponent::UOpenCityInventoryComponent()
{
    SetIsReplicatedByDefault(true);
}

void UOpenCityInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UOpenCityInventoryComponent, MaxWeight);
    DOREPLIFETIME(UOpenCityInventoryComponent, Items);
}

bool UOpenCityInventoryComponent::AddItem(FName ItemId, int32 Quantity, float WeightEach)
{
    if (ItemId.IsNone() || Quantity <= 0 || !GetOwner() || !GetOwner()->HasAuthority()) return false;
    if (GetCurrentWeight() + Quantity * FMath::Max(0.f, WeightEach) > MaxWeight) return false;
    for (FOpenCityInventoryStack& Stack : Items)
    {
        if (Stack.ItemId == ItemId)
        {
            Stack.Quantity += Quantity;
            return true;
        }
    }
    FOpenCityInventoryStack Stack;
    Stack.ItemId = ItemId;
    Stack.Quantity = Quantity;
    Stack.WeightEach = FMath::Max(0.f, WeightEach);
    Items.Add(Stack);
    return true;
}

bool UOpenCityInventoryComponent::RemoveItem(FName ItemId, int32 Quantity)
{
    if (Quantity <= 0 || !GetOwner() || !GetOwner()->HasAuthority()) return false;
    for (int32 Index = 0; Index < Items.Num(); ++Index)
    {
        if (Items[Index].ItemId == ItemId && Items[Index].Quantity >= Quantity)
        {
            Items[Index].Quantity -= Quantity;
            if (Items[Index].Quantity == 0) Items.RemoveAt(Index);
            return true;
        }
    }
    return false;
}

int32 UOpenCityInventoryComponent::GetItemCount(FName ItemId) const
{
    for (const FOpenCityInventoryStack& Stack : Items)
        if (Stack.ItemId == ItemId) return Stack.Quantity;
    return 0;
}

float UOpenCityInventoryComponent::GetCurrentWeight() const
{
    float Weight = 0.f;
    for (const FOpenCityInventoryStack& Stack : Items) Weight += Stack.Quantity * Stack.WeightEach;
    return Weight;
}

UOpenCityWeaponComponent::UOpenCityWeaponComponent()
{
    SetIsReplicatedByDefault(true);
}

void UOpenCityWeaponComponent::BeginPlay()
{
    Super::BeginPlay();
    if (Arsenal.IsEmpty())
    {
        FOpenCityWeaponSpec Unarmed;
        Unarmed.Id = TEXT("Unarmed");
        Unarmed.DisplayName = FText::FromString(TEXT("Unarmed"));
        Unarmed.Damage = 8.f;
        Unarmed.Range = 180.f;
        Unarmed.FireInterval = 0.55f;
        Unarmed.MagazineSize = 1;
        Unarmed.ReserveAmmo = 0;
        Arsenal.Add(Unarmed);

        FOpenCityWeaponSpec Pistol;
        Pistol.Id = TEXT("Pistol");
        Pistol.DisplayName = FText::FromString(TEXT("9mm Pistol"));
        Arsenal.Add(Pistol);
    }
    EquipWeapon(FMath::Clamp(CurrentWeaponIndex, 0, Arsenal.Num() - 1));
}

void UOpenCityWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UOpenCityWeaponComponent, CurrentWeaponIndex);
    DOREPLIFETIME(UOpenCityWeaponComponent, MagazineAmmo);
    DOREPLIFETIME(UOpenCityWeaponComponent, ReserveAmmo);
}

FOpenCityWeaponSpec UOpenCityWeaponComponent::GetCurrentWeapon() const
{
    return Arsenal.IsValidIndex(CurrentWeaponIndex) ? Arsenal[CurrentWeaponIndex] : FOpenCityWeaponSpec();
}

bool UOpenCityWeaponComponent::Fire()
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !Arsenal.IsValidIndex(CurrentWeaponIndex)) return false;
    const FOpenCityWeaponSpec Spec = Arsenal[CurrentWeaponIndex];
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now - LastFireTime < Spec.FireInterval) return false;
    if (Spec.Id != TEXT("Unarmed") && MagazineAmmo <= 0) return false;

    LastFireTime = Now;
    if (Spec.Id != TEXT("Unarmed")) --MagazineAmmo;

    FVector Start;
    FRotator ViewRotation;
    if (const APawn* Pawn = Cast<APawn>(GetOwner()))
    {
        if (const AController* Controller = Pawn->GetController()) Controller->GetPlayerViewPoint(Start, ViewRotation);
        else { Start = Pawn->GetActorLocation(); ViewRotation = Pawn->GetActorRotation(); }
    }
    else { Start = GetOwner()->GetActorLocation(); ViewRotation = GetOwner()->GetActorRotation(); }

    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(OpenCityWeaponTrace), true, GetOwner());
    if (GetWorld()->LineTraceSingleByChannel(Hit, Start, Start + ViewRotation.Vector() * Spec.Range, ECC_Visibility, Params))
    {
        if (UOpenCityHealthComponent* HealthComponent = Hit.GetActor()->FindComponentByClass<UOpenCityHealthComponent>())
            HealthComponent->ApplyOpenCityDamage(Spec.Damage, EOpenCityDamageKind::Bullet, GetOwner());
    }
    OnWeaponChanged.Broadcast(Spec.Id, MagazineAmmo);
    return true;
}

bool UOpenCityWeaponComponent::Reload()
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !Arsenal.IsValidIndex(CurrentWeaponIndex)) return false;
    const FOpenCityWeaponSpec Spec = Arsenal[CurrentWeaponIndex];
    const int32 Needed = Spec.MagazineSize - MagazineAmmo;
    const int32 Loaded = FMath::Min(Needed, ReserveAmmo);
    if (Loaded <= 0) return false;
    MagazineAmmo += Loaded;
    ReserveAmmo -= Loaded;
    OnWeaponChanged.Broadcast(Spec.Id, MagazineAmmo);
    return true;
}

bool UOpenCityWeaponComponent::EquipWeapon(int32 Index)
{
    if (!Arsenal.IsValidIndex(Index)) return false;
    CurrentWeaponIndex = Index;
    const FOpenCityWeaponSpec Spec = Arsenal[Index];
    MagazineAmmo = Spec.MagazineSize;
    ReserveAmmo = Spec.ReserveAmmo;
    OnWeaponChanged.Broadcast(Spec.Id, MagazineAmmo);
    return true;
}

void UOpenCityWeaponComponent::NextWeapon()
{
    if (!Arsenal.IsEmpty()) EquipWeapon((CurrentWeaponIndex + 1) % Arsenal.Num());
}

void UOpenCityWeaponComponent::OnRep_WeaponState()
{
    OnWeaponChanged.Broadcast(GetCurrentWeapon().Id, MagazineAmmo);
}

bool UOpenCityDialogueComponent::StartDialogue(FName NodeId)
{
    if (!Nodes.ContainsByPredicate([NodeId](const FOpenCityDialogueNode& Node){ return Node.Id == NodeId; })) return false;
    CurrentNode = NodeId;
    OnDialogueChanged.Broadcast(CurrentNode);
    return true;
}

bool UOpenCityDialogueComponent::Choose(int32 ChoiceIndex)
{
    const FOpenCityDialogueNode Node = GetCurrentDialogue();
    if (!Node.Choices.IsValidIndex(ChoiceIndex)) return false;
    const FOpenCityDialogueChoice& Choice = Node.Choices[ChoiceIndex];
    if (Choice.MoneyCost > 0)
    {
        UOpenCityGameSubsystem* Systems = GetWorld()->GetGameInstance()->GetSubsystem<UOpenCityGameSubsystem>();
        if (!Systems || !Systems->SpendMoney(Choice.MoneyCost)) return false;
    }
    CurrentNode = Choice.NextNode;
    OnDialogueChanged.Broadcast(CurrentNode);
    return true;
}

FOpenCityDialogueNode UOpenCityDialogueComponent::GetCurrentDialogue() const
{
    for (const FOpenCityDialogueNode& Node : Nodes)
        if (Node.Id == CurrentNode) return Node;
    return FOpenCityDialogueNode();
}

UOpenCityRoutineComponent::UOpenCityRoutineComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UOpenCityRoutineComponent::BeginPlay()
{
    Super::BeginPlay();
    if (Schedule.IsEmpty())
    {
        auto AddEntry = [this](float Start, float End, EOpenCityRoutineTask Task, const FVector& Destination)
        {
            FOpenCityRoutineEntry Entry;
            Entry.StartHour = Start;
            Entry.EndHour = End;
            Entry.Task = Task;
            Entry.Destination = Destination;
            Schedule.Add(Entry);
        };
        AddEntry(6.f, 8.f, EOpenCityRoutineTask::Walk, FVector(1000.f, 0.f, 0.f));
        AddEntry(8.f, 17.f, EOpenCityRoutineTask::Work, FVector(2500.f, 2500.f, 0.f));
        AddEntry(17.f, 22.f, EOpenCityRoutineTask::Shop, FVector(-1500.f, 1000.f, 0.f));
        AddEntry(22.f, 24.f, EOpenCityRoutineTask::Sleep, FVector::ZeroVector);
        AddEntry(0.f, 6.f, EOpenCityRoutineTask::Sleep, FVector::ZeroVector);
    }
}

void UOpenCityRoutineComponent::UpdateRoutine(float WorldHour)
{
    for (const FOpenCityRoutineEntry& Entry : Schedule)
    {
        if (WorldHour >= Entry.StartHour && WorldHour < Entry.EndHour)
        {
            CurrentTask = Entry.Task;
            CurrentDestination = Entry.Destination;
            if (APawn* Pawn = Cast<APawn>(GetOwner()))
                if (AAIController* Controller = Cast<AAIController>(Pawn->GetController()))
                    Controller->MoveToLocation(CurrentDestination, 120.f);
            return;
        }
    }
    CurrentTask = EOpenCityRoutineTask::Idle;
}
