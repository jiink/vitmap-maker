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

//----------------------------------------------------------------------------------
// Controls Functions Declaration
//----------------------------------------------------------------------------------
static void SaveButton();
static void LoadButton();
static void EncodeButton();
static void DecodeButton();
static void LabelButton007();

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
	for (int i = 0; i < numPoints + 1; i++)
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

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
	// Initialization
	//---------------------------------------------------------------------------------------

	Vector2 gridSize = {16, 16};
	Rectangle drawingArea = {424, 40, 592, 592};
	
	Vitmap vitmap;
	initVitmap(&vitmap);

	int screenWidth = 1280;
	int screenHeight = 720;

	InitWindow(screenWidth, screenHeight, "vitmapMaker");

	// vitmapMaker: controls initialization
	//----------------------------------------------------------------------------------
	bool TextmultiBox005EditMode = false;
	char TextmultiBox005Text[128] = "SAMPLE TEXT";
	int modeToggleGroupActive = 0;
	Color ColorPickerValue = {0, 0, 0, 0};
	//----------------------------------------------------------------------------------

	SetTargetFPS(60);
	//--------------------------------------------------------------------------------------
	GuiLoadStyle("lavanda.rgs");

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


		Shape *currentShape = &vitmap.shapes[vitmap.numShapes];
		currentShape->points[currentShape->numPoints] = mouseSnappedPos;
		if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && currentShape->numPoints < MAX_POINTS - 1)
		{
			currentShape->numPoints++;
		}
		if (IsKeyPressed(KEY_BACKSPACE) && currentShape->numPoints > 0)
		{
			currentShape->numPoints--;
		}
		if (IsKeyPressed(KEY_ENTER) && vitmap.numShapes < MAX_SHAPES - 1)
		{
			vitmap.numShapes++;
		}
		if (IsKeyPressed(KEY_DELETE) && vitmap.numShapes > 0)
		{
			vitmap.numShapes--;
		}
		currentShape->color = (Color){ColorPickerValue.r, ColorPickerValue.g, ColorPickerValue.b, 255};

		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();

		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		DrawRectangleRec(drawingArea, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));
		// draw grid dots (gridSize has how many dots to draw in each direction)
		for (int i = 0; i < gridSize.x; i++)
		{
			for (int j = 0; j < gridSize.y; j++)
			{
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
			SaveButton();
		if (GuiButton((Rectangle){168, 24, 120, 24}, "Load..."))
			LoadButton();
		if (GuiButton((Rectangle){24, 408, 120, 24}, "Encode"))
			EncodeButton();
		if (GuiButton((Rectangle){168, 408, 120, 24}, "Decode"))
			DecodeButton();
		if (GuiTextBox((Rectangle){24, 456, 336, 192}, TextmultiBox005Text, 128, TextmultiBox005EditMode))
			TextmultiBox005EditMode = !TextmultiBox005EditMode;
		modeToggleGroupActive = GuiToggleGroup((Rectangle){240, 120, 40, 24}, "VIEW;DRAW", modeToggleGroupActive);
		if (GuiLabelButton((Rectangle){240, 96, 120, 24}, "Modes"))
			LabelButton007();
		GuiGroupBox((Rectangle){drawingArea.x - 16, drawingArea.y - 16, drawingArea.width + 32, drawingArea.height + 32}, "Vitmap View");
		ColorPickerValue = GuiColorPicker((Rectangle){240, 176, 96, 96}, NULL, ColorPickerValue);
		//----------------------------------------------------------------------------------

		EndDrawing();
		//----------------------------------------------------------------------------------
	}

	// De-Initialization
	//--------------------------------------------------------------------------------------
	CloseWindow(); // Close window and OpenGL context
	//--------------------------------------------------------------------------------------

	return 0;
}

//------------------------------------------------------------------------------------
// Controls Functions Definitions (local)
//------------------------------------------------------------------------------------
static void SaveButton()
{
	// TODO: Implement control logic
}
static void LoadButton()
{
	// TODO: Implement control logic
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
