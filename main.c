// this GUI was designed using https://raylibtech.itch.io/rguilayout

#define RAYGUI_IMPLEMENTATION
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "include/raylib.h"
#include "include/raygui.h"
#include "include/tesselator.h"
#include "vitmap.h"

typedef enum Tool
{
    TOOL_DRAW,
    TOOL_SELECT,
    TOOL_EDIT,
    TOOL_ERASE,
    TOOL_PAN,
    TOOL_MAX
} Tool;

const char* toolNames[TOOL_MAX] = {
    "Draw",
    "Select",
    "Erase",
    "Pan"
};

// Variable definitions

Vector2 gridSize = {16, 16};
Rectangle drawingArea = {424, 40, 592, 592};

//----------------------------------------------------------------------------------
// Controls Functions Declaration
//----------------------------------------------------------------------------------
static void SaveButton(Vitmap* vitmap, const char* name);
static void LoadButton(Vitmap* vitmapOut, const char* name);
static void EncodeButton();
static void DecodeButton();
static void LabelButton007();

bool isPointInPoly(int nvert, float *vertx, float *verty, float testx, float testy)
{
    int i, j = 0;
    bool c = false;
    for (i = 0, j = nvert-1; i < nvert; j = i++) {
        if ( ((verty[i]>testy) != (verty[j]>testy)) &&
        (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
        c = !c;
    }
    return c;
}


void drawTesselation(TESStesselator* tesselator, Color color)
{
    //int vertexCount = tessGetVertexCount(tesselator);
    const TESSreal *vertices = tessGetVertices(tesselator);
    // Print out vertices
    //printf("vertices:\n");
    // for (int i = 0; i < vertexCount; i++)
    // {
    // 	printf("%f %f\n", vertices[i * 2], vertices[i * 2 + 1]);
    // }
    int indexCount = tessGetElementCount(tesselator) * 3;
    const TESSindex *indices = tessGetElements(tesselator);
    // Print out indices
    // printf("indices:\n");
    // for (int i = 0; i < indexCount; i++)
    // {
    // 	printf("%d\n", indices[i]);
    // }
    // Draw tesselator
    for (int i = 0; i < indexCount; i += 3)
    {
        DrawTriangle(
            (Vector2){vertices[indices[i] * 2], vertices[indices[i] * 2 + 1]},
            (Vector2){vertices[indices[i + 1] * 2], vertices[indices[i + 1] * 2 + 1]},
            (Vector2){vertices[indices[i + 2] * 2], vertices[indices[i + 2] * 2 + 1]},
            color);
    }
}

void drawShape(Shape *shape, Vector2 position, Vector2 scale)
{
    Vector2* points = shape->points;
    int numPoints = shape->numPoints;
    Vector2* transformedPoints = calloc(numPoints, sizeof(Vector2));
    Color color = shape->color;
    for (int i = 0; i < numPoints; i++)
    {
        transformedPoints[i].x = position.x + points[i].x * scale.x;
        transformedPoints[i].y = position.y + points[i].y * scale.y;
    }

    TESStesselator *tesselator = tessNewTess(NULL);
    tessSetOption(tesselator, TESS_CONSTRAINED_DELAUNAY_TRIANGULATION, 1);
    tessAddContour(tesselator, 2, transformedPoints, sizeof(Vector2), numPoints);
    tessTesselate(tesselator, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, NULL);
    drawTesselation(tesselator, color);
    tessDeleteTess(tesselator);
    free(transformedPoints);

    // DrawLineStrip(transformedPoints, numPoints, WHITE);
    // DrawLineV(transformedPoints[numPoints - 1], transformedPoints[0], WHITE);
}

void drawShapeOutline(Shape* shape, Vector2 position, Vector2 scale, int pattern)
{
    Vector2* points = shape->points;
    int numPoints = shape->numPoints;
    Vector2* transformedPoints = calloc(numPoints, sizeof(Vector2));
    Color color = WHITE;
    switch (pattern)
    {
        case 0:
            color = ColorFromHSV(GetTime() * 100, 1, 1);
            break;
        case 1:
            color = ColorFromHSV(0, 0, sinf(GetTime() * 10) * 0.4 + 0.5);
            break;
    }
    
    for (int i = 0; i < numPoints; i++)
    {
        transformedPoints[i].x = position.x + points[i].x * scale.x;
        transformedPoints[i].y = position.y + points[i].y * scale.y;
    }
    
    DrawLineStrip(transformedPoints, numPoints, color);
    DrawLineV(transformedPoints[numPoints-1], transformedPoints[0], color);
    free(transformedPoints);
}


void drawVitmap(Vitmap *vitmap, Vector2 position, Vector2 scale)
{
    for (int i = 0; i < vitmap->numShapes; i++)
    {
        Shape* shape = &vitmap->shapes[i];
        drawShape(shape, position, scale);
    }
}

void drawPlus(Vector2 position)
{
    DrawLineV((Vector2){position.x - 10, position.y}, (Vector2){position.x + 10, position.y}, WHITE);
    DrawLineV((Vector2){position.x, position.y - 10}, (Vector2){position.x, position.y + 10}, WHITE);
    DrawLineV((Vector2){position.x - 9, position.y + 1}, (Vector2){position.x + 9, position.y + 1}, BLACK);
    DrawLineV((Vector2){position.x + 1, position.y - 9}, (Vector2){position.x + 1, position.y + 9}, BLACK);
}

float clamp(float value, float min, float max)
{
    if (value < min)
    {
        return min;
    }
    else if (value > max)
    {
        return max;
    }
    else
    {
        return value;
    }
}

float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
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

int getShapesUnderPos(Vitmap* vitmap, Vector2 pos, Shape* shapesUnderMouseOut[111])
{
    int shapesUnderMouseIndex = 0;
    for (int i = 0; i < vitmap->numShapes; i++)
    {
        int numVerts = vitmap->shapes[i].numPoints + 1;
        float* xVerts = calloc(numVerts, sizeof(float));
        float* yVerts = calloc(numVerts, sizeof(float));
        for (int j = 0; j < numVerts; j++)
        {
            xVerts[j] = vitmap->shapes[i].points[j].x;
            yVerts[j] = vitmap->shapes[i].points[j].y;
        }
        if (isPointInPoly(numVerts, xVerts, yVerts, pos.x, pos.y))
        {
            shapesUnderMouseOut[shapesUnderMouseIndex] = &vitmap->shapes[i];
            shapesUnderMouseIndex++;
        }
    }
    int shapesFound = shapesUnderMouseIndex;
    return shapesFound;
}

Shape* getShapeUnderPos(Vitmap* vitmap, Vector2 pos)
{
    const int max_shapes = 111;
    Shape* shapesUnderPos[111];
    // initialize them to null
    for (int i = 0; i < max_shapes; i++)
    {
        shapesUnderPos[i] = NULL;
    }
    getShapesUnderPos(vitmap, pos, shapesUnderPos);
    // print out the results
    for (int i = 0; i < max_shapes; i++)
    {
        if (shapesUnderPos[i] != NULL)
        {
            printf("Shape %x is under the mouse!\n", shapesUnderPos[i]);
        }
    }
    // Find and return the one on top (The last one in the array)
    for (int i = max_shapes - 1; i >= 0; i--)
    {
        if (shapesUnderPos[i] != NULL)
        {
            return shapesUnderPos[i];
        }
    }
    printf("No shapes here.\n");
    return NULL;
}

Vector2 vector2Transform(Vector2 input, Vector2 pos, Vector2 scale)
{
    Vector2 output = {0.0f, 0.0f};
    output.x = pos.x + input.x * scale.x;
    output.y = pos.y + input.y * scale.y;
    return output;
}

Vector2 vitToScreenCoord(Vector2 vitSpaceCoord)
{
    return vector2Transform(vitSpaceCoord, (Vector2){drawingArea.x, drawingArea.y}, (Vector2){drawingArea.width / gridSize.x, drawingArea.height / gridSize.y});
}

bool checkCollisionPointCircle(Vector2 point, Vector2 center, float radius)
{
    return (point.x - center.x) * (point.x - center.x) + (point.y - center.y) * (point.y - center.y) < radius * radius;
}


//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    // If the filepath is given as the second argument (argv[1]),
    // write down the path to load from it soon.
    char* fileToLoad = NULL;
    if (argc > 1)
    {
        fileToLoad = argv[1];
    }
    
    // Initialization
    //---------------------------------------------------------------------------------------

    
    
    Vitmap* currentVitmap = createVitmap();
    Shape* currentShape = NULL;
    if (fileToLoad != NULL)
    {
        Vitmap loadedVmp = loadVitmapFromFile(fileToLoad);
        currentVitmap = &loadedVmp;
    }
    

    int screenWidth = 1280;
    int screenHeight = 720;

    Vector2 lastMouseDrawAreaPos = {0.0, 0.0};

    InitWindow(screenWidth, screenHeight, "vitmapMaker");

    InitAudioDevice();

    SetMasterVolume(10.0);
    Sound clickSound = LoadSound("assets/click.wav");
    Sound pressSound = LoadSound("assets/press.wav");
    Sound snapSound = LoadSound("assets/snap.wav");
    Sound slidingSound = LoadSound("assets/firing.wav");

    float realVol = 0.0;

    // vitmapMaker: controls initialization
    //----------------------------------------------------------------------------------
    bool FilePathEditMode = false;
    char FilePathText[128] = "MyVitmap.vmp";
    Color ColorPickerValue = {0, 0, 0, 0};
    Color BgGridColor = {50, 50, 50, 255};
    float resolution = 3.0;

    //----------------------------------------------------------------------------------

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------
    GuiLoadStyle("lavanda.rgs");

    PlaySound(slidingSound);

    
    Tool currentTool = TOOL_DRAW;
    bool isEditingShape = false;

    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update
        //currentVitmap = &vitmapAnim.vitmaps[vitmapAnim.currentFrame];
        //currentShape = &currentVitmap->shapes[currentVitmap->numShapes - 1];

        // Get mouse coords in drawingArea coords
        Vector2 mouseDrawAreaPos = 
        {
            (GetMousePosition().x - drawingArea.x) / (drawingArea.width / gridSize.x),
            (GetMousePosition().y - drawingArea.y) / (drawingArea.height / gridSize.y),
        };
        // Must stay between 0 and gridSize
        mouseDrawAreaPos.x = clamp(mouseDrawAreaPos.x, 0, gridSize.x);
        mouseDrawAreaPos.y = clamp(mouseDrawAreaPos.y, 0, gridSize.y);
        Vector2 mouseSnappedPos = 
        {
            roundf(mouseDrawAreaPos.x),
            roundf(mouseDrawAreaPos.y)
        };

        bool isMouseInRect = CheckCollisionPointRec(GetMousePosition(), drawingArea);


        // VOLUME CONTROL FOR SLIIDEE!!!	---------------------------
        float mouse_dist = (fabs(mouseDrawAreaPos.x - lastMouseDrawAreaPos.x) + fabs(mouseDrawAreaPos.y - lastMouseDrawAreaPos.y));
        float targetVol = 1.0 * mouse_dist;

        if (!isMouseInRect)
        {
            targetVol = 0.0;
        }
        realVol = clamp(
            lerp(realVol, targetVol, 0.4),
            0.0,
            1.0
        );
        SetSoundVolume(
            slidingSound,
            realVol
        );
        // ------------------------------------------------------------
        
        // if (IsKeyPressed(KEY_BACKSPACE) && currentShape->numPoints > 0)
        // {
        //     currentShape->numPoints--;
        // }
        // if (IsKeyPressed(KEY_ENTER) && currentVitmap->numShapes < MAX_SHAPES - 1)
        // {
        //     currentVitmap->numShapes++;
        //     currentShape = &currentVitmap->shapes[currentVitmap->numShapes];
        //     PlaySound(snapSound);
        // }
        // if (IsKeyPressed(KEY_DELETE) && currentVitmap->numShapes > 0)
        // {
        //     currentVitmap->numShapes--;
        // }
        if (IsKeyPressed(KEY_P))
        {
            printVitmap(currentVitmap);
        }

        

        if (currentShape != NULL)
        {
            currentShape->color = (Color){ColorPickerValue.r, ColorPickerValue.g, ColorPickerValue.b, 255};
        }


        // Loop sliding sound
        if (!IsSoundPlaying(slidingSound)) PlaySound(slidingSound);


        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        DrawRectangleRec(drawingArea, BgGridColor);
        // draw grid dots AND checkerboard (gridSize has how many dots to draw in each direction)
        for (int i = 0; i < gridSize.x; i++)
        {
            for (int j = 0; j < gridSize.y; j++)
            {
                // Checkerboard on every other juicy tile
                if ((i - j) % 2 == 0)
                {
                    DrawRectangle(
                        drawingArea.x + drawingArea.width / gridSize.x * i,
                        drawingArea.y + drawingArea.height / gridSize.y * j,
                        drawingArea.width / gridSize.x,
                        drawingArea.height / gridSize.y,
                        (Color){BgGridColor.r + 10, BgGridColor.g + 10, BgGridColor.b + 10, 255} //Find a better way to do this😨
                    );
                }
                DrawRectangle(drawingArea.x + drawingArea.width / gridSize.x * i, drawingArea.y + drawingArea.height / gridSize.y * j, 3, 3, BLACK);
            }	
        }

        rlDisableBackfaceCulling();
        if (currentShape != NULL)
        {
            drawVitmap(currentVitmap, (Vector2){drawingArea.x, drawingArea.y}, (Vector2){drawingArea.width / gridSize.x, drawingArea.height / gridSize.y});
            //drawShape(currentShape, (Vector2){drawingArea.x, drawingArea.y}, (Vector2){drawingArea.width / gridSize.x, drawingArea.height / gridSize.y});
            if (isEditingShape)
            {
                drawShapeOutline(currentShape, (Vector2){drawingArea.x, drawingArea.y}, (Vector2){drawingArea.width / gridSize.x, drawingArea.height / gridSize.y}, 0);
            }
            else
            {
                drawShapeOutline(currentShape, (Vector2){drawingArea.x, drawingArea.y}, (Vector2){drawingArea.width / gridSize.x, drawingArea.height / gridSize.y}, 1);
            }
        }
        // rlEnableBackfaceCulling();

        if (currentShape != NULL) DrawText(TextFormat("pts: %d", currentShape->numPoints), 24, 456, 20, BLACK);
        DrawText(TextFormat("mouse draw area pos: %f, %f", mouseDrawAreaPos.x, mouseDrawAreaPos.y), 24, 480, 20, BLACK);

        // raygui: controls drawing
        //----------------------------------------------------------------------------------
        if (GuiButton((Rectangle){24, 24, 120, 24}, "Save Vitmap..."))
        {
            SaveButton(currentVitmap, FilePathText);
            PlaySound(clickSound);
        }
        if (GuiButton((Rectangle){168, 24, 120, 24}, "Load Vitmap..."))
        {
            LoadButton(currentVitmap, FilePathText);
            PlaySound(clickSound);
        }
        if (GuiButton((Rectangle){24, 48, 120, 24}, "Save Animation..."))
        {
            
            PlaySound(clickSound);
        }
        if (GuiButton((Rectangle){168, 48, 120, 24}, "Load Animation..."))
        {
            
            PlaySound(clickSound);
        }
        if (GuiButton((Rectangle){24, 408, 120, 24}, "Encode"))
        {
            EncodeButton();
            PlaySound(clickSound);
        }
        if (GuiButton((Rectangle){168, 408, 120, 24}, "Decode"))
        {
            DecodeButton();
            PlaySound(clickSound);
        }
        if (GuiTextBox((Rectangle){24, 4, 336, 20}, FilePathText, 128, FilePathEditMode))
            FilePathEditMode = !FilePathEditMode;
        currentTool = GuiToggleGroup((Rectangle){240, 120, 40, 24}, "DRAW;SELECT;EDIT;ERASE;PAN", currentTool);
        if (GuiLabelButton((Rectangle){240, 96, 120, 24}, "Modes"))
        {
            LabelButton007();
        }
        GuiGroupBox((Rectangle){drawingArea.x - 16, drawingArea.y - 16, drawingArea.width + 32, drawingArea.height + 32}, "Vitmap View");
        ColorPickerValue = GuiColorPicker((Rectangle){240, 176, 96, 96}, NULL, ColorPickerValue);

        BgGridColor = GuiColorPicker((Rectangle){60, 176, 96, 96}, NULL, BgGridColor);
        DrawText(TextFormat("Background color"), 60, 146, 20, BLACK);

        // Resolution changer o-matic
        resolution = GuiSlider((Rectangle){60, 300, 96, 20}, "0", "32", resolution, 1.0, 5.0);
        int realResolution = pow(2.0, roundf(resolution));
        gridSize = (Vector2){realResolution, realResolution};

        // Animation frame slider
        // if (vitmapAnim.numFrames > 1)
        // {
        //     vitmapAnim.currentFrame = (int)roundf(GuiSlider((Rectangle){500, 640, 300, 20}, "0", TextFormat("%d", vitmapAnim.numFrames - 1), vitmapAnim.currentFrame, 0.0, (float)(vitmapAnim.numFrames - 1)));
        // }
        // // Animation add frame button
        // if (GuiButton((Rectangle){816, 640, 20, 20}, "+"))
        // {
        //     vitmapAnim.numFrames++;
        // }

        
        switch (currentTool)
        {
            case TOOL_DRAW:
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && isMouseInRect)
                {
                    if (isEditingShape)
                    {
                        addPointToShape(currentShape, mouseSnappedPos);
                    }
                    else
                    {
                        addShapeToVitmap(currentVitmap);
                        currentShape = &currentVitmap->shapes[currentVitmap->numShapes - 1];
                        isEditingShape = true;
                        addPointToShape(currentShape, mouseSnappedPos);
                    }
                    PlaySound(pressSound);
                }
                // Drop it
                if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && isMouseInRect)
                {
                    isEditingShape = false;
                }
                drawPlus(vitToScreenCoord(mouseSnappedPos));
                break;
            case TOOL_SELECT:
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && isMouseInRect)
                {
                    Shape* shapeUnderMouse = getShapeUnderPos(currentVitmap, mouseDrawAreaPos);
                    if (shapeUnderMouse != NULL)
                    {
                        currentShape = shapeUnderMouse;
                        ColorPickerValue = currentShape->color;
                    }
                    PlaySound(clickSound);
                }
                break;
            case TOOL_EDIT:
                // Draw a dot at each vertex of the current shape
                if (currentShape != NULL)
                {
                    for (int i = 0; i < currentShape->numPoints; i++)
                    {
                        Vector2 point = currentShape->points[i];
                        DrawCircle(
                            drawingArea.x + (point.x * drawingArea.width / gridSize.x),
                            drawingArea.y + (point.y * drawingArea.height / gridSize.y),
                            2.0,
                            GREEN
                        );
                    }
                }
                // Click near a dot and drag it to change its position
                if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && isMouseInRect)
                {
                    if (currentShape != NULL)
                    {
                        for (int i = 0; i < currentShape->numPoints; i++)
                        {
                            Vector2 point = currentShape->points[i];
                            if (checkCollisionPointCircle(mouseDrawAreaPos, point, 1.0))
                            {
                                if (IsKeyDown(KEY_LEFT_CONTROL))
                                {
                                    currentShape->points[i] = mouseDrawAreaPos;
                                }
                                else
                                {
                                    currentShape->points[i] = mouseSnappedPos;
                                }
                                break;
                            }
                        }
                    }
                }
                break;
        }


        DrawText(TextFormat("Resolution: %i", realResolution), 60, 330, 20, WHITE);
        DrawText(TextFormat("Slide volume: %f", targetVol), 60, 360, 20, WHITE);
        DrawText(TextFormat("Tool: %s", toolNames[currentTool]), 60, 390, 20, WHITE);
        //DrawText(TextFormat("Frame: %i", vitmapAnim.currentFrame), 500, 620, 20, WHITE);
        //----------------------------------------------------------------------------------

        EndDrawing();
        //----------------------------------------------------------------------------------


        lastMouseDrawAreaPos = mouseDrawAreaPos;

    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadSound(clickSound);
    UnloadSound(pressSound);
    UnloadSound(slidingSound);
    UnloadSound(snapSound);

    CloseAudioDevice();

    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Controls Functions Definitions (local)
//------------------------------------------------------------------------------------
static void SaveButton(Vitmap* vitmap, const char* name)
{
    saveVitmapToFile(vitmap, name);
}
static void LoadButton(Vitmap* vitmapOut, const char* name)
{
    *vitmapOut = loadVitmapFromFile(name);
}
static void EncodeButton()
{
    // TODO: Implement control logic
}
static void DecodeButton()
{
    // TODO: Implement control logic
}
static void LabelButton007()
{
    // TODO: Implement control logic
}
