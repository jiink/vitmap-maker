#include "vitmap.h"
#include <stdio.h>
#include <stddef.h>

void initShape(Shape* shape)
{
    shape->numPoints = 0;
    shape->color = (Color){0, 0, 0, 0};
}

void initVitmap(Vitmap* vitmap)
{
    vitmap->numShapes = 0;
}

void initVitmapAnimation(VitmapAnimation* vitmapAnimation)
{
    vitmapAnimation->numFrames = 0;
    vitmapAnimation->currentFrame = 0;
}

void initVitmapAnimationSet(VitmapAnimationSet* vitmapAnimationSet)
{
    vitmapAnimationSet->numAnimations = 0;
}

void printVitmap(const Vitmap* vitmap)
{
    printf("Vitmap:\n");
    printf("  Number of Shapes: %d\n", vitmap->numShapes);
    printf("  Shapes:\n");

    for (int i = 0; i < vitmap->numShapes; i++)
    {
        printf("    Shape %d:\n", i + 1);
        printf("      Number of Points: %d\n", vitmap->shapes[i].numPoints);
        printf("      Color (RGBA): (%d, %d, %d, %d)\n",
               vitmap->shapes[i].color.r,
               vitmap->shapes[i].color.g,
               vitmap->shapes[i].color.b,
               vitmap->shapes[i].color.a);

        printf("      Points:\n");
        for (int j = 0; j < vitmap->shapes[i].numPoints; j++)
        {
            printf("          Point %d: (%.2f, %.2f)\n",
                   j + 1,
                   vitmap->shapes[i].points[j].x,
                   vitmap->shapes[i].points[j].y);
        }
    }
}

Shape* createShape()
{
    Shape* shape = (Shape*)malloc(sizeof(Shape));
    if (shape == NULL) {
        return NULL;
    }
    shape->numPoints = 0;
    shape->color = (Color){0, 0, 0, 255};
    shape->points = NULL;
    return shape;
}

Vitmap* createVitmap()
{
    Vitmap* vitmap = (Vitmap*)malloc(sizeof(Vitmap));
    if (vitmap == NULL) {
        return NULL;
    }
    vitmap->numShapes = 0;
    vitmap->shapes = NULL;
    return vitmap;
}

void addPointToShape(Shape* shape, Vector2 point)
{
    printf("adding point to shape. current count: %d\n", shape->numPoints);
    shape->points = (Vector2*)realloc(shape->points, (shape->numPoints + 1) * sizeof(Vector2));
    shape->points[shape->numPoints] = point;
    shape->numPoints++;
    printf("points in shape:\n");
    for(int i = 0; i < shape->numPoints; i++)
    {
        printf("%f %f\n", shape->points[i].x, shape->points[i].y);
    }
}

void addShapeToVitmap(Vitmap* vitmap)
{
    printf("Shapes pointer before: %p\n", vitmap->shapes);
    // Increment the number of shapes in the vitmap
    vitmap->numShapes++;
  
    // Reallocate memory for the shapes array to accommodate the new shape
    Shape* newShapes = (Shape*)realloc(vitmap->shapes, vitmap->numShapes * sizeof(Shape));
    if (newShapes == NULL) {
        // Handle allocation failure
        vitmap->numShapes = 0; // Reset the number of shapes
        return;
    }
  
    vitmap->shapes = newShapes; // Update the pointer to the reallocated memory
  
    // Create the new shape using createShape function
    Shape* newShape = createShape();
    if (newShape == NULL) {
        // Handle creation failure
        vitmap->numShapes--; // Decrement the number of shapes
        return;
    }
  
    // Add the new shape to the shapes array
    vitmap->shapes[vitmap->numShapes - 1] = *newShape;
}

Vitmap* addVitmapToAnimation(VitmapAnimation* animation, Vitmap vitmap)
{
    animation->vitmaps = (Vitmap*)realloc(animation->vitmaps, (animation->numFrames + 1) * sizeof(Vitmap));
    animation->vitmaps[animation->numFrames] = vitmap;
    animation->numFrames++;
    return &animation->vitmaps[animation->numFrames];
}
