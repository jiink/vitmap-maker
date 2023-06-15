// this GUI was designed using https://raylibtech.itch.io/rguilayout

#define RAYGUI_IMPLEMENTATION
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "include/raylib.h"
#include "include/raygui.h"

#define MAX_POINTS  11      

//----------------------------------------------------------------------------------
// Controls Functions Declaration
//----------------------------------------------------------------------------------
static void SaveButton();
static void LoadButton();
static void EncodeButton();
static void DecodeButton();
static void LabelButton007();

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main()
{
    // Initialization
    //---------------------------------------------------------------------------------------

	Vector2 coords[MAX_POINTS] = {
			(Vector2){ 0.75f, 0.0f },
			(Vector2){ 0.25f, 0.0f },
			(Vector2){ 0.0f, 0.5f },
			(Vector2){ 0.0f, 0.75f },
			(Vector2){ 0.25f, 1.0f},
			(Vector2){ 0.375f, 0.875f},
			(Vector2){ 0.625f, 0.875f},
			(Vector2){ 0.75f, 1.0f},
			(Vector2){ 1.0f, 0.75f},
			(Vector2){ 1.0f, 0.5f},
			(Vector2){ 0.75f, 0.0f}  // Close the poly
		};

		Vector2 points[MAX_POINTS] = { 0 };
    for (int i = 0; i < MAX_POINTS; i++)
    {
        points[i].x = (coords[i].x - 0.5f)*256.0f;
        points[i].y = (coords[i].y - 0.5f)*256.0f;
    }
    

    int screenWidth = 1280;
    int screenHeight = 720;

    InitWindow(screenWidth, screenHeight, "vitmapMaker");

    // vitmapMaker: controls initialization
    //----------------------------------------------------------------------------------
    bool TextmultiBox005EditMode = false;
    char TextmultiBox005Text[128] = "SAMPLE TEXT";
    int modeToggleGroupActive = 0;
    Color ColorPickerValue = { 0, 0, 0, 0 };
    //----------------------------------------------------------------------------------

    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Implement required update logic
        //----------------------------------------------------------------------------------

        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR))); 

			// Draw polygons

			DrawTriangleStrip(points, MAX_POINTS, BLACK);

            // raygui: controls drawing
            //----------------------------------------------------------------------------------
            if (GuiButton((Rectangle){ 24, 24, 120, 24 }, "Save...")) SaveButton(); 
            if (GuiButton((Rectangle){ 168, 24, 120, 24 }, "Load...")) LoadButton(); 
            if (GuiButton((Rectangle){ 24, 408, 120, 24 }, "Encode")) EncodeButton(); 
            if (GuiButton((Rectangle){ 168, 408, 120, 24 }, "Decode")) DecodeButton(); 
            if (GuiTextBox((Rectangle){ 24, 456, 336, 192 }, TextmultiBox005Text, 128, TextmultiBox005EditMode)) TextmultiBox005EditMode = !TextmultiBox005EditMode;
            modeToggleGroupActive = GuiToggleGroup((Rectangle){ 240, 120, 40, 24 }, "VIEW;DRAW", modeToggleGroupActive);
            if (GuiLabelButton((Rectangle){ 240, 96, 120, 24 }, "Modes")) LabelButton007(); 
            GuiGroupBox((Rectangle){ 408, 24, 624, 624 }, "Vitmap View");
            ColorPickerValue = GuiColorPicker((Rectangle){ 240, 176, 96, 96 }, NULL, ColorPickerValue);
            //----------------------------------------------------------------------------------

        EndDrawing();
        //----------------------------------------------------------------------------------
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
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

