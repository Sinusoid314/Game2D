#include <iostream>
#include "..\Game2D.h"
#include "Game2DTest.h"

using namespace std;


bool Game2DTestSetup(void)
//Set up resources
{
    if(!Game2D_Setup())
        return false;

    SetupGraphics();

    //Set the input handlers
    gameMainWin.AddEvent(WM_KEYDOWN, gameMainWin_OnKeyDown, WINEVENT_MESSAGE);
    gameMainWin.AddEvent(WM_KEYUP, gameMainWin_OnKeyUp, WINEVENT_MESSAGE);
    beginMoveSpritesFuncPtr = OnBeginMoveSprites;

    ResizeView(600, 400);

    gravityAcceleration.y = 10;

    return true;
}

void Game2DTestCleanup(void)
//Clean up resources
{
    CleanupGraphics();
    Game2D_Cleanup();
}

void SetupGraphics(void)
//Load and initialize images and sprites
{
    //Load background image and frame sheets
    streetImgPtr = new CImage("street.bmp");
    bubbaSheetPtr = new CSpriteFrameSheet("bubba.bmp", RGB(255,0,128), 1, 2);
    carSheetPtr = new CSpriteFrameSheet("car.bmp", RGB(255,0,128), 1, 1);
    curbSheetPtr = new CSpriteFrameSheet("curb.bmp", RGB(255,0,128), 1, 1);

    //Set background image
    backImagePtr = streetImgPtr;

    //Create sprites
    layerList.front().spriteList.emplace_front(carSheetPtr);
    carSpriteItr = layerList.front().spriteList.begin();
    carSpriteItr->spriteName = "car";
    carSpriteItr->position.SetComponents(350, 10);
    carSpriteItr->gravityScale = 1;

    layerList.front().spriteList.emplace_front(bubbaSheetPtr);
    bubbaSpriteItr = layerList.front().spriteList.begin();
    bubbaSpriteItr->spriteName = "bubba";
    bubbaSpriteItr->position.SetComponents(200, 10);
    bubbaSpriteItr->drawsPerFrame = 7;
    bubbaSpriteItr->SetFrameRange(0, 0);
    bubbaSpriteItr->gravityScale = 1;

    layerList.front().spriteList.emplace_front(curbSheetPtr);
    curbSpriteItr = layerList.front().spriteList.begin();
    curbSpriteItr->spriteName = "curb";
    curbSpriteItr->position.SetComponents(0, 375);
}

void CleanupGraphics(void)
//Delete images
{
    delete carSheetPtr;
    delete bubbaSheetPtr;
    delete streetImgPtr;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
//
{
    WPARAM retVal;

    if(!Game2DTestSetup())
        return 0;

    gameMainWin.Show();

    retVal = GameLoop();

    Game2DTestCleanup();

    return retVal;
}

LRESULT gameMainWin_OnKeyDown(CWindow* winPtr, const CWinEvent& eventObj)
//
{
    //cout << "gameMainWin_OnKeyDown" << endl;

    if((!isKeyDown[MOVE_LEFT_KEY]) && (!isKeyDown[MOVE_RIGHT_KEY]))
    {
        if(eventObj.wParam == MOVE_LEFT_KEY)
        {
            isKeyDown[MOVE_LEFT_KEY] = true;
            bubbaSpriteItr->SetFrameRange(0, 1);
            bubbaSpriteItr->isMirroredX = true;
            bubbaSpriteItr->velocity.x = -60;
        }
        else if(eventObj.wParam == MOVE_RIGHT_KEY)
        {
            isKeyDown[MOVE_RIGHT_KEY] = true;
            bubbaSpriteItr->SetFrameRange(0, 1);
            bubbaSpriteItr->isMirroredX = false;
            bubbaSpriteItr->velocity.x = 60;
        }
    }

    return 0;
}

LRESULT gameMainWin_OnKeyUp(CWindow* winPtr, const CWinEvent& eventObj)
//
{
    //cout << "gameMainWin_OnKeyUp" << endl;

    if((eventObj.wParam == MOVE_LEFT_KEY) && (isKeyDown[MOVE_LEFT_KEY]))
    {
        isKeyDown[MOVE_LEFT_KEY] = false;
        bubbaSpriteItr->SetFrameRange(0, 0);
        bubbaSpriteItr->velocity.x = 0;
    }
    else if((eventObj.wParam == MOVE_RIGHT_KEY) && (isKeyDown[MOVE_RIGHT_KEY]))
    {
        isKeyDown[MOVE_RIGHT_KEY] = false;
        bubbaSpriteItr->SetFrameRange(0, 0);
        bubbaSpriteItr->velocity.x = 0;
    }

    return 0;
}

void OnBeginMoveSprites(void)
//
{
    if(isKeyDown[MOVE_LEFT_KEY])
    {
        if(bubbaSpriteItr->velocity.x > -60)
            bubbaSpriteItr->velocity.x -= 2;
    }
    else if(isKeyDown[MOVE_RIGHT_KEY])
    {
        if(bubbaSpriteItr->velocity.x < 60)
            bubbaSpriteItr->velocity.x += 2;
    }
}

