//Game2D Library v1.0
//Written by Andrew Sturges

#ifndef _GAME_2D_H
#define _GAME_2D_H

#include <list>
#include <vector>
#include <fstream>
#include "..\WinGUI\WinGUI.h"
#include "..\Vector2D\Vector2D.h"

#define BACK_STYLE_NORMAL 0
#define BACK_STYLE_CENTER 1
#define BACK_STYLE_STRETCH 2
#define BACK_STYLE_TILE 3
#define CONTACT_AXIS_NIL 0
#define CONTACT_AXIS_X 1
#define CONTACT_AXIS_Y 2
#define CONTACT_SIDE_NIL 0
#define CONTACT_SIDE_LEFT 1
#define CONTACT_SIDE_RIGHT 2
#define CONTACT_SIDE_TOP 3
#define CONTACT_SIDE_BOTTOM 4

class CLayer;
class CBoundingRect;
class CSpriteEx;
class CCollisionEvent;

extern bool gameLoopDone;
extern bool gameLoopPaused;
extern DWORD ticksPerFrame;
extern float motionStepTime;
extern CVector2D gravityAcceleration;
extern CImage* backImagePtr;
extern int backImageStyle;
extern bool drawBackImageRelativeToView;
extern CVector2D viewPosition;
extern CVector2D viewSize;
extern CBoundingRect viewLimitRect;
extern std::list<CSpriteFrameSheet> frameSheetList;
extern std::list<CLayer> layerList;
extern std::list<CCollisionEvent> collisionEventList;
extern std::list<CCollisionEvent> prevCollisionEventList;
extern CMainWindow gameMainWin;
extern CDrawBox gameView;
extern void (*beginGameLoopFuncPtr)(void);
extern void (*endGameLoopFuncPtr)(void);
extern void (*beginMoveSpritesFuncPtr)(void);
extern void (*endMoveSpritesFuncPtr)(void);
extern void (*beginDetectCollisionsFuncPtr)(void);
extern void (*endDetectCollisionsFuncPtr)(void);
extern void (*beginResolveCollisionsFuncPtr)(void);
extern void (*endResolveCollisionsFuncPtr)(void);
extern void (*beginDrawScreenFuncPtr)(void);
extern void (*endDrawScreenFuncPtr)(void);

bool Game2D_Setup(void);
void Game2D_Cleanup(void);
WPARAM GameLoop(void);
void MoveSprites(void);
void DetectCollisions(void);
void AddCollisionEvent(void);
void ResolveCollisions(void);
void DrawScreen(void);
void DrawSprites(void);
void DrawBackImage(void);
void ResizeView(int, int);
void MoveView(float, float);
void ShiftView(float, float);
void ClampView(void);
void SetLayerZOrder(std::list<CLayer>::iterator, std::list<CLayer>::iterator);
void SaveGameState(void);
void LoadGameState(void);
std::list<CCollisionEvent>::iterator FindCollisionEventBySprites(std::list<CCollisionEvent>::iterator);
LRESULT gameMainWin_OnClose_Default(CWindow*, const CWinEvent&);
LRESULT gameMainWin_OnSize_Default(CWindow*, const CWinEvent&);

class CLayer
{
  public:

    std::string layerName;
    bool allowMotion;
    bool allowCollisions;
    bool isVisible;
    std::list<CSpriteEx> spriteList;

    CLayer(void);

    bool WriteToFile(std::ofstream&);
    void SetSpriteZOrder(std::list<CSpriteEx>::iterator, std::list<CSpriteEx>::iterator);
    void MoveSpriteToLayer(std::list<CSpriteEx>::iterator, std::list<CLayer>::iterator, std::list<CSpriteEx>::iterator);
};

class CBoundingRect
{
  public:

    CVector2D min;
    CVector2D max;

    bool IntersectsWith(CBoundingRect&);
    bool IntersectsWith(CVector2D&, CVector2D&);
};

class CSpriteEx : public CSprite
{
  public:

    CVector2D position;
    CVector2D velocity;
    CVector2D acceleration;
    CVector2D minPosition;
    CVector2D maxPosition;
    CVector2D minVelocity;
    CVector2D maxVelocity;
    float gravityScale;
    bool applyMotion;
    bool applyCollisions;
    bool clampPosition;
    bool clampVelocity;
    bool drawRelativeToView;
    CBoundingRect boundingRect;
    CVector2D prevPosition;
    CVector2D prevVelocity;
    void (*preResoveCollisionFuncPtr)(std::list<CCollisionEvent>::iterator, unsigned int, unsigned int);
    void (*postResoveCollisionFuncPtr)(std::list<CCollisionEvent>::iterator, unsigned int, unsigned int);
    void (*leaveCollisionEventFuncPtr)(std::list<CCollisionEvent>::iterator, unsigned int, unsigned int);

    CSpriteEx(void);
    CSpriteEx(CSpriteFrameSheet*);
    //~CSpriteEx(void);

    bool WriteToFile(std::ofstream&);
    void UpdateMotion(float);
    void GetAbsBoundingRect(CBoundingRect&);
    void ResizeWithBoundingRect(int, int);
    void ScaleWithBoundingRect(int, int);

  private:

    void Init(void);
};

class CCollisionEvent
{
  public:

    std::list<CLayer>::iterator collisionLayerItr;
    std::list<CSpriteEx>::iterator collidingSpriteItr[2];
    float contactTime;
    int contactAxis;
    int contactSide[2];
    CVector2D resolvedVelocity[2];
    CVector2D resolvedPosition[2];
    bool resolveCollision;
    bool resolveAxisX;
    bool resolveAxisY;

    CCollisionEvent(void);
    CCollisionEvent(std::list<CSpriteEx>::iterator, std::list<CSpriteEx>::iterator, float, int, int, int);
    //~CCollisionEvent(void);

    bool operator<(const CCollisionEvent&) const;
    void PrintData(void);
};

#endif
