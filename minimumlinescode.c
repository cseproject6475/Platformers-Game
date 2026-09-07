#include "raylib.h"
#include <stdlib.h>

#define PLATFORMS_COUNT 10

typedef struct { Vector2 pos; } Platform;

int main(void) {
    InitWindow(400, 600, "Raylib Doodle Jump");
    SetTargetFPS(60);

    Vector2 player = { 200, 400 };
    float velocityY = 0;
    int score = 0;
    bool gameOver = false;

    Platform platforms[PLATFORMS_COUNT];
    for (int i = 0; i < PLATFORMS_COUNT; i++) {
        platforms[i].pos = (Vector2){ rand() % 320, i * 60 };
    }

    while (!WindowShouldClose()) {
        if (!gameOver) {
            // 1. Input & Horizontal Movement
            if (IsKeyDown(KEY_LEFT)) player.x -= 5;
            if (IsKeyDown(KEY_RIGHT)) player.x += 5;
            if (player.x < -20) player.x = 400; // Screen wrap
            if (player.x > 400) player.x = -20;

            // 2. Physics & Gravity
            velocityY += 0.3f;
            player.y += velocityY;

            // 3. Platform Collision Check (Only when falling)
            if (velocityY > 0) {
                for (int i = 0; i < PLATFORMS_COUNT; i++) {
                    Rectangle pRec = { platforms[i].pos.x, platforms[i].pos.y, 80, 15 };
                    if (CheckCollisionCircleRec(player, 15, pRec) && player.y < platforms[i].pos.y) {
                        velocityY = -10.0f; // Bounce
                    }
                }
            }

            // 4. Camera Scroll & World Movement
            if (player.y < 300) {
                float diff = 300 - player.y;
                player.y = 300;
                score += (int)diff;

                for (int i = 0; i < PLATFORMS_COUNT; i++) {
                    platforms[i].pos.y += diff;
                    // Recycle platform to top when off-screen bottom
                    if (platforms[i].pos.y > 600) {
                        platforms[i].pos.y = 0;
                        platforms[i].pos.x = rand() % 320;
                    }
                }
            }

            // Game Over
            if (player.y > 600) gameOver = true;
        } else if (IsKeyPressed(KEY_ENTER)) {
            // Reset Game
            player = (Vector2){ 200, 400 };
            velocityY = 0;
            score = 0;
            gameOver = false;
        }

        // 5. Drawing
        BeginDrawing();
            ClearBackground(RAYWHITE);
            if (!gameOver) {
                // Draw Platforms
                for (int i = 0; i < PLATFORMS_COUNT; i++) {
                    DrawRectangleV(platforms[i].pos, (Vector2){ 80, 15 }, GREEN);
                }
                // Draw Player
                DrawCircleV(player, 15, RED);
                // Draw Score
                DrawText(TextFormat("Score: %i", score), 10, 10, 20, DARKGRAY);
            } else {
                DrawText("GAME OVER", 120, 250, 30, RED);
                DrawText("Press ENTER to restart", 100, 300, 18, GRAY);
            }
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
