#include "MyGameInstance.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Winter2/MyLib.h"
#include "Winter2/Winter2.h"

UMyGameInstance* UMyGameInstance::Get = nullptr;

void UMyGameInstance::Init()
{
	Super::Init();
	
	Get = this;

	m_ZoneMoveManager = NewObject<ULevelManager>(this);

	m_bIsPlayerWin = false;
}

void UMyGameInstance::LoadComplete(const float LoadTime, const FString& MapName)
{
	SetPlayerWinFalse();
	
	if(MapName!=TEXT("InitLevel"))
	{
		if(m_ZoneMoveManager->IsGameStart())
		{
			m_ZoneMoveManager->OnOpenWorldLevelComplete();
		}
	}
}

void UMyGameInstance::StartGame()
{
	OpenMap(TEXT("WinterLevel"));

	UKismetSystemLibrary::ControlScreensaver(false);
}

void UMyGameInstance::OpenMap(FName mapName)
{
	m_ZoneMoveManager->OpenMyLevel(mapName);
}

void UMyGameInstance::ExitGame()
{
	UKismetSystemLibrary::QuitGame(GetWorld(), UMyLib::GetPlayerCon(), EQuitPreference::Quit, true);
}

void UMyGameInstance::WinGame()
{
	PRINTF("UMyGameInstance::WinGame()!!");
	m_bIsPlayerWin = true;
}

bool UMyGameInstance::IsPlayerWin()
{
	return m_bIsPlayerWin;
}

void UMyGameInstance::SetPlayerWinFalse()
{
	m_bIsPlayerWin = false;
}
