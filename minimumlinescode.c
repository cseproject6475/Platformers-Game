#include "raylib.h"
#include <stdlib.h>

#define PLATFORM_COUNT 10

typedef struct { Vector2 pos; } Platform;

int main(void) {
    InitWindow(400, 600, "Doodle Jump");
    SetTargetFPS(60);

    Vector2 player = { 200, 400 };
    float velY = 0;
    int score = 0;
    bool gameOver = false;
    Platform platforms[PLATFORM_COUNT];

    // Platform setup lambda/closure equivalent
    #define RESET_GAME() do { \
        player = (Vector2){ 200, 400 }; \
        velY = -8.0f; score = 0; gameOver = false; \
        for (int i = 0; i < PLATFORM_COUNT; i++) \
            platforms[i].pos = (Vector2){ rand() % 320, i * 60.0f }; \
    } while(0)

    RESET_GAME();

    while (!WindowShouldClose()) {
        if (!gameOver) {
            // Movement & Screen Wrap
            if (IsKeyDown(KEY_LEFT))  player.x -= 5;
            if (IsKeyDown(KEY_RIGHT)) player.x += 5;
            if (player.x < -15) player.x = 400;
            if (player.x > 400) player.x = -15;

            // Physics & Gravity
            velY += 0.3f;
            player.y += velY;

            // Platform Collision
            if (velY > 0) {
                for (int i = 0; i < PLATFORM_COUNT; i++) {
                    Rectangle rect = { platforms[i].pos.x, platforms[i].pos.y, 80, 15 };
                    if (CheckCollisionCircleRec(player, 15, rect) && player.y < platforms[i].pos.y + 10) {
                        velY = -10.0f;
                    }
                }
            }

            // Camera Scroll & Recycling
            if (player.y < 300) {
                float diff = 300 - player.y;
                player.y = 300;
                score += (int)diff;

                for (int i = 0; i < PLATFORM_COUNT; i++) {
                    platforms[i].pos.y += diff;
                    if (platforms[i].pos.y > 600) {
                        platforms[i].pos.y -= 600;
                        platforms[i].pos.x = rand() % 320;
                    }
                }
            }

            if (player.y > 600) gameOver = true;
        } else if (IsKeyPressed(KEY_ENTER)) {
            RESET_GAME();
        }

        // Render Routine
        BeginDrawing();
            ClearBackground(RAYWHITE);
            if (!gameOver) {
                for (int i = 0; i < PLATFORM_COUNT; i++)
                    DrawRectangleV(platforms[i].pos, (Vector2){ 80, 15 }, GREEN);
                
                DrawCircleV(player, 15, RED);
                DrawText(TextFormat("Score: %i", score), 10, 10, 20, DARKGRAY);
            } else {
                DrawText("GAME OVER", 110, 250, 30, RED);
                DrawText("Press ENTER to Restart", 90, 300, 18, GRAY);
            }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
