// Don't worry about Intellisense errors here, as this file is only used in conjunction with Options.h/Options.cpp
// To add a new option, add a new variable entry and a corresponding OptionInfo in Options.cpp

// General options
OPT int displayWidth, displayHeight, maxFrameSkip, baseXResolution, baseYResolution, baseXGeoscape, baseYGeoscape, baseXBattlescape, baseYBattlescape,
	soundVolume, musicVolume, uiVolume, audioSampleRate, audioBitDepth, audioChunkSize, pauseMode, windowedModePositionX, windowedModePositionY, FPS, FPSInactive,
	changeValueByMouseWheel, dragScrollTimeTolerance, dragScrollPixelTolerance, mousewheelSpeed, autosaveFrequency;
OPT bool fullscreen, asyncBlit, playIntro,
	autosave, allowResize, borderless, debug, debugUi, fpsCounter, newSeedOnLoad, keepAspectRatio, nonSquarePixelRatio,
	maximizeInfoScreens, musicAlwaysLoop, StereoSound, verboseLogging, soldierDiaries, touchEnabled,
	rootWindowedMode, rawScreenShots, lazyLoadResources, listVFSContents, embeddedOnly;
// SDL2 scalers
OPT bool useNearestScaler, useLinearScaler, useAnisotropicScaler, integerRatioScaling;
OPT bool integerRatioScaling;

OPT std::string language, renderDriver, renderFilterSDL, renderFilterGL2;
OPT KeyboardType keyboardMode;
OPT SaveSort saveOrder;
OPT MusicFormat preferredMusic;
OPT SoundFormat preferredSound;
OPT VideoFormat preferredVideo;
OPT bool captureMouse;
OPT SDL_Keycode keyOk, keyCancel, keyScreenshot, keyFps, keyQuickLoad, keyQuickSave;

// Geoscape options
OPT int geoClockSpeed, dogfightSpeed, geoScrollSpeed, geoDragScrollButton, geoscapeScale;
OPT bool includePrimeStateInSavedLayout, anytimePsiTraining, weaponSelfDestruction, retainCorpses, craftLaunchAlways, globeSeasons, globeDetail, globeRadarLines, globeFlightPaths, globeAllRadarsOnBaseBuild,
	storageLimitsEnforced, canSellLiveAliens, canTransferCraftsWhileAirborne, customInitialBase, aggressiveRetaliation, geoDragScrollInvert,
	allowBuildingQueue, showFundsOnGeoscape, psiStrengthEval, allowPsiStrengthImprovement, fieldPromotions, meetingPoint;
// Added for Android
OPT bool dragSoldierReorder;
OPT SDL_Keycode keyGeoLeft, keyGeoRight, keyGeoUp, keyGeoDown, keyGeoZoomIn, keyGeoZoomOut, keyGeoSpeed1, keyGeoSpeed2, keyGeoSpeed3, keyGeoSpeed4, keyGeoSpeed5, keyGeoSpeed6,
	keyGeoIntercept, keyGeoBases, keyGeoGraphs, keyGeoUfopedia, keyGeoOptions, keyGeoFunding, keyGeoToggleDetail, keyGeoToggleRadar,
	keyBaseSelect1, keyBaseSelect2, keyBaseSelect3, keyBaseSelect4, keyBaseSelect5, keyBaseSelect6, keyBaseSelect7, keyBaseSelect8;

// Battlescape options
OPT ScrollType battleEdgeScroll;
OPT PathPreview battleNewPreviewPath;
OPT int battleScrollSpeed, battleDragScrollButton, battleFireSpeed, battleXcomSpeed, battleAlienSpeed, battleExplosionHeight, battlescapeScale;
OPT bool traceAI, sneakyAI, battleInstantGrenade, battleNotifyDeath, battleTooltips, battleHairBleach, battleAutoEnd,
	strafe, forceFire, showMoreStatsInInventoryView, allowPsionicCapture, skipNextTurnScreen, disableAutoEquip, battleDragScrollInvert,
	battleUFOExtenderAccuracy, battleConfirmFireMode, battleSmoothCamera, noAlienPanicMessages, alienBleeding;
OPT SDL_Keycode keyBattleLeft, keyBattleRight, keyBattleUp, keyBattleDown, keyBattleLevelUp, keyBattleLevelDown, keyBattleCenterUnit, keyBattlePrevUnit, keyBattleNextUnit, keyBattleDeselectUnit,
keyBattleUseLeftHand, keyBattleUseRightHand, keyBattleInventory, keyBattleMap, keyBattleOptions, keyBattleEndTurn, keyBattleAbort, keyBattleStats, keyBattleKneel,
keyBattleReserveKneel, keyBattleReload, keyBattlePersonalLighting, keyBattleReserveNone, keyBattleReserveSnap, keyBattleReserveAimed, keyBattleReserveAuto,
keyBattleCenterEnemy1, keyBattleCenterEnemy2, keyBattleCenterEnemy3, keyBattleCenterEnemy4, keyBattleCenterEnemy5, keyBattleCenterEnemy6, keyBattleCenterEnemy7, keyBattleCenterEnemy8,
keyBattleCenterEnemy9, keyBattleCenterEnemy10, keyBattleVoxelView, keyBattleZeroTUs, keyInvCreateTemplate, keyInvApplyTemplate, keyInvClear, keyInvAutoEquip;

// Extra hotkeys (OXCE)
OPT SDL_Keycode keyGeoUfoTracker, keyGeoTechTreeViewer, keyGeoGlobalResearch, keyGeoGlobalProduction,
	keyGraphsZoomIn, keyGraphsZoomOut,
	keyToggleQuickSearch,
	keyCraftLoadoutSave, keyCraftLoadoutLoad,
	keyMarkAllAsSeen,
	keySellAll, keySellAllButOne,
	keyRemoveSoldiersFromCraft, keyRemoveSoldiersFromAllCrafts,
	keyRemoveEquipmentFromCraft,
	keyRemoveArmorFromCraft, keyRemoveArmorFromAllCrafts,
	keyRemoveSoldiersFromTraining,
	keyInventoryArmor, keyInventoryAvatar, keyInventorySave, keyInventoryLoad,
	keyBattleUseSpecial,
	keyBattleActionItem1, keyBattleActionItem2, keyBattleActionItem3, keyBattleActionItem4, keyBattleActionItem5,
	keyNightVisionToggle, keyNightVisionHold, keySelectMusicTrack;

// Extra options (OXCE)
OPT int oxceStartUpTextMode;
OPT bool oxceUpdateCheck;

OPT bool oxceUfoLandingAlert;
OPT int oxceWoundedDefendBaseIf;
OPT bool oxcePlayBriefingMusicDuringEquipment;
OPT int oxceNightVisionColor;
OPT bool oxceAutoSell;
OPT int oxceAutoNightVisionThreshold;
OPT bool oxceRememberDisabledCraftWeapons;
OPT bool oxceHighlightNewTopicsHidden;
OPT bool oxceEnableUnitResponseSounds;
OPT std::string password;

// System-specific options
OPT bool swipeToTurn, holdToTurn;
OPT SystemUIStyle systemUI;
OPT bool logToFile, logToSystem, logTouch, fakeEvents, forceGLMode;
OPT int mouseMode;
OPT bool listDragScroll;
OPT int longPressDuration;
OPT bool showCursor;

// Flags and other stuff that don't need OptionInfo's.
OPT bool mute, reload, newRootWindowedMode, newFullscreen, newAllowResize, newBorderless, newKeepAspectRatio, newNonSquarePixelRatio;
OPT int newDisplayWidth, newDisplayHeight, newBattlescapeScale, newGeoscapeScale, newWindowedModePositionX, newWindowedModePositionY;
OPT std::string newRenderDriver, newRenderFilterSDL, newRenderFilterGL2;
OPT std::vector< std::pair<std::string, bool> > mods; // ordered list of available mods (lowest priority to highest) and whether they are active
OPT SoundFormat currentSound;
OPT std::string systemLocale;
