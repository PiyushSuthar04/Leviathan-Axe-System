#include "LeviathanAxe.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

ALeviathanAxe::ALeviathanAxe()
{
    PrimaryActorTick.bCanEverTick = true;
}

void ALeviathanAxe::BeginPlay()
{
    Super::BeginPlay();

    CharacterRef =
        UGameplayStatics::GetPlayerCharacter(this, 0);

    FindStickPoint();
}
void ALeviathanAxe::ThrowAxe(const FVector& InThrowDirection)
{
    if (InThrowDirection.IsNearlyZero())
    {
        return;
    }

    // Remove axe from character hand/socket
    DetachFromActor(
        FDetachmentTransformRules::KeepWorldTransform
    );

    ThrowDirection =
        InThrowDirection.GetSafeNormal();

    SetActorRotation(
        ThrowDirection.Rotation()
    );

    // Reset throw state
    bManualRecall = false;
    bManualThrow = true;
    bShouldSpin = true;
    bIsThrown = false;
    bIsStuck = false;
}

void ALeviathanAxe::UpdateThrowMovement(float DeltaTime)
{
    if (!bManualThrow)
    {
        return;
    }

    const FVector CurrentLocation =
        GetActorLocation();

    const FVector Movement =
        ThrowDirection *
        ThrowSpeed *
        DeltaTime;

    const FVector NewLocation =
        CurrentLocation + Movement;

    FHitResult HitResult;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    if (CharacterRef)
    {
        QueryParams.AddIgnoredActor(CharacterRef);
    }

    FCollisionObjectQueryParams ObjectParams;

    ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

    const bool bHit =
        GetWorld()->SweepSingleByObjectType(
            HitResult,
            CurrentLocation,
            NewLocation,
            GetActorQuat(),
            ObjectParams,
            FCollisionShape::MakeSphere(10.0f),
            QueryParams
        );

    if (bHit)
    {
        bManualThrow = false;

        PendingHitResult = HitResult;

        OnAxeImpactSound();

        GetWorldTimerManager().SetTimer(
            StickDelayTimer,
            this,
            &ALeviathanAxe::CompleteStickAxe,
            StickDelay,
            false
        );

        return;
    }

    SetActorLocation(
        NewLocation,
        false,
        nullptr,
        ETeleportType::None
    );
}
void ALeviathanAxe::StickAxe(const FHitResult& HitResult)
{
    if (!StickPoint)
    {
        return;
    }

    const FVector ImpactPoint =
        HitResult.ImpactPoint;

    const FVector ImpactNormal =
        HitResult.ImpactNormal.GetSafeNormal();

    bManualThrow = false;
    bShouldSpin = false;
    bIsThrown = true;
    bIsStuck = true;
    
    const bool bIsGround =
        ImpactNormal.Z > 0.7f;

    if (bIsGround)
    {

        const FVector GroundUp =
            ImpactNormal;

        FVector GroundForward =
            FVector::VectorPlaneProject(
                ThrowDirection,
                GroundUp
            ).GetSafeNormal();

        // Safety fallback
        if (GroundForward.IsNearlyZero())
        {
            GroundForward =
                FVector::ForwardVector;
        }

        const float GroundYaw =
            FMath::RadiansToDegrees(
                FMath::Atan2(
                    GroundForward.Y,
                    GroundForward.X
                )
            );

        const FRotator BaseRotation(
            -90.0f,       // Pitch
            GroundYaw,    // Yaw
            0.0f          // Roll
        );

        FQuat AxeRotation =
            BaseRotation.Quaternion();

        const FVector GroundRight =
            FVector::CrossProduct(
                GroundUp,
                GroundForward
            ).GetSafeNormal();

        const float GroundTiltAngle =
            15.0f;

        const FQuat TiltRotation =
            FQuat(
                GroundRight,
                FMath::DegreesToRadians(
                    GroundTiltAngle
                )
            );

        AxeRotation =
            TiltRotation * AxeRotation;

        SetActorRotation(
            AxeRotation
        );

        const FVector NewStickPointLocation =
            StickPoint->GetComponentLocation();


        // Move Axe so StickPoint reaches ImpactPoint
        const FVector PositionCorrection =
            ImpactPoint -
            NewStickPointLocation;

        const float SurfaceOffset =
            2.0f;

        const FVector SurfaceOffsetVector =
            GroundUp *
            SurfaceOffset;

        const FVector FinalLocation =
            GetActorLocation() +
            PositionCorrection +
            SurfaceOffsetVector;


        SetActorLocation(
            FinalLocation,
            false,
            nullptr,
            ETeleportType::TeleportPhysics
        );
    }

    else
    {
        const FRotator SurfaceRotation =
            ImpactNormal.Rotation();

        const FRotator OffsetRotation(
            0.0f,
            180.0f,
            0.0f
        );

        const FRotator CombinedRotation =
            UKismetMathLibrary::ComposeRotators(
                SurfaceRotation,
                OffsetRotation
            );

        const FVector ForwardVector =
            CombinedRotation.Vector();

        const FVector RotationAxis =
            FVector::CrossProduct(
                ImpactNormal,
                FVector::UpVector
            ).GetSafeNormal();

        FVector RotatedForward =
            ForwardVector;

        if (!RotationAxis.IsNearlyZero())
        {
            RotatedForward =
                ForwardVector.RotateAngleAxis(
                    25.0f,
                    RotationAxis
                );
        }

        const FRotator FinalRotation =
            RotatedForward.Rotation();

        SetActorRotation(
            FinalRotation
        );
        const FVector StickPointLocation =
            StickPoint->GetComponentLocation();

        const FVector CurrentLocation =
            GetActorLocation();

        const float SurfaceOffset =
            2.0f;

        const FVector SurfaceOffsetVector =
            ImpactNormal *
            SurfaceOffset;

        const FVector FinalLocation =
            CurrentLocation +
            (ImpactPoint - StickPointLocation) +
            SurfaceOffsetVector;


        SetActorLocation(
            FinalLocation,
            false,
            nullptr,
            ETeleportType::TeleportPhysics
        );
    }
}

void ALeviathanAxe::UpdateSpin(float DeltaTime)
{
    if (!bShouldSpin)
    {
        return;
    }

    const float RotationAmount =
        SpinSpeed * DeltaTime;

    // Blueprint was rotating local Pitch/Y axis
    AddActorLocalRotation(
        FRotator(
            -RotationAmount,
            0.0f,
            0.0f
        )
    );
}

void ALeviathanAxe::RecallAxe()
{
    if (!CharacterRef)
    {
        return;
    }

    // If axe is stuck, shake first
    if (bIsStuck)
    {
        bManualThrow = false;
        bManualRecall = false;
        bShouldSpin = false;

        bIsShaking = true;

        ShakeStartRotation =
            GetActorRotation();

        GetWorldTimerManager().SetTimer(
            RecallDelayTimer,
            this,
            &ALeviathanAxe::StartRecallAfterDelay,
            RecallDelay,
            false
        );

        return;
    }

    // If axe is flying in air,
    // recall immediately
    if (bManualThrow)
    {
        bManualThrow = false;

        StartRecallAfterDelay();
    }
}
void ALeviathanAxe::StartRecallAfterDelay()
{
    bIsShaking = false;
    OnRecallSoundStart();

    // Get current axe position
    RecallStartLocation =
        GetActorLocation();
    RecallProgress = 0.0f;

    USkeletalMeshComponent* CharacterMesh =
        CharacterRef->GetMesh();

    if (!CharacterMesh)
    {
        return;
    }

    const FVector HandLocation =
        CharacterMesh->GetSocketLocation(
            TEXT("AxeSocket")
        );

    // Direction from axe to hand
    const FVector ToHand =
        (HandLocation - RecallStartLocation)
        .GetSafeNormal();

    // Create a right-side direction
    const FVector RightDirection =
        CharacterRef->GetActorRightVector();

    // Control point in the middle of the path
    const FVector MidPoint =
        (RecallStartLocation + HandLocation) * 0.5f;

    RecallControlPoint =
        MidPoint +
        RightDirection *
        RecallCurveStrength;

    // Start recall
    bManualRecall = true;
    bShouldSpin = true;
    bIsThrown = true;
}
void ALeviathanAxe::UpdateAxeShake(float DeltaTime)
{
    if (!bIsShaking)
    {
        return;
    }

    const float Time =
        GetWorld()->GetTimeSeconds();

    const float ShakeAmount =
        FMath::Sin(Time * ShakeSpeed) * ShakeAngle;

    FRotator NewRotation =
        ShakeStartRotation;

    NewRotation.Roll += ShakeAmount;

    SetActorRotation(NewRotation);
}
void ALeviathanAxe::UpdateRecallMovement(float DeltaTime)
{
    if (!bManualRecall || !CharacterRef)
    {
        return;
    }

    USkeletalMeshComponent* CharacterMesh =
        CharacterRef->GetMesh();

    if (!CharacterMesh)
    {
        return;
    }

    const FVector HandLocation =
        CharacterMesh->GetSocketLocation(
            TEXT("AxeSocket")
        );

    const float Distance =
        FVector::Dist(
            RecallStartLocation,
            HandLocation
        );

    if (Distance <= 1.0f)
    {
        FinishRecall();
        return;
    }

    // Move along curve based on speed
    RecallProgress +=
        (RecallSpeed * DeltaTime) /
        Distance;

    RecallProgress =
        FMath::Clamp(
            RecallProgress,
            0.0f,
            1.0f
        );

    const float T =
        RecallProgress;

    const float OneMinusT =
        1.0f - T;

    // Quadratic Bezier curve
    const FVector CurveLocation =
        (OneMinusT * OneMinusT) *
        RecallStartLocation
        +
        (2.0f * OneMinusT * T) *
        RecallControlPoint
        +
        (T * T) *
        HandLocation;

    SetActorLocation(
        CurveLocation,
        false,
        nullptr,
        ETeleportType::None
    );

    if (RecallProgress >= 1.0f)
    {
        FinishRecall();
    }
}

void ALeviathanAxe::FinishRecall()
{
    if (!CharacterRef)
    {
        return;
    }

    USkeletalMeshComponent* CharacterMesh =
        CharacterRef->GetMesh();

    if (!CharacterMesh)
    {
        return;
    }

    AttachToComponent(
        CharacterMesh,
        FAttachmentTransformRules::
        SnapToTargetNotIncludingScale,
        TEXT("AxeSocket")
    );

    bManualRecall = false;
    bManualThrow = false;
    bShouldSpin = false;
    bIsThrown = false;
    bIsStuck = false;

    OnAxeEquipped();
}

void ALeviathanAxe::FindStickPoint()
{
    TArray<USceneComponent*> Components;

    GetComponents<USceneComponent>(
        Components
    );

    for (USceneComponent* Component : Components)
    {
        if (!Component)
        {
            continue;
        }

        const FString ComponentName =
            Component->GetName();

        if (ComponentName.Contains(
            TEXT("stick point")))
        {
            StickPoint = Component;
            return;
        }
    }
}
void ALeviathanAxe::CompleteStickAxe()
{
    StickAxe(PendingHitResult);
}

void ALeviathanAxe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    UpdateThrowMovement(DeltaTime);
    UpdateRecallMovement(DeltaTime);
    UpdateAxeShake(DeltaTime);
    UpdateSpin(DeltaTime);
}