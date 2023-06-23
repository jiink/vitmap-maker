// this GUI was designed using https://raylibtech.itch.io/rguilayout

#define RAYGUI_IMPLEMENTATION
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "include/raylib.h"
#include "include/raygui.h"
#include "include/tesselator.h"

#define MAX_SHAPES 24
#define MAX_POINTS 24

// Shapes are closed polygons
typedef struct Shape
{
    Vector2 points[MAX_POINTS];
    int numPoints;
    Color color;
} Shape;

typedef struct Vitmap
{
    Shape shapes[MAX_SHAPES];
    int numShapes;
} Vitmap;


//----------------------------------------------------------------------------------
// Controls Functions Declaration
//----------------------------------------------------------------------------------
static void SaveButton(Vitmap* vitmap, const char* name);
static void LoadButton(Vitmap* vitmapOut, const char* name);
static void EncodeButton();
static void DecodeButton();
static void LabelButton007();

int isPointInPoly(int nvert, float *vertx, float *verty, float testx, float testy)
{
  int i, j, c = 0;
  for (i = 0, j = nvert-1; i < nvert; j = i++) {
    if ( ((verty[i]>testy) != (verty[j]>testy)) &&
     (testx < (vertx[j]-vertx[i]) * (testy-verty[i]) / (verty[j]-verty[i]) + vertx[i]) )
       c = !c;
  }
  return c;
}

bool isPointInTri(Vector2 p, Vector2 p0, Vector2 p1, Vector2 p2) {
    float A = 1.0f/2.0f * (-p1.y * p2.x + p0.y * (-p1.x + p2.x) + p0.x * (p1.y - p2.y) + p1.x * p2.y);
    float sign = A < 0.0f ? -1.0f : 1.0f;
    float s = (p0.y * p2.x - p0.x * p2.y + (p2.y - p0.y) * p.x + (p0.x - p2.x) * p.y) * sign;
    float t = (p0.x * p1.y - p0.y * p1.x + (p0.y - p1.y) * p.x + (p1.x - p0.x) * p.y) * sign;
    
    return s > 0.0f && t > 0.0f && (s + t) < 2.0f * A * sign;
}


void initVitmap(Vitmap *vitmap)
{
    vitmap->numShapes = 0;
    for (int i = 0; i < MAX_SHAPES; i++)
    {
        vitmap->shapes[i].numPoints = 0;
        for (int j = 0; j < MAX_POINTS; j++)
        {
            vitmap->shapes[i].points[j] = (Vector2){0, 0};
            vitmap->shapes[i].color = (Color){0, 0, 0, 0};
        }
    }
}

void drawTesselation(TESStesselator* tesselator, Color color)
{
    int vertexCount = tessGetVertexCount(tesselator);
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
    Vector2* points = (Vector2*)&shape->points;
    Vector2 transformedPoints[MAX_POINTS];
    int numPoints = shape->numPoints;
    Color color = shape->color;
    // todo: figure out this +1 stuff
    for (int i = 0; i <= numPoints; i++)
    {
        transformedPoints[i].x = position.x + points[i].x * scale.x;
        transformedPoints[i].y = position.y + points[i].y * scale.y;
    }
    
    DrawLineStrip(transformedPoints, numPoints + 1, color);
    DrawLineV(transformedPoints[numPoints], transformedPoints[0], color);

    TESStesselator *tesselator = tessNewTess(NULL);
    tessSetOption(tesselator, TESS_CONSTRAINED_DELAUNAY_TRIANGULATION, 1);
    tessAddContour(tesselator, 2, transformedPoints, sizeof(Vector2), numPoints + 1);
    tessTesselate(tesselator, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, NULL);
    drawTesselation(tesselator, color);
    tessDeleteTess(tesselator);
}

void drawVitmap(Vitmap *vitmap, Vector2 position, Vector2 scale)
{
    for (int i = 0; i < vitmap->numShapes + 1; i++)
    {
        Shape *shape = &vitmap->shapes[i];
        drawShape(shape, position, scale);
    }
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

size_t writeOutVitmap(Vitmap* vitmap, const char* filename)
{
    FILE* f = fopen(filename, "wb");
    return fwrite(vitmap, sizeof(Vitmap), 1, f);
}

size_t readInVitmap(Vitmap* vitmapOut, const char* filename)
{
    FILE* f = fopen(filename, "rb");
    return fread(vitmapOut, sizeof(Vitmap), 1, f);
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

    Vector2 gridSize = {16, 16};
    Rectangle drawingArea = {424, 40, 592, 592};
    
    Vitmap vitmap;
    initVitmap(&vitmap);
    if (fileToLoad != NULL)
    {
        readInVitmap(&vitmap, fileToLoad);
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
    bool TextmultiBox005EditMode = false;
    char TextmultiBox005Text[128] = "SAMPLE TEXT";
    int modeToggleGroupActive = 0;
    Color ColorPickerValue = {0, 0, 0, 0};
    Color BgGridColor = {50, 50, 50, 255};
    float resolution = 2.0;

    //----------------------------------------------------------------------------------

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------
    GuiLoadStyle("lavanda.rgs");

    PlaySound(slidingSound);


    // Main game loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Implement required update logic
        //----------------------------------------------------------------------------------

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

        Shape *currentShape = &vitmap.shapes[vitmap.numShapes];
        

        if (isMouseInRect)
        {
            currentShape->points[currentShape->numPoints] = mouseSnappedPos;
        }
        else if (currentShape->numPoints > 1)
        {
            currentShape->points[currentShape->numPoints] = currentShape->points[currentShape->numPoints - 1];
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && currentShape->numPoints < MAX_POINTS - 1 && isMouseInRect)
        {
            currentShape->numPoints++;
            PlaySound(pressSound);
        }
        if (IsKeyPressed(KEY_BACKSPACE) && currentShape->numPoints > 0)
        {
            currentShape->numPoints--;
        }
        if (IsKeyPressed(KEY_ENTER) && vitmap.numShapes < MAX_SHAPES - 1)
        {
            vitmap.numShapes++;
            PlaySound(snapSound);
        }
        if (IsKeyPressed(KEY_DELETE) && vitmap.numShapes > 0)
        {
            vitmap.numShapes--;
        }

		if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) && isMouseInRect)
        {
			// See if there is this point is over a polygon
			for (int i = 0; i < vitmap.numShapes; i++)
			{
				int numVerts = vitmap.shapes[i].numPoints;
				float xVerts[MAX_POINTS];
				float yVerts[MAX_POINTS];
				for (int j = 0; j < numVerts + 1; j++)
				{
					xVerts[j] = vitmap.shapes[i].points[j].x;
					yVerts[j] = vitmap.shapes[i].points[j].y;
				}
				int result = isPointInPoly(numVerts, xVerts, yVerts, mouseDrawAreaPos.x, mouseDrawAreaPos.y);
				printf("result: %d\n", result);
			}
		}

        currentShape->color = (Color){ColorPickerValue.r, ColorPickerValue.g, ColorPickerValue.b, 255};


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
        drawVitmap(&vitmap, (Vector2){drawingArea.x, drawingArea.y}, (Vector2){drawingArea.width / gridSize.x, drawingArea.height / gridSize.y});
        // rlEnableBackfaceCulling();

        DrawText(TextFormat("pts: %d", currentShape->numPoints), 24, 456, 20, BLACK);
        DrawText(TextFormat("mouse draw area pos: %f, %f", mouseDrawAreaPos.x, mouseDrawAreaPos.y), 24, 480, 20, BLACK);

        // raygui: controls drawing
        //----------------------------------------------------------------------------------
        if (GuiButton((Rectangle){24, 24, 120, 24}, "Save..."))
        {
            SaveButton(&vitmap, "MyVitmap.vmp");
            PlaySound(clickSound);
        }
        if (GuiButton((Rectangle){168, 24, 120, 24}, "Load..."))
        {
            LoadButton(&vitmap, "MyVitmap.vmp");
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
        if (GuiTextBox((Rectangle){24, 456, 336, 192}, TextmultiBox005Text, 128, TextmultiBox005EditMode))
            TextmultiBox005EditMode = !TextmultiBox005EditMode;
        modeToggleGroupActive = GuiToggleGroup((Rectangle){240, 120, 40, 24}, "VIEW;DRAW", modeToggleGroupActive);
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
        DrawText(TextFormat("Resolution: %i", realResolution), 60, 330, 20, BLACK);
        DrawText(TextFormat("Slide volume: %f", targetVol), 60, 360, 20, BLACK);
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
    writeOutVitmap(vitmap, name);
}
static void LoadButton(Vitmap* vitmapOut, const char* name)
{
    readInVitmap(vitmapOut, name);
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
