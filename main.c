// this GUI was designed using https://raylibtech.itch.io/rguilayout

#define RAYGUI_IMPLEMENTATION
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "include/raylib.h"
#include "include/raygui.h"
#include "include/tesselator.h"

#define MAX_SHAPES 12
#define MAX_POINTS 12

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
	// TESStesselator* tesselator;
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
			// vitmap->shapes[i].tesselator = tessNewTess(NULL);
			// tessSetOption(vitmap->shapes[i].tesselator, TESS_CONSTRAINED_DELAUNAY_TRIANGULATION, 1);
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

void drawShape(Shape *shape)
{
	// For now just draw a line between every point and connect the last to the first
	DrawLineStrip(shape->points, shape->numPoints + 1, shape->color);
	DrawLineV(shape->points[shape->numPoints], shape->points[0], shape->color);

	TESStesselator *tesselator = tessNewTess(NULL);
	tessSetOption(tesselator, TESS_CONSTRAINED_DELAUNAY_TRIANGULATION, 1);
	tessAddContour(tesselator, 2, shape->points, sizeof(Vector2), shape->numPoints + 1);
	tessTesselate(tesselator, TESS_WINDING_ODD, TESS_POLYGONS, 3, 2, NULL);
	drawTesselation(tesselator, shape->color);
	tessDeleteTess(tesselator);
}

void drawVitmap(Vitmap *vitmap)
{
	for (int i = 0; i < vitmap->numShapes + 1; i++)
	{
		Shape *shape = &vitmap->shapes[i];
		drawShape(shape);
	}
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
	// Initialization
	//---------------------------------------------------------------------------------------

	Vector2 gridSize = {32, 32};
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
		Shape *currentShape = &vitmap.shapes[vitmap.numShapes];
		currentShape->points[currentShape->numPoints] = GetMousePosition();
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
		currentShape->color = (Color){ColorPickerValue.r, ColorPickerValue.g, ColorPickerValue.b, 255};

		// Draw
		//----------------------------------------------------------------------------------
		BeginDrawing();

		ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

		DrawRectangleRec(drawingArea, GetColor(GuiGetStyle(DEFAULT, LINE_COLOR)));


		rlDisableBackfaceCulling();
		drawVitmap(&vitmap);
		// rlEnableBackfaceCulling();

		DrawText(TextFormat("pts: %d", currentShape->numPoints), 24, 456, 20, BLACK);

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
