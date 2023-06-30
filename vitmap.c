#include "vitmap.h"
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>

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
    // printf("points in shape:\n");
    // for(int i = 0; i < shape->numPoints; i++)
    // {
    //     printf("%f %f\n", shape->points[i].x, shape->points[i].y);
    // }
}

void removePointFromShape(Shape *shape, Vector2 *point)
{
    // See if shape has this pointer
    int index = -1;
    for (int i = 0; i < shape->numPoints; i++)
    {
        if (&shape->points[i] == point)
        {
            index = i;
            break;
        }
    }
    if (index == -1)
    {
        return;
    }
    // Remove the point
    for (int i = index; i < shape->numPoints - 1; i++)
    {
        shape->points[i] = shape->points[i + 1];
    }
    shape->numPoints--;
    // Reallocate memory
    shape->points = (Vector2*)realloc(shape->points, shape->numPoints * sizeof(Vector2));
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

void saveVitmapToFile(Vitmap* vitmap, const char* filename)
{
    // Open the file in binary write mode
    FILE* file = fopen(filename, "wb");
    if (file == NULL)
    {
        printf("Failed to open file for writing.\n");
        return;
    }
    
    // Write the number of shapes in the Vitmap
    fwrite(&(vitmap->numShapes), sizeof(int), 1, file);
    
    // Write each shape in the Vitmap
    for (int i = 0; i < vitmap->numShapes; i++)
    {
        Shape* shape = &(vitmap->shapes[i]);
        
        // Write the number of points in the shape
        fwrite(&(shape->numPoints), sizeof(int), 1, file);
        
        // Write the points of the shape
        fwrite(shape->points, sizeof(Vector2), shape->numPoints, file);
        
        // Write the color of the shape
        fwrite(&(shape->color), sizeof(Color), 1, file);
    }
    
    // Close the file
    fclose(file);
    
    printf("Vitmap saved successfully.\n");
}

Vitmap loadVitmapFromFile(const char* filename)
{
    Vitmap vitmap = *createVitmap();

    // addShapeToVitmap(&vitmap);
    // addPointToShape(&vitmap.shapes[0], (Vector2){0.0f, 0.0f});
    // addPointToShape(&vitmap.shapes[0], (Vector2){1.0f, 1.0f});
    // addPointToShape(&vitmap.shapes[0], (Vector2){0.0f, 1.0f});

    // printVitmap(&vitmap);
    
    // Open the file in binary read mode
    FILE* file = fopen(filename, "rb");
    if (file == NULL)
    {
        printf("Failed to open file for reading.\n");
        return vitmap;
    }
    
    // Read the number of shapes in the Vitmap
    int numShapesInTheFile = 0;
    fread(&numShapesInTheFile, sizeof(int), 1, file);
    printf("Number of shapes: %d\n", vitmap.numShapes);
    // Read each shape in the Vitmap
    for (int i = 0; i < numShapesInTheFile; i++)
    {
        addShapeToVitmap(&vitmap);
        Shape* shape = &(vitmap.shapes[vitmap.numShapes - 1]);
        
        // Read the number of points in the shape
        int numPointsInHere = 0;
        fread(&(numPointsInHere), sizeof(int), 1, file);
        for (int j = 0; j < numPointsInHere; j++)
        {
            Vector2 pointToAdd = {0.0f, 0.0f};
            fread(&pointToAdd, sizeof(Vector2), 1, file);
            addPointToShape(shape, pointToAdd);
        }
        
        // Read the color of the shape
        fread(&(shape->color), sizeof(Color), 1, file);
    }
    
    // Close the file
    fclose(file);
    
    printf("Vitmap loaded successfully.\n");
    
    return vitmap;
}
