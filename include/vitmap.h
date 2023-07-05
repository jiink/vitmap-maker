#ifndef VITMAP_H
#define VITMAP_H

#include "raylib.h"
#include "tesselator.h"

// Shapes are closed polyogns
typedef struct Shape
{
    Vector2* points;
    int numPoints;
    TESStesselator* tesselation;
    Color color;
} Shape;

typedef struct Vitmap
{
    Shape* shapes;
    int numShapes;
} Vitmap;

typedef struct VitmapAnimation
{
    Vitmap* vitmaps;
    int numFrames;
    int currentFrame;
} VitmapAnimation;

typedef struct VitmapAnimationSet
{
    VitmapAnimation* animations;
    int numAnimations;
} VitmapAnimationSet;

void printVitmap(const Vitmap* vitmap);
Shape* createShape();
Vitmap* createVitmap();
void bakeVitmap(Vitmap* vitmap);
void addPointToShape(Shape* shape, Vector2 point);
void removePointFromShape(Shape* shape, Vector2* point);
void addShapeToVitmap(Vitmap* vitmap);
void removeShapeFromVitmap(Vitmap* vitmap, Shape* shape);
Shape* reorderShapeInVitmap(Vitmap* vitmap, Shape* shape, int direction);
Vitmap* addVitmapToAnimation(VitmapAnimation* animation, Vitmap vitmap);
void saveVitmapToFile(Vitmap* vitmap, const char* filename);
Vitmap loadVitmapFromFile(const char* filename);
Vitmap* loadAndBakeVitmap(const char* filename);
void drawVitmap(Vitmap *vitmap, Vector2 position, Vector2 scale, float rotation);

#endif // VITMAP_H