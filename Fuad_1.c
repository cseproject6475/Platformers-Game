#include "raylib.h"
#define ScreenWidth 800
#define ScreenHeight 500
void DrawGridBackground(int width, int height, int gridSize, Color lineColor) {
    for (int x = 0; x < width; x += gridSize) {
        DrawLine(x, 0, x, height, lineColor);
    }
    for (int y = 0; y < height; y += gridSize) {
        DrawLine(0, y, width, y, lineColor);
    }
}
int main(void){
    
    InitWindow(ScreenWidth,ScreenHeight,"Platformer Game");
    SetTargetFPS(60);
    Color bgColor = { 244, 244, 246, 255 };      
    Color gridColor = { 180, 190, 205, 80 };
    while(!WindowShouldClose()){
        BeginDrawing();
        ClearBackground(bgColor);
        DrawGridBackground(ScreenWidth,ScreenHeight,40,gridColor);
        DrawRectangle(340,90,300,40,WHITE);
        DrawRectangleGradientEx((Rectangle){340,90,300,40},RED,GREEN,WHITE,GREEN);
        DrawText("Welcome to Game!",405,100,20,BLACK);
        DrawLine(200,0,200,500,BLUE);
        DrawRectangle(20,30,160,40,GRAY);
        DrawRectangleGradientEx((Rectangle){20,30,160,40},GRAY,RED,MAGENTA,GREEN);
        DrawText("SCORE",60,40,20,BLACK);
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
