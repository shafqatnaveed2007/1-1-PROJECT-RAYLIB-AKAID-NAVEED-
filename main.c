// headers
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <math.h>
#include <stdbool.h>

// macros
#define WIDTH 1600
#define HEIGHT 800
#define NORMALBALLOONSNUM 4
#define SPAWNPOINTS 5
#define MAXBALLOONS 10
#define MAXPOPUPS 5
#define MAXARROWPOPUPS 5

// enum for gamestate
typedef enum
{
    GAME_MENU,
    GAME_PLAYING,
    GAME_HOWTOPLAY,
    GAME_HIGHESTSCORE
} GameState;

// structs for arrow, ballloons and popups
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
    bool active;
} ScorePopUp;

typedef struct
{
    Vector2 position;
    int value;
    float visibletime;
    bool active;
} ArrowPopUp;

// Scorepopup and arrowpopup functions
void ScorePopup(ScorePopUp scorepopup[], Vector2 pos, int score)
{
    for (int i = 0; i < MAXPOPUPS; i++)
    {
        if (scorepopup[i].active == false)
        {
            scorepopup[i].active = true;
            scorepopup[i].position = pos;
            scorepopup[i].value = score;
            scorepopup[i].visibletime = 1.0f;
            break;
        }
    }
}
void ArrowPopup(ArrowPopUp arrowpopup[], Vector2 pos, int arrows)
{
    for (int i = 0; i < MAXARROWPOPUPS; i++)
    {
        if (arrowpopup[i].active == false)
        {
            arrowpopup[i].active = true;
            arrowpopup[i].position = pos;
            arrowpopup[i].value = arrows;
            arrowpopup[i].visibletime = 1.0f;
            break;
        }
    }
}

// saving highest score to file
void SaveHighestScore(int score)
{
    FILE *savefile = fopen("highestscore.txt", "w");
    if (savefile != NULL)
    {
        fprintf(savefile, "%d", score);
        fclose(savefile);
    }
}

int main(void)
{
    InitWindow(WIDTH, HEIGHT, "HIT 'EM ALL");
    InitAudioDevice();
    SetTargetFPS(60);

    // Initialize Game State
    GameState currentstate = GAME_MENU;

    // loading all audio, textures, fonts
    Music bgmusic = LoadMusicStream("assets/audio/Game Window.mp3");
    Sound shootsound = LoadSound("assets/audio/Gun Shooting.ogg");
    Sound popsound = LoadSound("assets/audio/Balloon Pop.mp3");
    Sound gameoversound = LoadSound("assets/audio/Game Over.mp3");
    Sound clicksound = LoadSound("assets/audio/Button Click.wav");

    PlayMusicStream(bgmusic);

    Texture2D menubackground = LoadTexture("assets/sprites/menuscreen.png");
    Texture2D background = LoadTexture("assets/sprites/gamescreen.png");
    Texture2D gameovertexture = LoadTexture("assets/sprites/gameover.png");
    Texture2D bowimage = LoadTexture("assets/sprites/bow.png");
    Texture2D arrowimage = LoadTexture("assets/sprites/arrow.png");
    Texture2D arrowballoon = LoadTexture("assets/sprites/arrowballoon.png");
    Texture2D dangerballoon = LoadTexture("assets/sprites/dangerballoon.png");
    Texture2D mustpopballoon = LoadTexture("assets/sprites/mustpopballoon.png");
    Texture2D normalballoons[NORMALBALLOONSNUM];
    for (int i = 0; i < NORMALBALLOONSNUM; i++)
    {
        normalballoons[i] = LoadTexture(TextFormat("assets/sprites/normalballoon%d.png", i + 1));
    }
    Texture2D boytexture = LoadTexture("assets/sprites/boy.png");
    Font customfont = LoadFontEx("assets/fonts/Carnival Font.ttf", 96, NULL, 0);

    // init spawnpoints, arrow, balloons
    Vector2 spawnpoints[SPAWNPOINTS];
    for (int i = 0; i < SPAWNPOINTS; i++)
    {
        spawnpoints[i] = (Vector2){1000.0f + i * 130.0f, HEIGHT + 50.0f};
    }

    Arrow arrow = {0};
    Balloon balloons[MAXBALLOONS] = {0};
    // arrays for popups
    ScorePopUp scorepopup[MAXPOPUPS] = {0};
    ArrowPopUp arrowpopup[MAXARROWPOPUPS] = {0};

    // init game variables
    int score = 0;
    int highestscore = 0;
    int arrowsleft = 10;
    float gravity = 1000.0f;
    float currenttimer = 0.0f;
    float spawninterval = 2.0f;
    float pulldistance = 0.0f;
    float launchspeed = 0.0f;
    bool gameover = false;

    // balloon sizes
    float normalballoonwidth = 140.0f;
    float normalballoonheight = 140.0f;
    float balloonradius = normalballoonwidth * 0.4f;

    float dangerwidth = 300.0f;
    float dangerheight = 164.0f;
    float dangerradius = dangerwidth * 0.25f;

    float mustpopwidth = 180.0f;
    float mustpopheight = 180.0f;
    float mustpopradius = mustpopwidth * 0.4f;

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
        Vector2 mouseposition = GetMousePosition();
        Rectangle crossbutton = {WIDTH / 2.0f + 340.0f, HEIGHT / 2.0f - 240.0f, 40.0f, 40.0f};
        // game menu state
        if (currentstate == GAME_MENU)
        {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);

            Rectangle startbutton = {WIDTH / 2.0f - 120.0f, 350.0f, 260.0f, 50.0f};
            Rectangle howtoplaybutton = {WIDTH / 2.0f - 150.0f, 420.0f, 380.0f, 50.0f};
            Rectangle highestscorebutton = {WIDTH / 2.0f - 150.0f, 490.0f, 420.0f, 50.0f};
            Rectangle exitbutton = {WIDTH / 2.0f - 90.0f, 560.0f, 200.0f, 50.0f};

            if (CheckCollisionPointRec(mouseposition, startbutton) ||
                CheckCollisionPointRec(mouseposition, howtoplaybutton) ||
                CheckCollisionPointRec(mouseposition, highestscorebutton) ||
                CheckCollisionPointRec(mouseposition, exitbutton))
            {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                if (CheckCollisionPointRec(mouseposition, startbutton))
                {
                    PlaySound(clicksound);
                    currentstate = GAME_PLAYING;
                }
                else if (CheckCollisionPointRec(mouseposition, exitbutton))
                {
                    PlaySound(clicksound);
                    break;
                }
                else if (CheckCollisionPointRec(mouseposition, howtoplaybutton))
                {
                    PlaySound(clicksound);
                    currentstate = GAME_HOWTOPLAY;
                }
                else if (CheckCollisionPointRec(mouseposition, highestscorebutton))
                {
                    PlaySound(clicksound);
                    currentstate = GAME_HIGHESTSCORE;
                }
            }
        }
        else if (currentstate == GAME_PLAYING)
        {
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);

            // bow and arrow settings
            Vector2 arrowpivot = {280.0f, 590.0f};
            float aimangle = 0.0f;
            Vector2 aimdirection = {1.0f, 0.0f};
            float maxpulldistance = 120.0f;
            float minarrowspeed = 500.0f;
            float maxarrowspeed = 2500.0f;
            float pullspeed = 100.0f;

            // aiming, pulling back and shooting arrow
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
                if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT) && arrow.active == false && pulldistance > 15.0f)
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
            float basespeed = 150.0f;
            float speedincrease = 0.75f;
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
                            balloons[i].speed = basespeed + score * speedincrease;
                            balloons[i].active = true;

                            int dangerroll = (score >= 120) ? 30 : 0;
                            int mustpoproll = (score >= 80) ? 20 : 0;
                            int roll = GetRandomValue(1, 100);

                            if (roll <= dangerroll)
                            {
                                balloons[i].danger = true;
                                balloons[i].mustpop = false;
                                balloons[i].gold = false;
                                balloons[i].radius = dangerradius;
                            }
                            else if (roll <= dangerroll + mustpoproll)
                            {
                                balloons[i].danger = false;
                                balloons[i].mustpop = true;
                                balloons[i].gold = false;
                                balloons[i].radius = mustpopradius;
                            }
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

            // scorepopups and arrowpopups update
            for (int i = 0; i < MAXPOPUPS; i++)
            {
                if (scorepopup[i].active)
                {
                    scorepopup[i].visibletime -= dt;
                    scorepopup[i].position.y -= 20.0f * dt;
                    if (scorepopup[i].visibletime <= 0)
                    {
                        scorepopup[i].active = false;
                    }
                }
            }
            for (int i = 0; i < MAXARROWPOPUPS; i++)
            {
                if (arrowpopup[i].active)
                {
                    arrowpopup[i].visibletime -= dt;
                    arrowpopup[i].position.y -= 20.0f * dt;
                    if (arrowpopup[i].visibletime <= 0)
                    {
                        arrowpopup[i].active = false;
                    }
                }
            }

            // balloon movement & collision physics
            for (int i = 0; i < MAXBALLOONS; i++)
            {
                if (balloons[i].active == false)
                    continue;

                balloons[i].position.y -= dt * balloons[i].speed;

                if (balloons[i].position.y < -100.0f)
                {
                    balloons[i].active = false;
                    if (balloons[i].mustpop == true)
                    {
                        arrowsleft -= 2;
                        if (arrowsleft < 0)
                            arrowsleft = 0;
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
                            SaveHighestScore(highestscore);
                        }
                    }
                    else if (balloons[i].gold == true)
                    {
                        arrowsleft += 2;
                        score += 10;
                        PlaySound(popsound);
                        ScorePopup(scorepopup, balloons[i].position, 10);
                        ArrowPopup(arrowpopup, (Vector2){balloons[i].position.x, balloons[i].position.y - 40.0f}, 2);
                    }
                    else
                    {
                        score += 10;
                        ScorePopup(scorepopup, balloons[i].position, 10);
                        PlaySound(popsound);
                    }
                }
            }

            // gameover and highscore saving
            if (arrowsleft == 0 && arrow.active == false && gameover == false)
            {
                gameover = true;
                StopMusicStream(bgmusic);
                PlaySound(gameoversound);

                if (score > highestscore)
                {
                    highestscore = score;
                    SaveHighestScore(highestscore);
                }
            }

            // restart logic
            if (gameover == true)
            {
                Rectangle menubutton = {WIDTH - 270.0f, 40.0f, 220.0f, 100.0f};
                bool ishovered = CheckCollisionPointRec(mouseposition, menubutton);

                if (ishovered)
                {
                    SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
                }
                if (IsKeyPressed(KEY_R) == true)
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
                else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && ishovered)
                {
                    PlaySound(clicksound);
                    SetMouseCursor(MOUSE_CURSOR_DEFAULT);

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
                    currentstate = GAME_MENU;
                }
            }
        }
        else if (currentstate == GAME_HOWTOPLAY)
        {
            Rectangle crossbutton = {WIDTH / 2.0f + 340.0f, HEIGHT / 2.0f - 240.0f, 40.0f, 40.0f};

            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            if (CheckCollisionPointRec(mouseposition, crossbutton))
            {
                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseposition, crossbutton))
            {
                PlaySound(clicksound);
                currentstate = GAME_MENU;
            }
        }
        else if (currentstate == GAME_HIGHESTSCORE)
        {
            Rectangle crossbutton = {WIDTH / 2.0f + 340.0f, HEIGHT / 2.0f - 240.0f, 40.0f, 40.0f};
            SetMouseCursor(MOUSE_CURSOR_DEFAULT);
            if (CheckCollisionPointRec(mouseposition, crossbutton))
            {

                SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            }
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && CheckCollisionPointRec(mouseposition, crossbutton))
            {
                PlaySound(clicksound);
                currentstate = GAME_MENU;
            }
        }

        // all drawings
        BeginDrawing();
        ClearBackground(RAYWHITE);

        if (currentstate == GAME_MENU)
        {
            // Draw Menu Background
            Rectangle menusource = {0.0f, 0.0f, (float)menubackground.width, (float)menubackground.height};
            Rectangle menudest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
            DrawTexturePro(menubackground, menusource, menudest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

            // Menu Option Rectangles
            Rectangle startbutton = {WIDTH / 2.0f - 120.0f, 350.0f, 260.0f, 50.0f};
            Rectangle howtoplaybutton = {WIDTH / 2.0f - 120.0f, 420.0f, 380.0f, 50.0f};
            Rectangle highestscorebutton = {WIDTH / 2.0f - 120.0f, 490.0f, 420.0f, 50.0f};
            Rectangle exitbutton = {WIDTH / 2.0f - 90.0f, 560.0f, 200.0f, 50.0f};

            Color startcolor = CheckCollisionPointRec(mouseposition, startbutton) ? GOLD : BLACK;
            Color howtoplaycolor = CheckCollisionPointRec(mouseposition, howtoplaybutton) ? GOLD : BLACK;
            Color highestscorecolor = CheckCollisionPointRec(mouseposition, highestscorebutton) ? GOLD : BLACK;
            Color exitcolor = CheckCollisionPointRec(mouseposition, exitbutton) ? GOLD : BLACK;

            // Text coordinates
            DrawTextEx(customfont, "PLAY", (Vector2){WIDTH / 2.0f - 50.0f, 350.0f}, 48, 2, startcolor);
            DrawTextEx(customfont, "HOW TO PLAY", (Vector2){WIDTH / 2.0f - 120.0f, 420.0f}, 48, 2, howtoplaycolor);
            DrawTextEx(customfont, "HIGHEST SCORE", (Vector2){WIDTH / 2.0f - 130.0f, 490.0f}, 48, 2, highestscorecolor);
            DrawTextEx(customfont, "EXIT", (Vector2){WIDTH / 2.0f - 50.0f, 560.0f}, 48, 2, exitcolor);
        }
        else if (currentstate == GAME_HOWTOPLAY)
        {
            Rectangle menusource = {0.0f, 0.0f, (float)menubackground.width, (float)menubackground.height};
            Rectangle menudest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
            DrawTexturePro(menubackground, menusource, menudest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

            Color backgrounddim = {0, 0, 0, 150};
            DrawRectangle(0, 0, WIDTH, HEIGHT, backgrounddim);
            Rectangle howtoplaypanel = {WIDTH / 2 - 400, HEIGHT / 2 - 250, 800, 500};
            DrawRectangleRounded(howtoplaypanel, 0.05f, 8, (Color){245, 235, 210, 255});

            Rectangle crossbutton = {WIDTH / 2.0f + 340.0f, HEIGHT / 2.0f - 240.0f, 40.0f, 40.0f};
            Color crossbuttoncolor = CheckCollisionPointRec(mouseposition, crossbutton) ? GOLD : BLACK;
            DrawTextEx(customfont, "X", (Vector2){WIDTH / 2.0f + 350.0f, HEIGHT / 2.0f - 235.0f}, 32, 2, crossbuttoncolor);

            const char *instructions[] = {
                "AIM your bow by moving the mouse.",
                "Click and HOLD to pull back the string.",
                "RELEASE to fire your arrow!",
                "",
                "Watch out for special balloons:",
                "GOLD balloons give bonus arrows and points.",
                "POP balloons must be popped before they escape!",
                "DANGER balloons end your game instantly!",
                "",
                "You start with 10 arrows. Good luck!"};
            int numberoflines = sizeof(instructions) / sizeof(instructions[0]);
            for (int i = 0; i < numberoflines; i++)
            {
                float linepositionY = HEIGHT / 2.0f - 200.0f + (i * 40.0f);
                Color textcolor = (Color){60, 38, 22, 255};
                DrawTextEx(customfont, instructions[i], (Vector2){WIDTH / 2.0f - 380.0f, linepositionY}, 32, 2, textcolor);
            }
        }
        else if (currentstate == GAME_HIGHESTSCORE)
        {
            Rectangle menusource = {0.0f, 0.0f, (float)menubackground.width, (float)menubackground.height};
            Rectangle menudest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
            DrawTexturePro(menubackground, menusource, menudest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

            Color backgrounddim = {0, 0, 0, 150};
            DrawRectangle(0, 0, WIDTH, HEIGHT, backgrounddim);
            Rectangle highestscorepanel = {WIDTH / 2 - 400, HEIGHT / 2 - 250, 800, 500};
            DrawRectangleRounded(highestscorepanel, 0.05f, 8, (Color){245, 235, 210, 255});

            Rectangle crossbutton = {WIDTH / 2.0f + 340.0f, HEIGHT / 2.0f - 240.0f, 40.0f, 40.0f};
            Color crossbuttoncolor = CheckCollisionPointRec(mouseposition, crossbutton) ? GOLD : BLACK;
            DrawTextEx(customfont, "X", (Vector2){WIDTH / 2.0f + 350.0f, HEIGHT / 2.0f - 235.0f}, 32, 2, crossbuttoncolor);

            const char *titletext = "HIGHEST SCORE";
            DrawTextEx(customfont, titletext, (Vector2){WIDTH / 2.0f - 160.0f, HEIGHT / 2.0f - 150.0f}, 54, 2, (Color){60, 38, 22, 255});
            const char *scoretext = TextFormat("%d", highestscore);
            Vector2 scoretextSize = MeasureTextEx(customfont, scoretext, 80, 2);
            DrawTextEx(customfont, scoretext, (Vector2){WIDTH / 2.0f - scoretextSize.x / 2.0f, HEIGHT / 2.0f - 30.0f}, 80, 2, (Color){60, 38, 22, 255});
        }
        else if (currentstate == GAME_PLAYING)
        {
            Rectangle bgsource = {0.0f, 0.0f, (float)background.width, (float)background.height};
            Rectangle bgdest = {0.0f, 0.0f, (float)WIDTH, (float)HEIGHT};
            DrawTexturePro(background, bgsource, bgdest, (Vector2){0.0f, 0.0f}, 0.0f, WHITE);

            // Draw boy sprite
            float boyWidth = 396.0f;
            float boyHeight = 528.0f;
            Vector2 boyPos = {240.0f, 630.0f};
            Rectangle boySource = {0.0f, 0.0f, (float)boytexture.width, (float)boytexture.height};
            Rectangle boyDest = {boyPos.x, boyPos.y, boyWidth, boyHeight};
            DrawTexturePro(boytexture, boySource, boyDest, (Vector2){boyWidth / 2.0f, boyHeight / 2.0f}, 0.0f, WHITE);

            // bow, arrow drawing variables
            Vector2 arrowpivot = {280.0f, 590.0f};
            float aimangle = 0.0f;
            Vector2 aimdirection = {1.0f, 0.0f};
            float mousepointerangle = atan2f(mouseposition.y - arrowpivot.y, mouseposition.x - arrowpivot.x);
            if (mousepointerangle > PI / 4.0)
                aimangle = PI / 4.0;
            else if (mousepointerangle < -PI / 4.0)
                aimangle = -PI / 4.0;
            else
                aimangle = mousepointerangle;
            aimdirection = (Vector2){cosf(aimangle), sinf(aimangle)};

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

            float arrowwidth = 110.0f;
            float arrowheight = 45.0f;
            if (gameover == false && arrow.active == false && arrowsleft > 0)
            {
                Vector2 pullpoint = Vector2Subtract(arrowpivot, Vector2Scale(aimdirection, pulldistance * 0.5f));
                Rectangle arrowsource = {0.0f, 0.0f, (float)arrowimage.width, (float)arrowimage.height};
                Rectangle arrowdest = {pullpoint.x, pullpoint.y, arrowwidth, arrowheight};
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
                        Rectangle balloonsource = {0, 0, (float)dangerballoon.width, (float)dangerballoon.height};
                        Rectangle balloondest = {balloons[i].position.x, balloons[i].position.y, dangerwidth, dangerheight};
                        Vector2 balloonorigin = {dangerwidth / 2.0f, dangerheight / 2.0f};
                        DrawTexturePro(dangerballoon, balloonsource, balloondest, balloonorigin, 0.0f, WHITE);
                    }
                    else if (balloons[i].mustpop == true)
                    {
                        Rectangle balloonsource = {0, 0, (float)mustpopballoon.width, (float)mustpopballoon.height};
                        Rectangle balloondest = {balloons[i].position.x, balloons[i].position.y, mustpopwidth, mustpopheight};
                        Vector2 balloonorigin = {mustpopwidth / 2.0f, mustpopheight / 2.0f};
                        DrawTexturePro(mustpopballoon, balloonsource, balloondest, balloonorigin, 0.0f, WHITE);
                    }
                    else
                    {
                        Texture2D balloontexture;
                        if (balloons[i].gold)
                            balloontexture = arrowballoon;
                        else
                            balloontexture = normalballoons[balloons[i].index];

                        Rectangle balloonsource = {0, 0, (float)balloontexture.width, (float)balloontexture.height};
                        Rectangle balloondest = {balloons[i].position.x, balloons[i].position.y, normalballoonwidth, normalballoonheight};
                        Vector2 balloonorigin = {normalballoonwidth / 2.0f, normalballoonheight / 2.0f};
                        DrawTexturePro(balloontexture, balloonsource, balloondest, balloonorigin, 0.0f, WHITE);
                    }
                }
            }

            // drawing texts
            DrawTextEx(customfont, TextFormat("SCORE: %d", score), (Vector2){32, 23}, 42, 2, BLACK);
            DrawTextEx(customfont, TextFormat("SCORE: %d", score), (Vector2){30, 25}, 42, 2, WHITE);
            DrawTextEx(customfont, TextFormat("ARROWS: %d", arrowsleft), (Vector2){32, 73}, 42, 2, BLACK);
            DrawTextEx(customfont, TextFormat("ARROWS: %d", arrowsleft), (Vector2){30, 75}, 42, 2, GOLD);

            DrawTextEx(customfont, TextFormat("ANGLE: %.2f", -(aimangle * RAD2DEG)), (Vector2){32, 671}, 42, 2, BLACK);
            DrawTextEx(customfont, TextFormat("ANGLE: %.2f", -(aimangle * RAD2DEG)), (Vector2){30, 673}, 42, 2, WHITE);
            DrawTextEx(customfont, TextFormat("LAUNCH SPEED: %.2f", launchspeed), (Vector2){32, 721}, 42, 2, BLACK);
            DrawTextEx(customfont, TextFormat("LAUNCH SPEED: %.2f", launchspeed), (Vector2){30, 723}, 42, 2, GOLD);

            // drawing scorepopups
            for (int i = 0; i < MAXPOPUPS; i++)
            {
                if (scorepopup[i].active)
                {
                    DrawTextEx(customfont, TextFormat("+%d", scorepopup[i].value), scorepopup[i].position, 65, 2, BLACK);
                    DrawTextEx(customfont, TextFormat("+%d", scorepopup[i].value), (Vector2){scorepopup[i].position.x + 2, scorepopup[i].position.y + 2}, 65, 2, GOLD);
                }
            }

            // drawing arrowpopups
            for (int i = 0; i < MAXARROWPOPUPS; i++)
            {
                if (arrowpopup[i].active)
                {
                    Color popupColor = (arrowpopup[i].value < 0) ? RED : GREEN;
                    const char *arrowpopuptext = (arrowpopup[i].value < 0) ? TextFormat("%d ARROWS", arrowpopup[i].value) : TextFormat("+%d ARROWS", arrowpopup[i].value);

                    DrawTextEx(customfont, arrowpopuptext, arrowpopup[i].position, 50, 2, BLACK);
                    DrawTextEx(customfont, arrowpopuptext, (Vector2){arrowpopup[i].position.x + 2, arrowpopup[i].position.y + 2}, 50, 2, popupColor);
                }
            }

            // gameover screen
            if (gameover == true)
            {
                float gameoverwidth = (float)gameovertexture.width * 1.8f;
                float gameoverheight = (float)gameovertexture.height * 1.8f;
                Rectangle gameoversource = {0.0f, 0.0f, (float)gameovertexture.width, (float)gameovertexture.height};
                Rectangle gameoverdest = {820.0f, 300.0f, gameoverwidth, gameoverheight};
                Vector2 gameoverorigin = {gameoverwidth / 2.0f, gameoverheight / 2.0f};
                DrawTexturePro(gameovertexture, gameoversource, gameoverdest, gameoverorigin, 0.0f, WHITE);

                Rectangle textpanel = {540.0f, 480.0f, 560.0f, 250.0f};
                Color paneldim = {0, 0, 0, 140};
                DrawRectangleRounded(textpanel, 0.2f, 8, paneldim);

                const char *restarttext = "PRESS R TO RESTART";
                DrawTextEx(customfont, restarttext, (Vector2){600.0f, 500.0f}, 48.0f, 2, BLACK);
                DrawTextEx(customfont, restarttext, (Vector2){598.0f, 498.0f}, 48.0f, 2, GOLD);

                const char *scoretext = TextFormat("YOUR SCORE: %d", score);
                DrawTextEx(customfont, scoretext, (Vector2){600.0f, 600.0f}, 48.0f, 2, BLACK);
                DrawTextEx(customfont, scoretext, (Vector2){598.0f, 598.0f}, 48.0f, 2, WHITE);

                const char *highscoretext = TextFormat("HIGHEST SCORE: %d", highestscore);
                DrawTextEx(customfont, highscoretext, (Vector2){600.0f, 650.0f}, 48.0f, 2, BLACK);
                DrawTextEx(customfont, highscoretext, (Vector2){598.0f, 648.0f}, 48.0f, 2, GOLD);

                Rectangle menubutton = {WIDTH - 270.0f, 40.0f, 220.0f, 100.0f};
                bool ishovered = CheckCollisionPointRec(mouseposition, menubutton);
                Color buttoncolor = ishovered ? GOLD : RAYWHITE;
                DrawRectangleRounded(menubutton, 0.3f, 4, (Color){0, 0, 0, 150});
                DrawRectangleRoundedLines(menubutton, 0.3f, 16, buttoncolor);
                DrawTextEx(customfont, "MAIN MENU", (Vector2){menubutton.x + 20.0f, menubutton.y + 20.0f}, 40.0f, 2, buttoncolor);
            }
        }

        EndDrawing();
    }

    // all unloading
    UnloadTexture(menubackground);
    UnloadTexture(background);
    for (int i = 0; i < NORMALBALLOONSNUM; i++)
    {
        UnloadTexture(normalballoons[i]);
    }

    UnloadTexture(boytexture);
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
    UnloadSound(clicksound);
    CloseAudioDevice();
    CloseWindow();
    return 0;
}