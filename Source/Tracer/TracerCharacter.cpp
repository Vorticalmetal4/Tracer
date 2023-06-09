// Copyright Epic Games, Inc. All Rights Reserved.

#include "TracerCharacter.h"
#include "TracerProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

//////////////////////////////////////////////////////////////////////////
// ATracerCharacter

ATracerCharacter::ATracerCharacter()
	:DashActivated(false),
	HasMove(false),
	HasRotate(false),
	CanMove(true),
	bIsRegressing(false),
	Health(1.f),
	Ammo(10),
	RetrocessPointsNumber(240)
{
	// Character doesnt have a rifle at start
	bHasRifle = false;
	
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
		
	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	//Mesh1P->SetRelativeRotation(FRotator(0.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	LaunchVector = {3000.f, 3000.f, 0.f};

	CooldownFirstAbilitie = 1.f;
	CooldownRemainingFirstAbilitie = 0.f;

	ImpulsesRemaining = MaxImpulses = 3;
	ImpulseReloadTimeRemaining = ImpulseReloadTime = 2.f;

	CurrentDashTime = DashTime = 0.3f;

	TimeTillRegisterData = TimeRegisterData = 0.5f;

	CharacterMovement = GetCharacterMovement();
}

void ATracerCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}

	for (int i = 0; i < RetrocessPointsNumber; i++)
	{
		RetrocessPoints[i].Movement = { 0.f, 0.f };
		RetrocessPoints[i].Rotation = {0.f, 0.f};
		RetrocessPoints[i].Health = Health;
		RetrocessPoints[i].Ammo = Ammo;
	}

}

void ATracerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (CooldownRemainingFirstAbilitie > 0)
		CooldownRemainingFirstAbilitie -= DeltaTime;

	if (ImpulseReloadTimeRemaining > 0 && ImpulsesRemaining < MaxImpulses) //Recharging the first ability
	{
		ImpulseReloadTimeRemaining -= DeltaTime;
		if (ImpulseReloadTimeRemaining <= 0)
		{
			ImpulseReloadTimeRemaining = ImpulseReloadTime;
			ImpulsesRemaining++;
			AddDash();
		}
	}
	
	if (DashActivated)
	{
		if (CurrentDashTime > 0) //Still dashing
		{
			AuxMovement.X = FirstPersonCameraComponent->GetForwardVector().X * 5;
			AuxMovement.Y = FirstPersonCameraComponent->GetForwardVector().Y * 5;
			HasMove = true;
			CurrentDashTime -= DeltaTime;
		}
		else //Dash has ended
		{
			DashActivated = false;
			CharacterMovement->GravityScale = 1.0f;
			CharacterMovement->StopActiveMovement();
			LaunchCharacter({ 0.f, 0.f, -100.f }, true, true);
		}
	}

	if(CanMove) 
		UpdateRetrocessData();
	else if(bIsRegressing) //Second ability is activated
	{
		Retrocess();
		RetrocessIterator--;
		if (RetrocessIterator < 0)
		{
			CanMove = true;
			bIsRegressing = false;
			Regressing(false);
		}
	}

}

//////////////////////////////////////////////////////////////////////////// Input

void ATracerCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		
		//FirstAbilitie
		EnhancedInputComponent->BindAction(FirstAbilityAction, ETriggerEvent::Started, this, &ATracerCharacter::FirstAbility);

		//SecondAbilitie
		EnhancedInputComponent->BindAction(SecondAbilityAction, ETriggerEvent::Started, this, &ATracerCharacter::SecondAbility);

		//Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATracerCharacter::Move);

		//Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATracerCharacter::Look);
	}
}


void ATracerCharacter::Move(const FInputActionValue& Value)
{
	if (CanMove)
	{
		// input is a Vector2D
		FVector2D MovementVector = Value.Get<FVector2D>();

		if (Controller != nullptr)
		{
			// add movement 
			AddMovementInput(GetActorForwardVector(), MovementVector.Y);
			AddMovementInput(GetActorRightVector(), MovementVector.X);
			AuxMovement = MovementVector;
			HasMove = true;
		}
	}
}


void ATracerCharacter::Retrocess()
{
	if (Controller) 
	{
		//Add the inverse inputs based to retrocess the movement
		AddMovementInput(GetActorForwardVector(), RetrocessPoints[RetrocessIterator].Movement.Y * -1);
		AddMovementInput(GetActorRightVector(), RetrocessPoints[RetrocessIterator].Movement.X * -1);

		AddControllerYawInput(RetrocessPoints[RetrocessIterator].Rotation.X * -1);
		AddControllerPitchInput(RetrocessPoints[RetrocessIterator].Rotation.Y * -1);
	}
}

void ATracerCharacter::Look(const FInputActionValue& Value)
{
	if (CanMove)
	{
		// input is a Vector2D
		FVector2D LookAxisVector = Value.Get<FVector2D>();

		if (Controller != nullptr)
		{
			// add yaw and pitch input to controller
			AddControllerYawInput(LookAxisVector.X);
			AddControllerPitchInput(LookAxisVector.Y);
			AuxRotation = LookAxisVector;
			HasRotate = true;
		}
	}
}

void ATracerCharacter::SetHasRifle(bool bNewHasRifle)
{
	bHasRifle = bNewHasRifle;
}

bool ATracerCharacter::GetHasRifle()
{
	return bHasRifle;
}

void ATracerCharacter::FirstAbility() //Dash
{
	if (CooldownRemainingFirstAbilitie <= 0 && ImpulsesRemaining >= 1)
	{
		DiscountDash();
		ImpulsesRemaining--;
		ImpulseReloadTimeRemaining = ImpulseReloadTime;
		CooldownRemainingFirstAbilitie = CooldownFirstAbilitie;
		CharacterMovement->GravityScale = 0.f;
		LaunchCharacter(LaunchVector * FirstPersonCameraComponent->GetForwardVector(), true, true);
		CurrentDashTime = DashTime;
		DashActivated = true;
	}
}

void ATracerCharacter::SecondAbility() //Retrocess
{
	CanMove = false;
	RetrocessIterator = RetrocessPointsNumber - 1;
	bIsRegressing = true;
	Regressing(true);
}

void ATracerCharacter::UpdateRetrocessData() //Save the changes in the states
{
	for (int i = 0; i < RetrocessPointsNumber - 1; i++)
	{
		RetrocessPoints[i].Movement = RetrocessPoints[i + 1].Movement;
		RetrocessPoints[i].Rotation = RetrocessPoints[i + 1].Rotation;
		RetrocessPoints[i].Health = RetrocessPoints[i + 1].Health;
		RetrocessPoints[i].Ammo = RetrocessPoints[i + 1].Ammo;
	}

	if (!HasMove)
		AuxMovement = { 0.f, 0.f };
	else
		HasMove = false;

	if (!HasRotate)
		AuxRotation = { 0.f, 0.f };
	else
		HasRotate = false;

	RetrocessPoints[RetrocessPointsNumber - 1].Movement = AuxMovement;
	RetrocessPoints[RetrocessPointsNumber - 1].Rotation = AuxRotation;
	RetrocessPoints[RetrocessPointsNumber - 1].Health = Health;
	RetrocessPoints[RetrocessPointsNumber - 1].Ammo = Ammo;
}