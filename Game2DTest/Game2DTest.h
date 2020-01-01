#ifndef _GAME_2D_TEST_H
#define _GAME_2D_TEST_H

#define MOVE_LEFT_KEY VK_LEFT
#define MOVE_RIGHT_KEY VK_RIGHT

CImage* streetImgPtr;
CSpriteFrameSheet* bubbaSheetPtr;
CSpriteFrameSheet* carSheetPtr;
CSpriteFrameSheet* curbSheetPtr;
std::list<CSpriteEx>::iterator bubbaSpriteItr;
std::list<CSpriteEx>::iterator carSpriteItr;
std::list<CSpriteEx>::iterator curbSpriteItr;
bool isKeyDown[256] = { false };

bool Game2DTestSetup(void);
void Game2DTestCleanup(void);
void SetupGraphics(void);
void CleanupGraphics(void);
int WINAPI WinMain (HINSTANCE, HINSTANCE, LPSTR, int);
LRESULT gameMainWin_OnKeyDown(CWindow*, const CWinEvent&);
LRESULT gameMainWin_OnKeyUp(CWindow*, const CWinEvent&);
void OnBeginMoveSprites(void);

#endif
