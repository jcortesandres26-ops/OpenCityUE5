#include "OpenCityPresentation.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

UOpenCityAudioDirectorComponent::UOpenCityAudioDirectorComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UOpenCityAudioDirectorComponent::Play(USoundBase* Sound, float Volume)
{
    if (!Sound || !GetWorld()) return;
    if (ActiveAudio) ActiveAudio->FadeOut(0.4f, 0.f);
    ActiveAudio = UGameplayStatics::SpawnSound2D(GetWorld(), Sound, Volume, 1.f, 0.f, nullptr, true, false);
}

void UOpenCityAudioDirectorComponent::PlayCityAmbience() { Play(CityAmbience, 0.55f); }
void UOpenCityAudioDirectorComponent::PlayCombatMusic() { Play(CombatMusic, 0.8f); }
void UOpenCityAudioDirectorComponent::PlayMissionMusic() { Play(MissionMusic, 0.75f); }
void UOpenCityAudioDirectorComponent::StopMusic(float FadeSeconds)
{
    if (ActiveAudio) ActiveAudio->FadeOut(FMath::Max(0.f, FadeSeconds), 0.f);
}

AOpenCityCinematicDirector::AOpenCityCinematicDirector()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = false;
}

void AOpenCityCinematicDirector::PlayShot(AActor* Target, float Duration, float BlendTime)
{
    if (!Target) return;
    CameraTarget = Target;
    RemainingTime = FMath::Max(0.1f, Duration);
    bPlaying = true;
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        PC->SetViewTargetWithBlend(Target, FMath::Max(0.f, BlendTime));
        PC->SetCinematicMode(true, true, true, true, true);
    }
}

void AOpenCityCinematicDirector::StopShot(float BlendTime)
{
    if (!bPlaying) return;
    bPlaying = false;
    RemainingTime = 0.f;
    if (APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0))
    {
        if (APawn* Pawn = PC->GetPawn()) PC->SetViewTargetWithBlend(Pawn, FMath::Max(0.f, BlendTime));
        PC->SetCinematicMode(false, true, true, true, true);
    }
    CameraTarget = nullptr;
}

void AOpenCityCinematicDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bPlaying) return;
    RemainingTime -= DeltaSeconds;
    if (RemainingTime <= 0.f) StopShot();
}

UOpenCityMotionComponent::UOpenCityMotionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UOpenCityMotionComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner())
    {
        PreviousLocation = GetOwner()->GetActorLocation();
        if (UStaticMeshComponent* Visual = GetOwner()->FindComponentByClass<UStaticMeshComponent>())
            StartLocation = Visual->GetRelativeLocation();
    }
}

void UOpenCityMotionComponent::TickComponent(float DeltaSeconds, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaSeconds, TickType, ThisTickFunction);
    AActor* Owner = GetOwner();
    if (!Owner || DeltaSeconds <= 0.f) return;
    UStaticMeshComponent* Visual = Owner->FindComponentByClass<UStaticMeshComponent>();
    if (!Visual) return;
    const FVector Location = Owner->GetActorLocation();
    const FVector Velocity = (Location - PreviousLocation) / DeltaSeconds;
    PreviousLocation = Location;
    const float Speed = Velocity.Size2D();
    Phase += DeltaSeconds * BobSpeed * FMath::Clamp(Speed / 300.f, 0.f, 2.f);
    const float Bob = FMath::Sin(Phase) * BobHeight * FMath::Clamp(Speed / 300.f, 0.f, 1.f);
    const float Lean = FMath::Clamp(Velocity.Y / 500.f, -1.f, 1.f) * LeanDegrees;
    Visual->SetRelativeLocation(StartLocation + FVector(0.f, 0.f, Bob));
    Visual->SetRelativeRotation(FRotator(0.f, 0.f, Lean));
}
