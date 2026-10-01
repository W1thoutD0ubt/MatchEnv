export module Setup;

import index;
import Util;
import Global;
import MatchCycle;
import MyEvent;

export void SetupEvents();

void onLoadEnd()
{
	auto app = PVZ::GetPVZApp();
	if (isIZMode)
		app.FastLoad(PVZLevel::I_Zombie_Endless);
	else
		app.FastLoad(PVZLevel::Vasebreaker_Endless);
}

void onTitleScreenUpdate(PVZ::Widget screen)
{
	if (PVZ::Memory::ReadMemory<bool>(screen.GetBaseAddress() + 0x0A1))
		onLoadEnd();
}

void onChallengeInitLevelAfter(PVZ::Challenge challenge)
{
	auto board = PVZ::GetBoard();
	if (board.LevelScene == SceneType::Fog)
	{
		auto lawn = board.GetLawn();
		lawn.SetRouteType(2, RouteType::Land);
		lawn.SetRouteType(3, RouteType::Land);

		for (int col = 0; col < 9; col++)
		{
			lawn.SetGridType(2, col, LawnType::Grass);
			lawn.SetGridType(3, col, LawnType::Grass);
		}
	}
	challenge.State = ChallengeState::BARLEYMATCH_IDLE;
}

bool onBoardKeyDown(PVZ::Board board, KeyCode::KeyCode code)
{
	auto widgetmgr = PVZ::GetWidgetManager();
	if (code == 'K' && widgetmgr.IsKeyDown[KeyCode::SHIFT].get())
	{
		auto challenge = board.GetChallenge();
		if (challenge.State == ChallengeState::BARLEYMATCH_IDLE)
			MatchStart();
	}
	return true;
}

bool onPopulate()
{
	Creator::CreateVase(-3, -3, VaseContent::Sun);
	return false;
}

void SetupEvents()
{
	PVZ::InitPVZDLL();
	PVZ::Memory::immediateExecute = true;

	Creator::AsmInit();

	PVZ::Memory::WriteMemory<uint32_t>(0x4526D3, accelerationFactor);
	if (accelerationFactor > 1)
		PVZ::Memory::WriteMemory<uint8_t>(0x6A9EAB, 1);
	if (!shouldDrawBoard)
		DisableBoardDraw();

	if (isAutoMode)
	{
		DisableZombieFailHome();

		EnableBackgroundRunning();
		DisableAllSounds();
		DisableNewParticle();

		DisableMusicInterfaceUpdate();
		DisableMusicUpdate();
		auto music = PVZ::GetMusic();
		music.StopAllMusic();
		music.Disabled = true;

		DisableMousePositionUpdate();
		PVZ::IsLocaleChanged = false;
	}
	else
		BoardKeyDownEvent((int)onBoardKeyDown);

	if (!isIZMode)
		VaseBreakerPopulateEvent((int)onPopulate);
	LoadEndEvent((int)onLoadEnd);
	TitleScreenUpdateEvent((int)onTitleScreenUpdate);
	ChallengeInitLevelAfterEvent((int)onChallengeInitLevelAfter);
}