#include "float.h"
#include "math.h"
#include <iostream>
#include "..\String Extension\stringExt.h"
#include "Game2D.h"

using namespace std;


bool gameLoopDone;
bool gameLoopPaused;
DWORD ticksPerFrame;
float motionStepTime;
CVector2D gravityAcceleration;
CImage* backImagePtr;
int backImageStyle;
bool drawBackImageRelativeToView;
CVector2D viewPosition;
CVector2D viewSize;
CBoundingRect viewLimitRect;
list<CLayer> layerList;
list<CCollisionEvent> collisionEventList;
CMainWindow gameMainWin;
CDrawBox gameView;
void (*beginGameLoopFuncPtr)(void);
void (*endGameLoopFuncPtr)(void);
void (*beginMoveSpritesFuncPtr)(void);
void (*endMoveSpritesFuncPtr)(void);
void (*beginDetectCollisionsFuncPtr)(void);
void (*endDetectCollisionsFuncPtr)(void);
void (*beginResolveCollisionsFuncPtr)(void);
void (*endResolveCollisionsFuncPtr)(void);
void (*beginDrawScreenFuncPtr)(void);
void (*endDrawScreenFuncPtr)(void);

//For use by this module only
list<CLayer>::iterator layerItr;
list<CSpriteEx>::iterator spriteItr1;
list<CSpriteEx>::iterator spriteItr2;
list<CCollisionEvent>::iterator eventItr;
CBoundingRect absBoundingRect1;
CBoundingRect absBoundingRect2;

bool Game2D_Setup(void)
//Create main window
{
    if(!WinGUISetup())
        return false;

    //Initialize game loop switches
    gameLoopDone = false;
    gameLoopPaused = false;

    //Initialize time length per frame (milliseconds)
    ticksPerFrame = 33;

    //Initialize time length per physics step (seconds)
    motionStepTime = ticksPerFrame / 1000.0f;

    //Initialize background image
    backImagePtr = NULL;
    backImageStyle = BACK_STYLE_NORMAL;
    drawBackImageRelativeToView = true;

    //Initialize the view rectangle
    viewSize.x = 100;
    viewSize.y = 100;

    //Initialize the view rectangle's limits
    viewLimitRect.min.SetComponents(-FLT_MAX, -FLT_MAX);
    viewLimitRect.max.SetComponents(FLT_MAX, FLT_MAX);

    //Create an initial layer
    layerList.emplace_front();

    //Create main window & view
    gameMainWin.Create("Game2D Test", 300, 100, 700, 500, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU
                       | WS_MINIMIZEBOX | WS_CLIPCHILDREN);
    gameView.Create(&gameMainWin, 0, 0, 600, 400, WS_VISIBLE | WS_CHILD | WS_BORDER);

    //Set the event handlers
    gameMainWin.AddEvent(WM_CLOSE, gameMainWin_OnClose_Default, WINEVENT_MESSAGE);
    gameMainWin.AddEvent(WM_SIZE, gameMainWin_OnSize_Default, WINEVENT_MESSAGE);

    //Draw graphics to the back buffer
    gameView.UseRedrawDC();

    //Initialize game loop event handlers
    beginGameLoopFuncPtr = NULL;
    endGameLoopFuncPtr = NULL;
    beginMoveSpritesFuncPtr = NULL;
    endMoveSpritesFuncPtr = NULL;
    beginDetectCollisionsFuncPtr = NULL;
    endDetectCollisionsFuncPtr = NULL;
    beginResolveCollisionsFuncPtr = NULL;
    endResolveCollisionsFuncPtr = NULL;
    beginDrawScreenFuncPtr = NULL;
    endDrawScreenFuncPtr = NULL;

    return true;
}

void Game2D_Cleanup(void)
//Destroy resources and main window
{
    //Close the main window
    gameMainWin.Destroy();

    //Cleanup GUI support
    WinGUICleanup();
}

WPARAM GameLoop(void)
//Process all main game functions
{
    DWORD startTick;
    DWORD sleepTicks;
    MSG msgInfo;

    while(true)
    {
        if(gameLoopDone)
        {
            return 0;
        }

        startTick = GetTickCount();

        if(beginGameLoopFuncPtr != NULL)
            (*beginGameLoopFuncPtr)();

        FlushWinEvents(&msgInfo);

        if(msgInfo.message == WM_QUIT)
        {
            return msgInfo.wParam;
        }

        if(!gameLoopPaused)
        {
            MoveSprites();

            DetectCollisions();

            ResolveCollisions();

            DrawScreen();
        }

        if(endGameLoopFuncPtr != NULL)
            (*endGameLoopFuncPtr)();

        sleepTicks = ticksPerFrame - (GetTickCount() - startTick);
        if((sleepTicks > 0) && (sleepTicks < ticksPerFrame))
        {
            Sleep(sleepTicks);
        }
    }
}

void MoveSprites(void)
//Apply accelerations and velocities to the sprites
{
    if(beginMoveSpritesFuncPtr != NULL)
        (*beginMoveSpritesFuncPtr)();

    for(layerItr = layerList.begin(); layerItr != layerList.end(); layerItr++)
    {
        if(layerItr->allowMotion)
        {
            for(spriteItr1 = layerItr->spriteList.begin(); spriteItr1 != layerItr->spriteList.end(); spriteItr1++)
            {
                if(spriteItr1->applyMotion)
                {
                    if(spriteItr1->applyGravity)
                    {
                        spriteItr1->acceleration += gravityAcceleration;
                    }

                    spriteItr1->UpdateMotion(motionStepTime);
                }
            }
        }
    }

    if(endMoveSpritesFuncPtr != NULL)
        (*endMoveSpritesFuncPtr)();
}

void DetectCollisions(void)
//Look for any overlapping sprites
{
    if(beginDetectCollisionsFuncPtr != NULL)
        (*beginDetectCollisionsFuncPtr)();

    for(layerItr = layerList.begin(); layerItr != layerList.end(); layerItr++)
    {
        if(layerItr->allowCollisions)
        {
            for(spriteItr1 = layerItr->spriteList.begin(); spriteItr1 != layerItr->spriteList.end(); spriteItr1++)
            {
                spriteItr2 = spriteItr1;
                spriteItr2++;
                for( ; spriteItr2 != layerItr->spriteList.end(); spriteItr2++)
                {
                    if(spriteItr1->applyCollisions && spriteItr2->applyCollisions)
                    {
                        spriteItr1->GetAbsBoundingRect(absBoundingRect1);
                        spriteItr2->GetAbsBoundingRect(absBoundingRect2);
                        if(absBoundingRect1.IntersectsWith(absBoundingRect2))
                        {
                            AddCollisionEvent();
                        }
                    }
                }
            }
        }
    }

    if(endDetectCollisionsFuncPtr != NULL)
        (*endDetectCollisionsFuncPtr)();
}

void AddCollisionEvent(void)
//Create a new collision event for two overlapping sprites
{
    static vector<CCollisionEvent> alignmentList;
    static CVector2D relativeVelocity;
    static float alignmentTime;
    static float deltaTime;

    //Only need max of 2 alignment events
    alignmentList.reserve(2);

    relativeVelocity = spriteItr2->velocity - spriteItr1->velocity;

    if(relativeVelocity.y != 0)
    {
        alignmentTime = motionStepTime + ((absBoundingRect1.max.y - absBoundingRect2.min.y) / relativeVelocity.y);
        if(fabs(alignmentTime) < 0.001f) alignmentTime = 0.0f;
        if((alignmentTime >= 0) && (alignmentTime < motionStepTime))
        {
            //Add bottom-top alignment event
            alignmentList.emplace_back();
            alignmentList.back().contactTime = alignmentTime;
            alignmentList.back().contactAxis = CONTACT_AXIS_Y;
            alignmentList.back().contactSide[0] = CONTACT_SIDE_BOTTOM;
            alignmentList.back().contactSide[1] = CONTACT_SIDE_TOP;
        }
        else
        {
            alignmentTime = motionStepTime + ((absBoundingRect1.min.y - absBoundingRect2.max.y) / relativeVelocity.y);
            if(fabs(alignmentTime) < 0.001f) alignmentTime = 0.0f;
            if((alignmentTime >= 0) && (alignmentTime < motionStepTime))
            {
                //Add top-bottom alignment event
                alignmentList.emplace_back();
                alignmentList.back().contactTime = alignmentTime;
                alignmentList.back().contactAxis = CONTACT_AXIS_Y;
                alignmentList.back().contactSide[0] = CONTACT_SIDE_TOP;
                alignmentList.back().contactSide[1] = CONTACT_SIDE_BOTTOM;
            }
        }
    }

    if(relativeVelocity.x != 0)
    {
        alignmentTime = motionStepTime + ((absBoundingRect1.max.x - absBoundingRect2.min.x) / relativeVelocity.x);
        if(fabs(alignmentTime) < 0.001f) alignmentTime = 0.0f;
        if((alignmentTime >= 0) && (alignmentTime < motionStepTime))
        {
            //Add right-left alignment event
            alignmentList.emplace_back();
            alignmentList.back().contactTime = alignmentTime;
            alignmentList.back().contactAxis = CONTACT_AXIS_X;
            alignmentList.back().contactSide[0] = CONTACT_SIDE_RIGHT;
            alignmentList.back().contactSide[1] = CONTACT_SIDE_LEFT;
        }
        else
        {
            alignmentTime = motionStepTime + ((absBoundingRect1.min.x - absBoundingRect2.max.x) / relativeVelocity.x);
            if(fabs(alignmentTime) < 0.001f) alignmentTime = 0.0f;
            if((alignmentTime >= 0) && (alignmentTime < motionStepTime))
            {
                //Add left-right alignment event
                alignmentList.emplace_back();
                alignmentList.back().contactTime = alignmentTime;
                alignmentList.back().contactAxis = CONTACT_AXIS_X;
                alignmentList.back().contactSide[0] = CONTACT_SIDE_LEFT;
                alignmentList.back().contactSide[1] = CONTACT_SIDE_RIGHT;
            }
        }
    }

    if(alignmentList.size() == 0)
    {
        collisionEventList.emplace_back();
        collisionEventList.back().resolveCollision = false;
    }
    else
    {
        if(alignmentList.size() == 2)
        {
            //If both alignment events happened at the same time, choose the Y-axis alignment
            if(alignmentList[0].contactTime >= alignmentList[1].contactTime)
            {
                //Contact happened on the Y-axis alignment event
                collisionEventList.push_back(alignmentList[0]);
            }
            else
            {
                //Contact happened on the X-axis alignment event
                collisionEventList.push_back(alignmentList[1]);
            }
        }
        else
        {
            collisionEventList.push_back(alignmentList[0]);
        }
    }

    //If the two sprites were initially in contact, allow them to "slide" along each other
    if(collisionEventList.back().contactTime == 0.0f)
    {
        if(collisionEventList.back().contactAxis == CONTACT_AXIS_Y)
        {
            //Slide along X-axis
            collisionEventList.back().resolveAxisX = false;
        }
        else if(collisionEventList.back().contactAxis == CONTACT_AXIS_X)
        {
            //Slide along Y-axis
            collisionEventList.back().resolveAxisY = false;
        }
    }

    //Set resolved positions to the time of contact
    deltaTime = motionStepTime - collisionEventList.back().contactTime;
    collisionEventList.back().resolvedPosition[0] = spriteItr1->position - (spriteItr1->velocity * deltaTime);
    collisionEventList.back().resolvedPosition[1] = spriteItr2->position - (spriteItr2->velocity * deltaTime);

    //Set resolved velocities
    if(collisionEventList.back().contactAxis == CONTACT_AXIS_Y)
    {
        collisionEventList.back().resolvedVelocity[0].SetComponents(spriteItr1->velocity.x, 0);
        collisionEventList.back().resolvedVelocity[1].SetComponents(spriteItr2->velocity.x, 0);
    }
    else if(collisionEventList.back().contactAxis == CONTACT_AXIS_X)
    {
        collisionEventList.back().resolvedVelocity[0].SetComponents(0, spriteItr1->velocity.y);
        collisionEventList.back().resolvedVelocity[1].SetComponents(0, spriteItr2->velocity.y);
    }

    //Save the layer and sprite iterators to the new collision event
    collisionEventList.back().collisionLayerItr = layerItr;
    collisionEventList.back().collidingSpriteItr[0] = spriteItr1;
    collisionEventList.back().collidingSpriteItr[1] = spriteItr2;

    alignmentList.clear();
}

void ResolveCollisions(void)
//Set each colliding sprite pair to their resolved positions
{
    if(beginResolveCollisionsFuncPtr != NULL)
        (*beginResolveCollisionsFuncPtr)();

    if(collisionEventList.empty())
        return;

    //Sort list by contact time in descending order
    collisionEventList.sort();

    for(eventItr = collisionEventList.begin(); eventItr != collisionEventList.end(); eventItr++)
    {
        //Pre-resolution callbacks
        if(eventItr->collidingSpriteItr[0]->preResoveCollisionFuncPtr != NULL)
            (*(eventItr->collidingSpriteItr[0]->preResoveCollisionFuncPtr))(eventItr, 0, 1);
        if(eventItr->collidingSpriteItr[1]->preResoveCollisionFuncPtr != NULL)
            (*(eventItr->collidingSpriteItr[1]->preResoveCollisionFuncPtr))(eventItr, 1, 0);

        if(eventItr->resolveCollision)
        {
            if(eventItr->resolveAxisY)
            {
                eventItr->collidingSpriteItr[0]->position.y = eventItr->resolvedPosition[0].y;
                eventItr->collidingSpriteItr[1]->position.y = eventItr->resolvedPosition[1].y;
                eventItr->collidingSpriteItr[0]->velocity.y = eventItr->resolvedVelocity[0].y;
                eventItr->collidingSpriteItr[1]->velocity.y = eventItr->resolvedVelocity[1].y;
            }

            if(eventItr->resolveAxisX)
            {
                eventItr->collidingSpriteItr[0]->position.x = eventItr->resolvedPosition[0].x;
                eventItr->collidingSpriteItr[1]->position.x = eventItr->resolvedPosition[1].x;
                eventItr->collidingSpriteItr[0]->velocity.x = eventItr->resolvedVelocity[0].x;
                eventItr->collidingSpriteItr[1]->velocity.x = eventItr->resolvedVelocity[1].x;
            }
        }

        //Post-resolution callbacks
        if(eventItr->collidingSpriteItr[0]->postResoveCollisionFuncPtr != NULL)
            (*(eventItr->collidingSpriteItr[0]->postResoveCollisionFuncPtr))(eventItr, 0, 1);
        if(eventItr->collidingSpriteItr[1]->postResoveCollisionFuncPtr != NULL)
            (*(eventItr->collidingSpriteItr[1]->postResoveCollisionFuncPtr))(eventItr, 1, 0);
    }

    collisionEventList.clear();

    if(endResolveCollisionsFuncPtr != NULL)
        (*endResolveCollisionsFuncPtr)();
}

void DrawScreen(void)
//Draw background image and sprites to the view
{
    if(beginDrawScreenFuncPtr != NULL)
        (*beginDrawScreenFuncPtr)();

    DrawBackImage();
    DrawSprites();
    gameView.Redraw();

    if(endDrawScreenFuncPtr != NULL)
        (*endDrawScreenFuncPtr)();
}

void DrawSprites(void)
//Draw sprites to the view
{
    for(layerItr = layerList.begin(); layerItr != layerList.end(); layerItr++)
    {
        if(layerItr->isVisible)
        {
            for(spriteItr1 = layerItr->spriteList.begin(); spriteItr1 != layerItr->spriteList.end(); spriteItr1++)
            {
                spriteItr1->drawLeft = int(roundf(spriteItr1->position.x));
                spriteItr1->drawTop = int(roundf(spriteItr1->position.y));
                if(spriteItr1->drawRelativeToView)
                {
                    spriteItr1->Draw(&gameView);
                }
                else
                {
                    spriteItr1->Draw(&gameView, -int(roundf(viewPosition.x)), -int(roundf(viewPosition.y)));
                }
            }
        }
    }
}

void DrawBackImage(void)
//Draw background image to the view
{
    int imgOffsetLeft;
    int imgOffsetTop;
    int imgLeft;
    int imgTop;

    if(backImagePtr == NULL)
        return;

    if(drawBackImageRelativeToView)
    {
        imgOffsetLeft = 0;
        imgOffsetTop = 0;
    }
    else
    {
        imgOffsetLeft = -(int(roundf(viewPosition.x)));
        imgOffsetTop = -(int(roundf(viewPosition.y)));
    }

    switch(backImageStyle)
    {
      case BACK_STYLE_NORMAL:
        gameView.DrawImage(backImagePtr, imgOffsetLeft, imgOffsetTop);
        break;

      case BACK_STYLE_CENTER:
        imgOffsetLeft += int(viewSize.x / 2) - int(backImagePtr->imgWidth / 2);
        imgOffsetTop += int(viewSize.y / 2) - int(backImagePtr->imgHeight / 2);
        gameView.DrawImage(backImagePtr, imgOffsetLeft, imgOffsetTop);
        break;

      case BACK_STYLE_STRETCH:
        gameView.DrawImageScaled(backImagePtr, imgOffsetLeft, imgOffsetTop, viewSize.x, viewSize.y);
        break;

      case BACK_STYLE_TILE:
        imgOffsetLeft = -((-imgOffsetLeft) % backImagePtr->imgWidth);
        imgOffsetTop = -((-imgOffsetTop) % backImagePtr->imgHeight);
        for(imgTop = imgOffsetTop; imgTop < viewSize.y; imgTop += backImagePtr->imgHeight)
        {
            for(imgLeft = imgOffsetLeft; imgLeft < viewSize.x; imgLeft += backImagePtr->imgWidth)
            {
                gameView.DrawImage(backImagePtr, imgLeft, imgTop);
            }
        }
        break;
    }
}

void ResizeView(int newWidth, int newHeight)
//Change the size of the view rectangle
{
    viewSize.x = newWidth;
    viewSize.y = newHeight;

    gameMainWin.SetClientSize(viewSize.x, viewSize.y);

    ClampView();
}

void MoveView(float newLeft, float newTop)
//Change view rectangle's position to a new absolute position
{
    viewPosition.x = newLeft;
    viewPosition.y = newTop;

    ClampView();
}

void ShiftView(float offsetX, float offsetY)
//Change the view rectangle's position by a relative offset
{
    viewPosition.x += offsetX;
    viewPosition.y += offsetY;

    ClampView();
}

void ClampView(void)
//Make sure the view rectangle is within the view limits
{
    viewPosition.ClampX(viewLimitRect.min.x, viewLimitRect.max.x - viewSize.x);
    viewPosition.ClampY(viewLimitRect.min.y, viewLimitRect.max.y - viewSize.y);
}

void SetLayerZOrder(list<CLayer>::iterator targetLayerItr, list<CLayer>::iterator beforeLayerItr)
//
{
    layerList.splice(beforeLayerItr, layerList, targetLayerItr);
}

LRESULT gameMainWin_OnClose_Default(CWindow* winPtr, const CWinEvent& eventObj)
//End the game
{
    PostQuitMessage(0);
    return 0;
}

LRESULT gameMainWin_OnSize_Default(CWindow* winPtr, const CWinEvent& eventObj)
//
{
    int newWidth = LOWORD(eventObj.lParam);
    int newHeight = HIWORD(eventObj.lParam);

    gameView.SetSize(newWidth, newHeight);

    return 0;
}

CLayer::CLayer(void)
//Initialize layer properties
{
    layerName = "";
    allowMotion = true;
    allowCollisions = true;
    isVisible = true;
}

void CLayer::SetSpriteZOrder(list<CSpriteEx>::iterator targetSpriteItr, list<CSpriteEx>::iterator beforeSpriteItr)
//
{
    spriteList.splice(beforeSpriteItr, spriteList, targetSpriteItr);
}

void CLayer::MoveSpriteToLayer(list<CSpriteEx>::iterator targetSpriteItr, list<CLayer>::iterator targetLayerItr,
                               list<CSpriteEx>::iterator beforeSpriteItr)
//
{
    targetLayerItr->spriteList.splice(beforeSpriteItr, spriteList, targetSpriteItr);
}

bool CBoundingRect::IntersectsWith(CBoundingRect& boundRectRef)
//Return true if the given bounding rectangle intersect with this one
{
    //Check along x-axis
    if((max.x <= boundRectRef.min.x) || (min.x >= boundRectRef.max.x))
        return false;

    //Check along y-axis
    if((max.y <= boundRectRef.min.y) || (min.y >= boundRectRef.max.y))
        return false;

    return true;
}

bool CBoundingRect::IntersectsWith(CVector2D& positionVectRef, CVector2D& sizeVectRef)
//Return true if the given position-size rectangle intersect with this one
{
    //Check along x-axis
    if((max.x <= positionVectRef.x) || (min.x >= (positionVectRef.x + sizeVectRef.x)))
        return false;

    //Check along y-axis
    if((max.y <= positionVectRef.y) || (min.y >= (positionVectRef.y + sizeVectRef.y)))
        return false;

    return true;
}

CSpriteEx::CSpriteEx(void)
//
{
    Init();
}

CSpriteEx::CSpriteEx(CSpriteFrameSheet* newFrameSheetPtr)
    : CSprite(newFrameSheetPtr)
//
{
    Init();
}

void CSpriteEx::UpdateMotion(float deltaTime)
//Update the sprite's velocity and position
{
    velocity += (acceleration * deltaTime);

    if(clampVelocity)
    {
        velocity.Clamp(minVelocity, maxVelocity);
    }

    position += (velocity * deltaTime);

    //Acceleration does not persist between time steps
    acceleration.SetComponents(0, 0);
}

void CSpriteEx::GetAbsBoundingRect(CBoundingRect& absBoundingRectRef)
//Get the absolute coordinates of the sprite's bounding rectangle
{
    absBoundingRectRef.min = position + boundingRect.min;
    absBoundingRectRef.max = position + boundingRect.max;
}

void CSpriteEx::Init(void)
//Initialize extended sprite properties
{
    minVelocity.SetComponents(-FLT_MAX, -FLT_MAX);
    maxVelocity.SetComponents(FLT_MAX, FLT_MAX);
    applyMotion = true;
    applyGravity = false;
    applyCollisions = true;
    clampVelocity = false;
    drawRelativeToView = false;
    boundingRect.max.x = float(drawWidth);
    boundingRect.max.y = float(drawHeight);
    preResoveCollisionFuncPtr = NULL;
    postResoveCollisionFuncPtr = NULL;
}

CCollisionEvent::CCollisionEvent(void)
//Initialize collision event data
{
    if(layerList.size() > 0)
    {
        collisionLayerItr = layerList.begin();
        collidingSpriteItr[0] = layerList.front().spriteList.begin();
        collidingSpriteItr[1] = layerList.front().spriteList.begin();
    }
    contactTime = 0.0f;
    contactAxis = CONTACT_AXIS_NIL;
    contactSide[0] = CONTACT_SIDE_NIL;
    contactSide[1] = CONTACT_SIDE_NIL;
    resolveCollision = true;
    resolveAxisX = true;
    resolveAxisY = true;
}

bool CCollisionEvent::operator<(const CCollisionEvent &compTest) const
//Used to sort the collision event list by contact time in descending order
{
    return (contactTime > compTest.contactTime);
}

void CCollisionEvent::PrintData(void)
//
{
    cout << endl;
    cout << "sprite0 = " << collidingSpriteItr[0]->spriteName.c_str() << endl;
    cout << "sprite1 = " << collidingSpriteItr[1]->spriteName.c_str() << endl;
    cout << "contactTime = " << contactTime << endl;
    cout << "contactAxis = " << contactAxis << endl;
    cout << "contactSide[0] = " << contactSide[0] << endl;
    cout << "contactSide[1] = " << contactSide[1] << endl;
    cout << "resolvedVelocity[0] = " << resolvedVelocity[0].x << ", " << resolvedVelocity[0].y << endl;
    cout << "resolvedVelocity[1] = " << resolvedVelocity[1].x << ", " << resolvedVelocity[1].y << endl;
    cout << "resolvedPosition[0] = " << resolvedPosition[0].x << ", " << resolvedPosition[0].y << endl;
    cout << "resolvedPosition[1] = " << resolvedPosition[1].x << ", " << resolvedPosition[1].y << endl;
    cout << "resolveCollision = " << resolveCollision << endl;
    cout << "resolveAxisX = " << resolveAxisX << endl;
    cout << "resolveAxisY = " << resolveAxisY << endl;
    cout << endl;
}

