#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

#define WIDTH 1600
#define HEIGHT 800
#define NORMALBALLONSNUM 4
#define SPAWNPOINTS 5
#define MAXBALLOONS 14
#define MAXPOPUPS 6
#define MAXARROWPOPUPS 6

typedef struct
{
    Vector2 position;
    Vector2 velocity;
    float radius;
    bool active;
} Arrow;

typedef struct
{
    Vector2 position;
    float radius;
    float speed;
    bool active;
    bool gold;
    bool danger;
    bool mustpop;
    int index;
} Balloon;

typedef struct
{
    Vector2 position;
    int value;
    float visibletime;
    bool isactive;
} ScorePopUp;

typedef struct
{
    Vector2 position;
    int value;
    float visibletime;
    bool isactive;
} ArrowPopUp;

// Scorepopup function for calling it in case of normal, mustpop, danger loons
void Popup(ScorePopUp scorepopup[], Vector2 pos, int score)
{
    for (int i = 0; i < MAXPOPUPS; i++)
    {
        if (scorepopup[i].isactive == false)
        {
            scorepopup[i].isactive = true;
            scorepopup[i].position = pos;
            scorepopup[i].value = score;
            scorepopup[i].visibletime = 1.0f;
            break;
        }
    }
}

// Arrowpopup function for calling it when a golden balloon is popped or mustpop balloon escapes
void ArrowPopup(ArrowPopUp arrowpopup[], Vector2 pos, int arrows)
{
    for (int i = 0; i < MAXARROWPOPUPS; i++)
    {
        if (arrowpopup[i].isactive == false)
        {
            arrowpopup[i].isactive = true;
            arrowpopup[i].position = pos;
            arrowpopup[i].value = arrows;
            arrowpopup[i].visibletime = 1.0f;
            break;
        }
    }
}

int main(void)
{
    InitWindow(WIDTH, HEIGHT, "HIT 'EM ALL");
    InitAudioDevice();
    SetTargetFPS(60);

    // loading all audio, textures, fonts
    Music bgmusic = LoadMusicStream("assets/audio/Game Window.mp3");
    Sound shootsound = LoadSound("assets/audio/Gun Shooting.ogg");
    Sound popsound = LoadSound("assets/audio/Balloon Pop.mp3");
    Sound gameoversound = LoadSound("assets/audio/Game Over.mp3");

    PlayMusicStream(bgmusic);

    Texture2D background = LoadTexture("assets/sprites/Untitled-1.png");
    Texture2D gameovertexture = LoadTexture("assets/sprites/gameover.png");
    Texture2D bowimage = LoadTexture("assets/sprites/bow.png");
    Texture2D arrowimage = LoadTexture("assets/sprites/arrow.png");
    Texture2D arrowballoon = LoadTexture("assets/sprites/arrowballoon.png");
    Texture2D dangerballoon = LoadTexture("assets/sprites/dangerballoon.png");
    Texture2D mustpopballoon = LoadTexture("assets/sprites/mustpopballoon.png");

    Texture2D normalballoons[NORMALBALLONSNUM];
    for (int i = 0; i < NORMALBALLONSNUM; i++)
    {
        normalballoons[i] = LoadTexture(TextFormat("assets/sprites/normalballoon%d.png", i + 1));
    }

    // Load Boy Animation Textures (boy01.png to boy10.png)
    Texture2D boytextures[10];
    for (int i = 0; i < 10; i++)
    {
        boytextures[i] = LoadTexture(TextFormat("assets/sprites/boy%02d-removebg-preview.png", i + 1));
    }

    Font customfont = LoadFont("assets/fonts/Carnival Font.ttf");

    // init spawnpoints, arrow, balloons
    Vector2 spawnpoints[SPAWNPOINTS];
    for (int i = 0; i < SPAWNPOINTS; i++)
    {
        spawnpoints[i] = (Vector2){1000.0f + i * 130.0f, HEIGHT + 50.0f};
    }

    Arrow arrow = {0};

    Balloon balloons[MAXBALLOONS] = {0};
    for (int i = 0; i < MAXBALLOONS; i++)
    {
        balloons[i].active = false;
    }

    // init game variables
    int score = 0;
    int highestscore = 0;
    bool gameover = false;
    int arrowsleft = 10;
    float gravity = 1000.0f;
    float currenttimer = 0.0f;
    float spawninterval = 2.0f;
    float pulldistance = 0.0f;
    float launchspeed = 0.0f;

    // Boy animation variables
    int boycurrentframe = 0;
    float boyanimtimer = 0.0f;
    float boyframeduration = 1.0f / 5.0f; // 5 FPS animation

    // Standard Balloon Size
    float balloonDrawSize = 140.0f;
    float balloonradius = balloonDrawSize * 0.4f;

    // Independent size settings for Danger Balloon
    float dangerDrawWidth = 300.0f;
    float dangerDrawHeight = 164.0f;
    float dangerRadius = dangerDrawWidth * 0.25f; // Radius derived from width

    // Independent size settings for Must Pop Balloon (defined exactly like Danger Balloon)
    float mustPopDrawWidth = 180.0f;
    float mustPopDrawHeight = 180.0f;
    float mustPopRadius = mustPopDrawWidth * 0.4f;

    // POPUP SCORE & ARROW array
    ScorePopUp scorepopup[MAXPOPUPS] = {0};
    ArrowPopUp arrowpopup[MAXARROWPOPUPS] = {0};

    // reading highest score from file
    FILE *highestscorefile = fopen("highestscore.txt", "r");
    if (highestscorefile != NULL)
    {
        fscanf(highestscorefile, "%d", &highestscore);
        fclose(highestscorefile);
    }

    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        UpdateMusicStream(bgmusic);

        // Update boy animation frame
        boyanimtimer += dt;
        if (boyanimtimer >= boyframeduration)
        {
            boyanimtimer = 0.0f;
            boycurrentframe = (boycurrentframe + 1) % 10;
        }

        // bow and arrow settings
        Vector2 mouseposition = GetMousePosition();
        Vector2 arrowpivot = {280.0f, 590.0f};
        float aimangle = 0.0f;
        Vector2 aimdirection = {1.0f, 0.0f};
        float maxpulldistance = 120.0f;
        float minarrowspeed = 500.0f;
        float maxarrowspeed = 2500.0f;
        float pullspeed = 100.0f;

        // aiming (clamped to +/-45 degrees), pulling back and shooting arrow
        if (arrowsleft > 0 && gameover == false)
        {
            float mousepointerangle = atan2f(mouseposition.y - arrowpivot.y, mouseposition.x - arrowpivot.x);
            if (mousepointerangle > PI / 4.0)
            {
                aimangle = PI / 4.0;
            }
            else if (mousepointerangle < -PI / 4.0)
            {
                aimangle = -PI / 4.0;
            }
            else
            {
                aimangle = mousepointerangle;
            }
            aimdirection = (Vector2){cosf(aimangle), sinf(aimangle)};

            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && arrow.active == false)
            {
                pulldistance += pullspeed * dt;
                if (pulldistance > maxpulldistance)
                {
                    pulldistance = maxpulldistance;
                }
            }
            if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && arrow.active == false)
            {
                float pullratio = pulldistance / maxpulldistance;
                float arrowspeed = minarrowspeed + pullratio * (maxarrowspeed - minarrowspeed);

                arrow.position = arrowpivot;
                arrow.velocity = Vector2Scale(aimdirection, arrowspeed);
                arrow.radius = 17.0f;
                arrow.active = true;
                arrowsleft--;
                PlaySound(shootsound);
                launchspeed = arrowspeed;
                pulldistance = 0.0f;
            }
        }

        // projectile formula
        if (arrow.active == true)
        {
            arrow.velocity.y += dt * gravity;
            arrow.position = Vector2Add(arrow.position, Vector2Scale(arrow.velocity, dt));
            if (arrow.position.x > WIDTH + 50 || arrow.position.y > HEIGHT + 50)
            {
                arrow.active = false;
            }
        }

        // spawning balloons
        if (gameover == false)
        {
            currenttimer += dt;
            if (currenttimer >= spawninterval)
            {
                currenttimer = 0.0f;
                for (int i = 0; i < MAXBALLOONS; i++)
                {
                    if (balloons[i].active == false)
                    {
                        balloons[i].position = spawnpoints[GetRandomValue(0, 4)];
                        balloons[i].speed = 150.0f;
                        balloons[i].active = true;

                        int dangerroll = (score >= 200) ? 40 : 0;
                        int mustpoproll = (score >= 100) ? 15 : 0;

                        int roll = GetRandomValue(1, 100);

                        // 1. Danger Balloon Check
                        if (roll <= dangerroll)
                        {
                            balloons[i].danger = true;
                            balloons[i].mustpop = false;
                            balloons[i].gold = false;
                            balloons[i].radius = dangerRadius;
                        }
                        // 2. Must Pop Balloon Check
                        else if (roll <= dangerroll + mustpoproll) // 25% chance (41 to 65)
                        {
                            balloons[i].danger = false;
                            balloons[i].mustpop = true;
                            balloons[i].gold = false;
                            balloons[i].radius = mustPopRadius;
                        }
                        // 3. Normal / Gold Balloon Check
                        else
                        {
                            balloons[i].danger = false;
                            balloons[i].mustpop = false;
                            balloons[i].gold = (GetRandomValue(1, 10) <= 2);
                            balloons[i].radius = balloonradius;
                        }

                        balloons[i].index = GetRandomValue(0, 3);
                        break;
                    }
                }
            }
        }

        // scorepopups update
        for (int i = 0; i < MAXPOPUPS; i++)
        {
            if (scorepopup[i].isactive)
            {
                scorepopup[i].visibletime -= dt;
                scorepopup[i].position.y -= 20.0f * dt;
                if (scorepopup[i].visibletime <= 0)
                {
                    scorepopup[i].isactive = false;
                }
            }
        }

        // arrowpopups update
        for (int i = 0; i < MAXARROWPOPUPS; i++)
        {
            if (arrowpopup[i].isactive)
            {
                arrowpopup[i].visibletime -= dt;
                arrowpopup[i].position.y -= 20.0f * dt;
                if (arrowpopup[i].visibletime <= 0)
                {
                    arrowpopup[i].isactive = false;
                }
            }
        }

        // balloon movement & collision physics
        for (int i = 0; i < MAXBALLOONS; i++)
        {
            if (balloons[i].active == false)
                continue;

            balloons[i].position.y -= dt * balloons[i].speed;

            // Screen boundary check: balloon escaped past top
            if (balloons[i].position.y < -100.0f)
            {
                balloons[i].active = false;

                // Penalty: decrease ammo by 2 if a mustpop balloon escapes
                if (balloons[i].mustpop == true)
                {
                    arrowsleft -= 2;
                    if (arrowsleft < 0)
                    {
                        arrowsleft = 0;
                    }

                    // Trigger red penalty popup near top of screen
                    ArrowPopup(arrowpopup, (Vector2){balloons[i].position.x, 40.0f}, -2);
                }
            }

            if (arrow.active == true && CheckCollisionCircles(arrow.position, arrow.radius, balloons[i].position, balloons[i].radius) == true)
            {
                balloons[i].active = false;

                if (balloons[i].danger == true)
                {
                    gameover = true;
                    StopMusicStream(bgmusic);
                    PlaySound(gameoversound);
                    if (score > highestscore)
                    {
                        highestscore = score;
                        FILE *highestscorefile = fopen("highestscore.txt", "w");
                        if (highestscorefile != NULL)
                        {
                            fprintf(highestscorefile, "%d", highestscore);
                            fclose(highestscorefile);
                        }
                    }
                }
                else if (balloons[i].gold == true)
                {
                    arrowsleft += 2;
                    score += 10;
                    PlaySound(popsound);
                    Popup(scorepopup, balloons[i].position, 10);
                    ArrowPopup(arrowpopup, (Vector2){balloons[i].position.x, balloons[i].position.y - 40.0f}, 2);
                }
                else
                {
                    // Handles standard popping for Normal and Must Pop balloons
                    score += 10;
                    Popup(scorepopup, balloons[i].position, 10);
                    PlaySound(popsound);
                }
            }
        }

        // gameover and highscore save
        if (arrowsleft == 0 && arrow.active == false && gameover == false)
        {
            gameover = true;
            StopMusicStream(bgmusic);
            PlaySound(gameoversound);

            if (score > highestscore)
            {
                highestscore = score;
                FILE *highestscorefile = fopen("highestscore.txt", "w");
                if (highestscorefile != NULL)
                {
                    fprintf(highestscorefile, "%d", highestscore);
                    fclose(highestscorefile);
                }
            }
        }

        // restart logic
        if (gameover == true && IsKeyPressed(KEY_R) == true)
        {
            gameover = false;
            arrowsleft = 10;
            score = 0;
            currenttimer = 0.0f;
            launchspeed = 0.0f;
            pulldistance = 0.0f;

            arrow.active = false;
            for (int i = 0; i < MAXBALLOONS; i++)
            {
                balloons[i].active = false;
            }
            StopSound(gameoversound);
            PlayMusicStream(bgmusic);
        }

        // drawing phase
        BeginDrawing();
        ClearBackground(RAYWHITE);
        Rectangle bgsource = {0.0f, 0.0f, (float)background.width, (float)background.height};
        Rectangle bgdest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
        DrawTexturePro(background, bgsource, bgdest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

        // draw boy sprite animation (hardcoded left of bow)
        float boyWidth = 396.0f;
        float boyHeight = 528.0f;
        Vector2 boyPos = {240.0f, 630.0f};
        Rectangle boySource = {0.0f, 0.0f, (float)boytextures[boycurrentframe].width, (float)boytextures[boycurrentframe].height};
        Rectangle boyDest = {boyPos.x, boyPos.y, boyWidth, boyHeight};
        DrawTexturePro(boytextures[boycurrentframe], boySource, boyDest, (Vector2){boyWidth / 2.0f, boyHeight / 2.0f}, 0.0f, WHITE);

        // bow drawing
        float bowwidth = 220.0f;
        float bowheight = 220.0f;
        Rectangle bowsource = {0.0f, 0.0f, (float)bowimage.width, (float)bowimage.height};
        Rectangle bowdest = {arrowpivot.x, arrowpivot.y, bowwidth, bowheight};
        Vector2 boworigin = {bowwidth / 2.0f, bowheight / 2.0f};
        DrawTexturePro(bowimage, bowsource, bowdest, boworigin, aimangle * RAD2DEG, WHITE);

        Vector2 bowstringtop = Vector2Add(arrowpivot, Vector2Rotate((Vector2){0.0, 90.0f}, aimangle));
        Vector2 bowstringbottom = Vector2Add(arrowpivot, Vector2Rotate((Vector2){0.0f, -90.0f}, aimangle));
        Vector2 pullpoint = Vector2Subtract(arrowpivot, Vector2Scale(aimdirection, pulldistance));
        DrawLineEx(bowstringbottom, pullpoint, 3.5f, LIGHTGRAY);
        DrawLineEx(bowstringtop, pullpoint, 3.5f, LIGHTGRAY);

        // arrow drawing (resting on string / in flight)
        float arrowwidth = 110.0f;
        float arrowheight = 45.0f;
        if (gameover == false && arrow.active == false && arrowsleft > 0)
        {
            Vector2 restPos = Vector2Subtract(arrowpivot, Vector2Scale(aimdirection, pulldistance * 0.5f));
            Rectangle arrowsource = {0.0f, 0.0f, (float)arrowimage.width, (float)arrowimage.height};
            Rectangle arrowdest = {restPos.x, restPos.y, arrowwidth, arrowheight};
            Vector2 arroworigin = {arrowwidth / 2.0f, arrowheight / 2.0f};
            DrawTexturePro(arrowimage, arrowsource, arrowdest, arroworigin, aimangle * RAD2DEG, WHITE);
        }
        else if (gameover == false && arrow.active == true)
        {
            float arrowangle = atan2f(arrow.velocity.y, arrow.velocity.x);
            Rectangle arrowsource = {0.0f, 0.0f, (float)arrowimage.width, (float)arrowimage.height};
            Rectangle arrowdest = {arrow.position.x, arrow.position.y, arrowwidth, arrowheight};
            Vector2 arroworigin = {arrowwidth / 2.0f, arrowheight / 2.0f};
            DrawTexturePro(arrowimage, arrowsource, arrowdest, arroworigin, arrowangle * RAD2DEG, WHITE);
        }

        // balloon drawing
        for (int i = 0; i < MAXBALLOONS; i++)
        {
            if (balloons[i].active == true)
            {
                if (balloons[i].danger == true)
                {
                    Rectangle loonSource = {0, 0, (float)dangerballoon.width, (float)dangerballoon.height};
                    Rectangle loonDest = {balloons[i].position.x, balloons[i].position.y, dangerDrawWidth, dangerDrawHeight};
                    Vector2 loonOrigin = {dangerDrawWidth / 2.0f, dangerDrawHeight / 2.0f};
                    DrawTexturePro(dangerballoon, loonSource, loonDest, loonOrigin, 0.0f, WHITE);
                }
                else if (balloons[i].mustpop == true)
                {
                    Rectangle loonSource = {0, 0, (float)mustpopballoon.width, (float)mustpopballoon.height};
                    Rectangle loonDest = {balloons[i].position.x, balloons[i].position.y, mustPopDrawWidth, mustPopDrawHeight};
                    Vector2 loonOrigin = {mustPopDrawWidth / 2.0f, mustPopDrawHeight / 2.0f};
                    DrawTexturePro(mustpopballoon, loonSource, loonDest, loonOrigin, 0.0f, WHITE);
                }
                else
                {
                    Texture2D balloonTex;
                    if (balloons[i].gold)
                    {
                        balloonTex = arrowballoon;
                    }
                    else
                    {
                        balloonTex = normalballoons[balloons[i].index];
                    }

                    Rectangle loonSource = {0, 0, (float)balloonTex.width, (float)balloonTex.height};
                    Rectangle loonDest = {balloons[i].position.x, balloons[i].position.y, balloonDrawSize, balloonDrawSize};
                    Vector2 loonOrigin = {balloonDrawSize / 2.0f, balloonDrawSize / 2.0f};
                    DrawTexturePro(balloonTex, loonSource, loonDest, loonOrigin, 0.0f, WHITE);
                }
            }
        }

        // drawing texts
        DrawTextEx(customfont, TextFormat("SCORE: %d", score), (Vector2){32, 23}, 42, 2, BLACK);
        DrawTextEx(customfont, TextFormat("SCORE: %d", score), (Vector2){30, 25}, 42, 2, WHITE);
        DrawTextEx(customfont, TextFormat("ARROWS: %d", arrowsleft), (Vector2){32, 73}, 42, 2, BLACK);
        DrawTextEx(customfont, TextFormat("ARROWS: %d", arrowsleft), (Vector2){30, 75}, 42, 2, WHITE);
        DrawTextEx(customfont, TextFormat("HIGHEST SCORE: %d", highestscore), (Vector2){32, 123}, 42, 2, BLACK);
        DrawTextEx(customfont, TextFormat("HIGHEST SCORE: %d", highestscore), (Vector2){30, 125}, 42, 2, GOLD);

        DrawTextEx(customfont, TextFormat("ANGLE: %.2f", -(aimangle * RAD2DEG)), (Vector2){30, 673}, 42, 2, WHITE);
        DrawTextEx(customfont, TextFormat("LAUNCH SPEED: %.2f", launchspeed), (Vector2){30, 723}, 42, 2, WHITE);

        // drawing scorepopups
        for (int i = 0; i < MAXPOPUPS; i++)
        {
            if (scorepopup[i].isactive)
            {
                DrawTextEx(customfont, TextFormat("+%d", scorepopup[i].value), scorepopup[i].position, 65, 2, BLACK);
                DrawTextEx(customfont, TextFormat("+%d", scorepopup[i].value), (Vector2){scorepopup[i].position.x + 2, scorepopup[i].position.y + 2}, 65, 2, GOLD);
            }
        }

        // drawing arrowpopups
        for (int i = 0; i < MAXARROWPOPUPS; i++)
        {
            if (arrowpopup[i].isactive)
            {
                Color popupColor = (arrowpopup[i].value < 0) ? RED : GREEN;
                const char *popupText = (arrowpopup[i].value < 0)
                                            ? TextFormat("%d ARROWS", arrowpopup[i].value)
                                            : TextFormat("+%d ARROWS", arrowpopup[i].value);

                DrawTextEx(customfont, popupText, arrowpopup[i].position, 50, 2, BLACK);
                DrawTextEx(customfont, popupText, (Vector2){arrowpopup[i].position.x + 2, arrowpopup[i].position.y + 2}, 50, 2, popupColor);
            }
        }

        // gameover screen
        if (gameover == true)
        {
            float gameoverwidth = (float)gameovertexture.width * 1.8f;
            float gameoverheight = (float)gameovertexture.height * 1.8f;
            Rectangle gameoversource = {0.0f, 0.0f, (float)gameovertexture.width, (float)gameovertexture.height};
            Rectangle gameoverdest = {800.0f, 300.0f, gameoverwidth, gameoverheight};
            Vector2 gameoverorigin = {gameoverwidth / 2.0f, gameoverheight / 2.0f};
            DrawTexturePro(gameovertexture, gameoversource, gameoverdest, gameoverorigin, 0.0f, WHITE);

            const char *restarttext = "PRESS R TO RESTART";
            DrawTextEx(customfont, restarttext, (Vector2){600.0f, 500.0f}, 48.0f, 2, BLACK);
            DrawTextEx(customfont, restarttext, (Vector2){598.0f, 498.0f}, 48.0f, 2, RAYWHITE);

            const char *scoretext = TextFormat("YOUR SCORE: %d", score);
            DrawTextEx(customfont, scoretext, (Vector2){600.0f, 600.0f}, 48.0f, 2, BLACK);
            DrawTextEx(customfont, scoretext, (Vector2){598.0f, 598.0f}, 48.0f, 2, RAYWHITE);

            const char *highscoretext = TextFormat("HIGHEST SCORE: %d", highestscore);
            DrawTextEx(customfont, highscoretext, (Vector2){600.0f, 650.0f}, 48.0f, 2, BLACK);
            DrawTextEx(customfont, highscoretext, (Vector2){598.0f, 648.0f}, 48.0f, 2, RAYWHITE);
        }
        EndDrawing();
    }

    // cleanup
    UnloadTexture(background);
    for (int i = 0; i < NORMALBALLONSNUM; i++)
    {
        UnloadTexture(normalballoons[i]);
    }
    for (int i = 0; i < 10; i++)
    {
        UnloadTexture(boytextures[i]);
    }
    UnloadTexture(arrowballoon);
    UnloadTexture(dangerballoon);
    UnloadTexture(mustpopballoon);
    UnloadTexture(gameovertexture);
    UnloadTexture(bowimage);
    UnloadTexture(arrowimage);
    UnloadFont(customfont);
    UnloadSound(shootsound);
    UnloadSound(popsound);
    UnloadMusicStream(bgmusic);
    UnloadSound(gameoversound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}