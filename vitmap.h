#include "include/raylib.h"

// Shapes are closed polygons
typedef struct Shape
{
    Vector2* points;
    int numPoints;
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
void addPointToShape(Shape* shape, Vector2 point);
void addShapeToVitmap(Vitmap* vitmap);
Vitmap* addVitmapToAnimation(VitmapAnimation* animation, Vitmap vitmap);