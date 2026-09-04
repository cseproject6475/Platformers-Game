#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PLATFORMS 12
#define SCREEN_W 800
#define SCREEN_H 600
#define MAX_NAME_LEN 12
#define SCORE_FILE "highscore.dat"

typedef enum { STATE_MENU, STATE_PLAYING, STATE_GAMEOVER } GameState;
typedef enum { PLAT_NORMAL, PLAT_MOVING, PLAT_BROKEN } PlatType;
typedef enum { ITEM_NONE, ITEM_SPRING, ITEM_ROCKET } ItemType;

typedef struct {
    Rectangle rect;
    PlatType type;
    ItemType item;
    float dir;       
    float squish;    
    float springAnim; 
} Platform;

// Color Palette
const Color COLOR_BG         = (Color){ 245, 243, 238, 255 }; 
const Color COLOR_ACCENT_BG  = (Color){ 232, 228, 218, 255 }; 
const Color COLOR_PRIMARY    = (Color){  44,  62,  80, 255 }; 
const Color COLOR_PLAT_NORM  = (Color){  72, 187, 120, 255 }; 
const Color COLOR_PLAT_MOVE  = (Color){  66, 153, 225, 255 }; 
const Color COLOR_PLAT_BRK   = (Color){ 245, 101, 101, 255 }; 
const Color COLOR_PLAYER     = (Color){ 237, 137,  54, 255 }; 
const Color COLOR_UI_CARD    = (Color){ 255, 255, 255, 230 }; 

Font customFont;

void DrawCustomText(const char *text, int posX, int posY, int fontSize, Color color) {
    if (customFont.texture.id > 0) {
        Vector2 pos = { (float)posX, (float)posY };
        DrawTextEx(customFont, text, pos, (float)fontSize, 1.0f, color);
    } else {
        DrawText(text, posX, posY, fontSize, color);
    }
}

int MeasureCustomText(const char *text, int fontSize) {
    if (customFont.texture.id > 0) {
        Vector2 size = MeasureTextEx(customFont, text, (float)fontSize, 1.0f);
        return (int)size.x;
    }
    return MeasureText(text, fontSize);
}

// Key-out solid white background for Logo / Watermark
Texture2D LoadAndProcessLogo(const char *fileName) {
    const char *candidates[] = {
        fileName, "images.png", "background.png", "logo.png"
    };
    const char *targetFile = NULL;

    for (int i = 0; i < 4; i++) {
        if (FileExists(candidates[i])) {
            targetFile = candidates[i];
            break;
        }
    }

    if (!targetFile) return (Texture2D){ 0 };

    Image img = LoadImage(targetFile);
    if (img.data == NULL) return (Texture2D){ 0 };

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *pixels = (Color *)img.data;
    int pixelCount = img.width * img.height;

    for (int i = 0; i < pixelCount; i++) {
        if (pixels[i].r > 210 && pixels[i].g > 210 && pixels[i].b > 210) {
            pixels[i].a = 0;
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

// Flexible loader for Player character picture
Texture2D LoadAndProcessPlayer(const char *fileName) {
    const char *candidates[] = {
        fileName,
        "player.png", "player.jpg", "player.jpeg", "player.bmp",
        "character.png", "character.jpg", "character.jpeg", "character.bmp",
        "assets/player.png", "assets/character.png"
    };
    const char *targetFile = NULL;
    int numCandidates = sizeof(candidates) / sizeof(candidates[0]);

    for (int i = 0; i < numCandidates; i++) {
        if (candidates[i] && FileExists(candidates[i])) {
            targetFile = candidates[i];
            break;
        }
    }

    if (!targetFile) return (Texture2D){ 0 };

    Image img = LoadImage(targetFile);
    if (img.data == NULL) return (Texture2D){ 0 };

    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    Color *pixels = (Color *)img.data;
    int pixelCount = img.width * img.height;

    // Optional background keying: Make white background pixels transparent
    for (int i = 0; i < pixelCount; i++) {
        if (pixels[i].r > 230 && pixels[i].g > 230 && pixels[i].b > 230) {
            pixels[i].a = 0;
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

void DrawBackgroundWatermark(Texture2D logo) {
    if (logo.id == 0) return;

    float targetHeight = 440.0f;
    float aspect = (float)logo.width / (float)logo.height;
    float targetWidth = targetHeight * aspect;

    float x = (SCREEN_W - targetWidth) / 2.0f;
    float y = (SCREEN_H - targetHeight) / 2.0f;

    Rectangle src = { 0, 0, (float)logo.width, (float)logo.height };
    Rectangle dest = { x, y, targetWidth, targetHeight };

    DrawTexturePro(logo, src, dest, (Vector2){ 0, 0 }, 0.0f, Fade(WHITE, 0.12f));
}

void DrawSpring(float x, float y, float animState) {
    float restHeight = 18.0f;
    float currentHeight = Clamp(restHeight + animState, 6.0f, 32.0f);
    int coils = 4;
    float step = currentHeight / coils;

    DrawRectangle(x - 10, y - 2, 20, 3, DARKGRAY);

    Vector2 p1 = { x - 7, y - 2 };
    for (int i = 0; i < coils; i++) {
        float nextY = y - 2 - ((i + 1) * step);
        float nextX = (i % 2 == 0) ? (x + 7) : (x - 7);
        Vector2 p2 = { nextX, nextY };
        DrawLineEx(p1, p2, 3.5f, GRAY);
        DrawLineEx(p1, p2, 1.5f, LIGHTGRAY);
        p1 = p2;
    }

    float topY = y - 2 - currentHeight;
    DrawRectangle(x - 12, topY - 3, 24, 4, GRAY);
    DrawRectangle(x - 10, topY - 2, 20, 2, LIGHTGRAY);
}

void DrawRocket(float x, float y, bool active) {
    DrawTriangle((Vector2){ x - 12, y + 10 }, (Vector2){ x - 5, y - 5 }, (Vector2){ x - 5, y + 10 }, COLOR_PLAT_BRK);
    DrawTriangle((Vector2){ x + 12, y + 10 }, (Vector2){ x + 5, y + 10 }, (Vector2){ x + 5, y - 5 }, COLOR_PLAT_BRK);

    DrawRectangleRounded((Rectangle){ x - 6, y - 12, 12, 22 }, 0.3f, 4, LIGHTGRAY);
    DrawRectangle(x - 2, y - 10, 4, 18, WHITE);

    DrawTriangle((Vector2){ x, y - 22 }, (Vector2){ x - 6, y - 12 }, (Vector2){ x + 6, y - 12 }, GOLD);
    DrawRectangle(x - 4, y + 10, 8, 4, DARKGRAY);

    if (active) {
        float flameLength = 18.0f + (rand() % 12);
        DrawTriangle((Vector2){ x, y + 14 + flameLength }, (Vector2){ x - 6, y + 14 }, (Vector2){ x + 6, y + 14 }, ORANGE);
        DrawTriangle((Vector2){ x, y + 14 + (flameLength * 0.6f) }, (Vector2){ x - 3, y + 14 }, (Vector2){ x + 3, y + 14 }, YELLOW);
    }
}

int LoadHighScore(char *outName) {
    FILE *f = fopen(SCORE_FILE, "rb");
    int score = 0;
    if (f) {
        fread(&score, sizeof(int), 1, f);
        fread(outName, sizeof(char), MAX_NAME_LEN, f);
        fclose(f);
    } else {
        strcpy(outName, "GUEST");
    }
    return score;
}

void SaveHighScore(int score, const char *name) {
    FILE *f = fopen(SCORE_FILE, "wb");
    if (f) {
        fwrite(&score, sizeof(int), 1, f);
        fwrite(name, sizeof(char), MAX_NAME_LEN, f);
        fclose(f);
    }
}

void ResetGameWorld(Vector2 *playerPos, Vector2 *playerVel, Platform platforms[], int *score, float *rocketTimer) {
    playerPos->x = SCREEN_W / 2.0f;
    playerPos->y = SCREEN_H - 120.0f;
    playerVel->x = 0;
    playerVel->y = -500.0f;
    *score = 0;
    *rocketTimer = 0.0f;

    platforms[0].rect = (Rectangle){ SCREEN_W / 2.0f - 60.0f, SCREEN_H - 80.0f, 120.0f, 16.0f };
    platforms[0].type = PLAT_NORMAL;
    platforms[0].item = ITEM_NONE;
    platforms[0].squish = 0.0f;
    platforms[0].springAnim = 0.0f;
    platforms[0].dir = 1.0f;

    float spacing = (SCREEN_H - 80.0f) / (MAX_PLATFORMS - 1);
    for (int i = 1; i < MAX_PLATFORMS; i++) {
        float spawnY = (SCREEN_H - 80.0f) - (i * spacing);
        platforms[i].rect = (Rectangle){ rand() % (SCREEN_W - 100), spawnY, 100, 16 };
        platforms[i].squish = 0.0f;
        platforms[i].springAnim = 0.0f;
        
        int roll = rand() % 100;
        platforms[i].type = (roll > 80) ? PLAT_BROKEN : ((roll > 55) ? PLAT_MOVING : PLAT_NORMAL);
        platforms[i].dir = (rand() % 2 == 0) ? 1.0f : -1.0f;
        
        platforms[i].item = ITEM_NONE;
        if (platforms[i].type != PLAT_BROKEN) {
            int itemRoll = rand() % 100;
            if (itemRoll > 85) platforms[i].item = ITEM_ROCKET;
            else if (itemRoll > 60) platforms[i].item = ITEM_SPRING;
        }
    }
}

int main(void) {
    InitWindow(SCREEN_W, SCREEN_H, "Doodle Jump");
    SetTargetFPS(60);

    const char *fontPaths[] = {
        "Hiragino Mincho ProN.ttc",
        "/System/Library/Fonts/Hiragino Mincho ProN.ttc",
        "/System/Library/Fonts/Supplemental/Hiragino Mincho ProN.ttc",
        "HiraMinProN-W3.otf"
    };

    for (int i = 0; i < 4; i++) {
        if (FileExists(fontPaths[i])) {
            customFont = LoadFontEx(fontPaths[i], 64, 0, 0);
            if (customFont.texture.id > 0) break;
        }
    }

    if (customFont.texture.id == 0) {
        customFont = LoadFontEx("/System/Library/Fonts/Hiragino Sans GB.ttc", 64, 0, 0);
    }
    SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);

    // Texture Loading & Debug Console Output
    printf("\n=== GAME STARTUP DEBUG ===\n");
    printf("[DEBUG] Current Working Dir: %s\n", GetWorkingDirectory());

    Texture2D logoTexture = LoadAndProcessLogo("images.png");
    Texture2D playerTexture = LoadAndProcessPlayer("player.png");

    if (playerTexture.id > 0) {
        printf("[DEBUG] SUCCESS: Player image loaded! (ID: %u | %dx%d px)\n", 
               playerTexture.id, playerTexture.width, playerTexture.height);
    } else {
        printf("[DEBUG] WARNING: 'player.png' or 'character.png' NOT found!\n");
        printf("[DEBUG] Place image in folder: %s\n", GetWorkingDirectory());
    }
    printf("===========================\n\n");

    GameState state = STATE_MENU;

    char playerName[MAX_NAME_LEN + 1] = "";
    int letterCount = 0;
    char highScoreName[MAX_NAME_LEN + 1];
    int highScore = LoadHighScore(highScoreName);

    Vector2 playerPos;
    Vector2 playerVel;
    int score = 0;
    float rocketTimer = 0.0f;
    Platform platforms[MAX_PLATFORMS];

    float transitionAlpha = 1.0f;
    bool isTransitioning = false;
    GameState nextState = STATE_MENU;

    ResetGameWorld(&playerPos, &playerVel, platforms, &score, &rocketTimer);

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (isTransitioning) {
            transitionAlpha += 3.0f * dt;
            if (transitionAlpha >= 1.0f) {
                transitionAlpha = 1.0f;
                state = nextState;
                isTransitioning = false;
            }
        } else if (transitionAlpha > 0.0f) {
            transitionAlpha -= 3.0f * dt;
            if (transitionAlpha < 0.0f) transitionAlpha = 0.0f;
        }

        switch (state) {
            case STATE_MENU: {
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= 32) && (key <= 125) && (letterCount < MAX_NAME_LEN)) {
                        playerName[letterCount] = (char)key;
                        playerName[letterCount + 1] = '\0';
                        letterCount++;
                    }
                    key = GetCharPressed();
                }

                if (IsKeyPressed(KEY_BACKSPACE)) {
                    letterCount--;
                    if (letterCount < 0) letterCount = 0;
                    playerName[letterCount] = '\0';
                }

                if (IsKeyPressed(KEY_ENTER) && !isTransitioning) {
                    if (letterCount == 0) strcpy(playerName, "PLAYER");
                    ResetGameWorld(&playerPos, &playerVel, platforms, &score, &rocketTimer);
                    nextState = STATE_PLAYING;
                    isTransitioning = true;
                }
            } break;

            case STATE_PLAYING: {
                bool isRocketActive = (rocketTimer > 0.0f);
                if (isRocketActive) {
                    rocketTimer -= dt;
                    playerVel.y = -1100.0f;
                } else {
                    playerVel.y += 980.0f * dt;
                }

                float moveDir = (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) - (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A));
                playerVel.x = Lerp(playerVel.x, moveDir * 450.0f, 12.0f * dt);
                playerPos.x += playerVel.x * dt;

                if (playerPos.x < -18) playerPos.x = SCREEN_W;
                if (playerPos.x > SCREEN_W) playerPos.x = -18;

                playerPos.y += playerVel.y * dt;

                for (int i = 0; i < MAX_PLATFORMS; i++) {
                    if (platforms[i].type == PLAT_MOVING) {
                        platforms[i].rect.x += platforms[i].dir * 140.0f * dt;
                        if (platforms[i].rect.x <= 0 || platforms[i].rect.x >= SCREEN_W - platforms[i].rect.width) {
                            platforms[i].dir *= -1.0f;
                        }
                    }

                    platforms[i].squish = Lerp(platforms[i].squish, 0.0f, 10.0f * dt);
                    platforms[i].springAnim = Lerp(platforms[i].springAnim, 0.0f, 8.0f * dt);

                    if (!isRocketActive && playerVel.y > 0 && 
                        (playerPos.y + 12.0f) < (platforms[i].rect.y + platforms[i].rect.height)) {
                        
                        if (platforms[i].item == ITEM_SPRING) {
                            Rectangle springRec = { platforms[i].rect.x + 38, platforms[i].rect.y - 20, 24, 20 };
                            if (CheckCollisionCircleRec(playerPos, 15.0f, springRec)) {
                                playerVel.y = -1000.0f;
                                platforms[i].squish = 8.0f;
                                platforms[i].springAnim = 14.0f;
                                continue;
                            }
                        }

                        if (platforms[i].item == ITEM_ROCKET) {
                            Rectangle rocketRec = { platforms[i].rect.x + 38, platforms[i].rect.y - 30, 24, 30 };
                            if (CheckCollisionCircleRec(playerPos, 15.0f, rocketRec)) {
                                rocketTimer = 2.5f;
                                platforms[i].item = ITEM_NONE;
                                continue;
                            }
                        }

                        if (CheckCollisionCircleRec(playerPos, 15.0f, platforms[i].rect)) {
                            if (platforms[i].type == PLAT_BROKEN) {
                                platforms[i].rect.y = 9999;
                            } else {
                                playerVel.y = -620.0f;
                                platforms[i].squish = 6.0f;
                                if (platforms[i].item == ITEM_SPRING) platforms[i].springAnim = -8.0f;
                            }
                        }
                    }
                }

                if (playerPos.y < 300.0f) {
                    float deltaY = 300.0f - playerPos.y;
                    playerPos.y = 300.0f;
                    score += (int)deltaY;

                    for (int i = 0; i < MAX_PLATFORMS; i++) {
                        platforms[i].rect.y += deltaY;

                        if (platforms[i].rect.y > SCREEN_H) {
                            platforms[i].rect = (Rectangle){ rand() % (SCREEN_W - 100), -16, 100, 16 };
                            platforms[i].squish = 0.0f;
                            platforms[i].springAnim = 0.0f;

                            int roll = rand() % 100;
                            platforms[i].type = (roll > 80) ? PLAT_BROKEN : ((roll > 55) ? PLAT_MOVING : PLAT_NORMAL);
                            platforms[i].dir = (rand() % 2 == 0) ? 1.0f : -1.0f;

                            platforms[i].item = ITEM_NONE;
                            if (platforms[i].type != PLAT_BROKEN) {
                                int itemRoll = rand() % 100;
                                if (itemRoll > 85) platforms[i].item = ITEM_ROCKET;
                                else if (itemRoll > 60) platforms[i].item = ITEM_SPRING;
                            }
                        }
                    }
                }

                if (playerPos.y > SCREEN_H + 40 && !isTransitioning) {
                    if (score > highScore) {
                        highScore = score;
                        strcpy(highScoreName, playerName);
                        SaveHighScore(highScore, highScoreName);
                    }
                    nextState = STATE_GAMEOVER;
                    isTransitioning = true;
                }
            } break;

            case STATE_GAMEOVER: {
                if ((IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) && !isTransitioning) {
                    ResetGameWorld(&playerPos, &playerVel, platforms, &score, &rocketTimer);
                    nextState = STATE_PLAYING;
                    isTransitioning = true;
                }
                if (IsKeyPressed(KEY_M) && !isTransitioning) {
                    nextState = STATE_MENU;
                    isTransitioning = true;
                }
            } break;
        }

        BeginDrawing();
            ClearBackground(COLOR_BG);

            // 1. Grid Lines
            for (int x = 0; x < SCREEN_W; x += 40) DrawLine(x, 0, x, SCREEN_H, COLOR_ACCENT_BG);
            for (int y = 0; y < SCREEN_H; y += 40) DrawLine(0, y, SCREEN_W, y, COLOR_ACCENT_BG);

            // 2. Faint Background Watermark
            DrawBackgroundWatermark(logoTexture);

            if (state == STATE_MENU) {
                DrawCustomText("DOODLE JUMP", SCREEN_W / 2 - MeasureCustomText("DOODLE JUMP", 52) / 2, 100, 52, COLOR_PRIMARY);

                DrawRectangleRounded((Rectangle){ SCREEN_W / 2 - 180, 185, 360, 70 }, 0.2f, 4, COLOR_UI_CARD);
                DrawCustomText("ALL-TIME HIGH SCORE", SCREEN_W / 2 - MeasureCustomText("ALL-TIME HIGH SCORE", 14) / 2, 197, 14, GRAY);
                DrawCustomText(TextFormat("%s - %i PTS", highScoreName, highScore), SCREEN_W / 2 - MeasureCustomText(TextFormat("%s - %i PTS", highScoreName, highScore), 22) / 2, 219, 22, COLOR_PRIMARY);

                DrawCustomText("ENTER YOUR NAME:", SCREEN_W / 2 - MeasureCustomText("ENTER YOUR NAME:", 16) / 2, 290, 16, COLOR_PRIMARY);
                DrawRectangleRounded((Rectangle){ SCREEN_W / 2 - 160, 320, 320, 55 }, 0.25f, 4, WHITE);
                DrawRectangleRoundedLines((Rectangle){ SCREEN_W / 2 - 160, 320, 320, 55 }, 0.25f, 4, 2.0f, COLOR_PRIMARY);
                
                char displayInput[MAX_NAME_LEN + 2];
                strcpy(displayInput, playerName);
                if (((int)(GetTime() * 2) % 2) == 0) strcat(displayInput, "_");
                
                DrawCustomText(displayInput, SCREEN_W / 2 - MeasureCustomText(displayInput, 24) / 2, 335, 24, COLOR_PLAYER);
                DrawCustomText("Press [ENTER] to Play", SCREEN_W / 2 - MeasureCustomText("Press [ENTER] to Play", 18) / 2, 415, 18, COLOR_PRIMARY);
            } 
            else if (state == STATE_PLAYING || state == STATE_GAMEOVER) {
                // Render Platforms
                for (int i = 0; i < MAX_PLATFORMS; i++) {
                    Color platColor = (platforms[i].type == PLAT_NORMAL) ? COLOR_PLAT_NORM :
                                      (platforms[i].type == PLAT_MOVING) ? COLOR_PLAT_MOVE : COLOR_PLAT_BRK;
                    
                    Rectangle drawRect = { 
                        platforms[i].rect.x, 
                        platforms[i].rect.y + platforms[i].squish, 
                        platforms[i].rect.width, 
                        platforms[i].rect.height - platforms[i].squish 
                    };
                    DrawRectangleRounded(drawRect, 0.4f, 4, platColor);

                    if (platforms[i].item == ITEM_SPRING) {
                        DrawSpring(platforms[i].rect.x + 50, platforms[i].rect.y, platforms[i].springAnim);
                    } else if (platforms[i].item == ITEM_ROCKET) {
                        DrawRocket(platforms[i].rect.x + 50, platforms[i].rect.y - 12, false);
                    }
                }

                // Render Player Character Image
                if (playerTexture.id > 0) {
                    float charSize = 48.0f; // Dimension of character picture
                    Rectangle src = { 0, 0, (float)playerTexture.width, (float)playerTexture.height };
                    
                    // Flip image horizontally when moving left
                    if (playerVel.x < -10.0f) src.width = -(float)playerTexture.width;

                    Rectangle dest = { playerPos.x - charSize / 2.0f, playerPos.y - charSize / 2.0f, charSize, charSize };
                    DrawTexturePro(playerTexture, src, dest, (Vector2){ 0, 0 }, 0.0f, WHITE);
                } else {
                    // Fallback Circle if image fails to load
                    DrawCircleV(playerPos, 15.0f, COLOR_PLAYER);
                    DrawCircleV(Vector2Add(playerPos, (Vector2){ playerVel.x * 0.02f, 0 }), 5.0f, WHITE);
                }

                if (rocketTimer > 0.0f) {
                    DrawRocket(playerPos.x, playerPos.y, true);
                }

                // HUD / Score Card
                DrawRectangleRounded((Rectangle){ 20, 15, 260, 50 }, 0.3f, 4, COLOR_UI_CARD);
                DrawCustomText(TextFormat("PLAYER: %s", playerName), 32, 21, 14, COLOR_PRIMARY);
                DrawCustomText(TextFormat("SCORE: %i", score), 32, 39, 16, COLOR_PLAYER);

                if (state == STATE_GAMEOVER) {
                    DrawRectangle(0, 0, SCREEN_W, SCREEN_H, (Color){ 44, 62, 80, 140 });
                    DrawRectangleRounded((Rectangle){ SCREEN_W / 2 - 200, 140, 400, 320 }, 0.15f, 4, COLOR_UI_CARD);
                    DrawCustomText("GAME OVER", SCREEN_W / 2 - MeasureCustomText("GAME OVER", 36) / 2, 170, 36, COLOR_PLAT_BRK);
                    DrawCustomText(TextFormat("Player: %s", playerName), SCREEN_W / 2 - MeasureCustomText(TextFormat("Player: %s", playerName), 20) / 2, 225, 20, COLOR_PRIMARY);
                    DrawCustomText(TextFormat("Your Score: %i", score), SCREEN_W / 2 - MeasureCustomText(TextFormat("Your Score: %i", score), 22) / 2, 255, 22, COLOR_PLAYER);
                    DrawCustomText("Press [SPACE] or [ENTER] to Replay", SCREEN_W / 2 - MeasureCustomText("Press [SPACE] or [ENTER] to Replay", 16) / 2, 350, 16, COLOR_PRIMARY);
                }
            }

            if (transitionAlpha > 0.0f) {
                DrawRectangle(0, 0, SCREEN_W, SCREEN_H, Fade(COLOR_PRIMARY, transitionAlpha));
            }

        EndDrawing();
    }

    if (logoTexture.id > 0) UnloadTexture(logoTexture);
    if (playerTexture.id > 0) UnloadTexture(playerTexture);
    if (customFont.texture.id > 0) UnloadFont(customFont);
    CloseWindow();
    return 0;
}
