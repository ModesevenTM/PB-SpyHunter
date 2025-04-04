#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include<windows.h>
#include <time.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#define SCREEN_WIDTH			1280
#define SCREEN_HEIGHT			720
#define TEXT_WIDTH				120

#define DEFAULT_SPEED			1.0
#define ACCEL_SPEED				2.0
#define DECEL_SPEED				0.3
#define ACCEL_PENALTY			3

#define START_TIME				50
#define DIST_BEFORE_ROAD_RESET	10
#define V_NARROW_ADD_TIME		2
#define DELTA_MULTIPLIER		1000
#define KILL_TIMER_VALUE		1
#define CIVIL_KILL_TIMER		2

#define ROAD_ENUM_ELEMS			4
#define ROAD_ARRAY_ELEMS		4
#define ROAD_START_SEQUENCE		0
#define ROAD_END_SEQUENCE		1
#define DEFAULT_ROAD			narrow
#define ROAD_SIDE_COUNT			2

#define CAR_HEIGHT				100
#define CAR_WIDTH				50
#define TRAFFIC_SPEED			0.7
#define ATTACK_SPEED			2.3
#define NO_MOVEMENT				0
#define DEFAULT_CLASS			civil
#define DEFAULT_MODE			false

#define DIRECTION_LEFT			true
#define DIRECTION_RIGHT			false
#define RNG_LEFT_SIDE			0
#define RNG_RIGHT_SIDE			1
#define RNG_MIDDLE_SIDE			2

#define OPPONENT_KILL_PTS		250
#define NO_SECONDARY_AMMO		-1
#define SECONDARY_AMMO			10
#define BULLET_WIDTH			7
#define BULLET_HEIGHT			20

#define NOT_PLACED				-1
#define H_GRASS_WIDE_LEFT		2 * SCREEN_WIDTH / 20 + CAR_WIDTH / 2 + 1
#define H_GRASS_WIDE_RIGHT		18 * SCREEN_WIDTH / 20 - 25
#define H_GRASS_MIDDLE_LEFT		6 * SCREEN_WIDTH / 15 - 25
#define H_GRASS_MIDDLE_RIGHT	9 * SCREEN_WIDTH / 15 + CAR_WIDTH / 2 + 1
#define H_GRASS_NARROW_LEFT		4 * SCREEN_WIDTH / 20 + CAR_WIDTH / 2 + 1
#define H_GRASS_NARROW_RIGHT	16 * SCREEN_WIDTH / 20 - 25
#define H_GRASS_VNARROW_LEFT	6 * SCREEN_WIDTH / 20 + CAR_WIDTH / 2 + 1
#define H_GRASS_VNARROW_RIGHT	14 * SCREEN_WIDTH / 20 - 25

#define BOX_HEIGHT				36
#define DIFF_ARRAY_LEN			4
#define FULL_COLOR				255

#define NEW_CAR_ADDITION		2500
#define POINT_ADDITION			50

#define INITIAL_GRASS_SHIFT		15
#define GRASS_MULTIPLIER		30

enum roadWidth {
	wide,
	narrow,
	veryNarrow,
	middleGrass
};

enum carType {
	civil,
	opponent
};

struct logic {
	struct {
		double toEnd = START_TIME;							// after this time player doesn't have infinite lives
		int t1 = SDL_GetTicks();							// previous timestamp
		int t2 = 0;											// current timestamp
		double delta = 0;									// time delta between t2 and t1
	} time;
	int xPosition = SCREEN_WIDTH / 2;						// player's position in x axis
	double speed = DEFAULT_SPEED;							// player's speed
	int points = 0;											// number of points accumulated by player
	int lives = 0;											// player's lives
	double deathTimer = 0;									// time before player respawns after death
	double civilKillTimer = 0;								// time before player can earn points
	double timeBeforeNewEnemy = 0;							// time before any enemy can be generated
	double distance = 0;									// distance driven by player
	double penalty = 0;										// penalty points accumulated for killing civil cars and going off the track
	bool isOffroad = false;									// true if car is off the track
	int opponentKills = 0;									// amount of killed opponent vehicles
	enum roadWidth road = DEFAULT_ROAD;						// current road type
	double distBeforeRoadChange = DIST_BEFORE_ROAD_RESET;
	struct {
		bool left = false;
		bool right = false;
		bool middle = false;
	} tree;													// true if there is a tree on any of the sides
	struct {
		int time = 0;
		int xPosition = 0;
	} shot;													// time and x-axis position of a bullet
	struct {
		int xPosition = NOT_PLACED;
		int yPosition = NOT_PLACED;
		int amount = NO_SECONDARY_AMMO;
	} ammo;													// powerup screen position and secondary ammo amount
	bool gameOver = false;									// state of the game
};

struct assets {
	SDL_Surface* charset = SDL_LoadBMP("Assets/cs8x8.bmp");;
	SDL_Surface* player = SDL_LoadBMP("Assets/player.bmp");
	SDL_Surface* civil = SDL_LoadBMP("Assets/civil.bmp");
	SDL_Surface* opponent = SDL_LoadBMP("Assets/opponent.bmp");
	SDL_Surface* explosion = SDL_LoadBMP("Assets/explosion.bmp");
	SDL_Surface* tree = SDL_LoadBMP("Assets/tree.bmp");
	SDL_Surface* powerup = SDL_LoadBMP("Assets/powerup.bmp");
};

struct traffic {
	enum carType type = DEFAULT_CLASS;	// car type
	double xPosition = NOT_PLACED;		// x-axis car position
	int yPosition = -CAR_HEIGHT / 2;	// y-axis car position
	double speed = TRAFFIC_SPEED;		// speed of the car
	bool attackMode = DEFAULT_MODE;		// true if an opponent attacks you
	int startX = 0;						// attack start position
	int targetX = 0;					// attack destination
	double killTimer = 0;				// if positive, explosion is visible
	struct traffic* next = NULL;		// pointer to the next list element
};

// draw a text txt on surface screen, starting from the point (x, y)
// charset is a 128x128 bitmap containing character images
void DrawString(SDL_Surface* screen, int x, int y, const char* text,
	SDL_Surface* charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while (*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
	};
};

// draw a surface sprite on a surface screen in point (x, y)
// (x, y) is the center of sprite on screen
void DrawSurface(SDL_Surface* screen, SDL_Surface* sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
};


// draw a single pixel
void DrawPixel(SDL_Surface* surface, int x, int y, Uint32 color) {
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	*(Uint32*)p = color;
};

// getting pixel data
Uint32 GetPixel(SDL_Surface* surface, int x, int y)
{
	int bpp = surface->format->BytesPerPixel;
	Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
	return *(Uint32*)p;
}


// draw a vertical (when dx = 0, dy = 1) or horizontal (when dx = 1, dy = 0) line
void DrawLine(SDL_Surface* screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for (int i = 0; i < l; i++) {
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};

// draw a rectangle of size l by k
void DrawRectangle(SDL_Surface* screen, int x, int y, int l, int k,
	Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for (i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
};

// random number generator, generates number from <first, last> range
int RNG(int first, int last)
{
	srand(time(NULL));
	return rand() % (last - first + 1) + first;
}

// crash the program if some assets aren't found
int CrashWhenNotFound(SDL_Surface* element, SDL_Surface* screen, SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer* renderer, SDL_Surface* charset = NULL)
{
	if (element == NULL) {
		printf("SDL_LoadBMP error: %s\n", SDL_GetError());
		if (charset != NULL) SDL_FreeSurface(charset);
		SDL_FreeSurface(screen);
		SDL_DestroyTexture(scrtex);
		SDL_DestroyWindow(window);
		SDL_DestroyRenderer(renderer);
		SDL_Quit();
		return 1;
	};
	return 0;
}

// crash the program if the window can't be created
int WindowCreationErrorCheck(int rc)
{
	if (rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};
	return 0;
}

// first game initialization
int GameInit(SDL_Window** window, SDL_Renderer** renderer)
{
	int rc;

	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}

	rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, window, renderer);
	WindowCreationErrorCheck(rc);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(*renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(*renderer, 0, 0, 0, 255);

	SDL_SetWindowTitle(*window, "Jakub Wojtkowiak 193546 - Spy Hunter");

	// disabling cursor visibility
	SDL_ShowCursor(SDL_DISABLE);
	return 0;
}

// fullscreen toggle
void FullScreen(SDL_Window* window, bool* isWindowed)
{
	if (*isWindowed)
	{
		SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
		*isWindowed = false;
	}
	else
	{
		SDL_SetWindowFullscreen(window, false);
		*isWindowed = true;
	}
}

// freeing surfaces before program ends
void FreeSurfaces(SDL_Surface* charset, SDL_Surface* screen, SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer* renderer)
{
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
}

// random x-axis position generator
int HorPosition(enum roadWidth road)
{
	int position;
	if (road == wide)
		position = RNG(H_GRASS_WIDE_LEFT, H_GRASS_WIDE_RIGHT);
	else if (road == middleGrass) {
		int whichSide = RNG(RNG_LEFT_SIDE, RNG_RIGHT_SIDE);
		if (whichSide == RNG_LEFT_SIDE)
			position = RNG(H_GRASS_WIDE_LEFT, H_GRASS_MIDDLE_LEFT);
		else
			position = RNG(H_GRASS_MIDDLE_RIGHT, H_GRASS_WIDE_RIGHT);
	}
	else if (road == narrow)
		position = RNG(H_GRASS_NARROW_LEFT, H_GRASS_NARROW_RIGHT);
	else
		position = RNG(H_GRASS_VNARROW_LEFT, H_GRASS_VNARROW_RIGHT);
	return position;
}

// draw green grass borders with appropriate width
void DrawBorders(SDL_Surface* screen, struct logic* gameData)
{
	int green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);

	// drawing default road grass border
	DrawRectangle(screen, 0, BOX_HEIGHT, SCREEN_WIDTH / 10, SCREEN_HEIGHT - BOX_HEIGHT, green, green);
	DrawRectangle(screen, 9 * SCREEN_WIDTH / 10, BOX_HEIGHT, SCREEN_WIDTH / 10, SCREEN_HEIGHT - BOX_HEIGHT, green, green);

	int maxRoadLen = DIST_BEFORE_ROAD_RESET;
	int diff[DIFF_ARRAY_LEN] = { 0 };

	// very narrow road lasts 2s longer by default, so the time needs to be added
	if (gameData->road == veryNarrow) maxRoadLen += V_NARROW_ADD_TIME;

	if (gameData->road == middleGrass) {
		for (int i = 0; i < ROAD_SIDE_COUNT; i++)
		{
			if (gameData->distBeforeRoadChange < DIST_BEFORE_ROAD_RESET - i && gameData->distBeforeRoadChange > i)
			{
				if (gameData->distBeforeRoadChange > DIST_BEFORE_ROAD_RESET - i - 1)
					diff[ROAD_START_SEQUENCE] = (SCREEN_HEIGHT - BOX_HEIGHT) * (gameData->distBeforeRoadChange - DIST_BEFORE_ROAD_RESET + i + 1);
				else if (gameData->distBeforeRoadChange < i + 1)
					diff[ROAD_END_SEQUENCE] = (SCREEN_HEIGHT - BOX_HEIGHT) * (gameData->distBeforeRoadChange - i - 1);
				DrawRectangle(screen, (7 - i) * SCREEN_WIDTH / 15, BOX_HEIGHT - diff[ROAD_END_SEQUENCE], SCREEN_WIDTH / 15, SCREEN_HEIGHT - BOX_HEIGHT - diff[ROAD_START_SEQUENCE] + diff[ROAD_END_SEQUENCE], green, green);
				DrawRectangle(screen, (7 + i) * SCREEN_WIDTH / 15, BOX_HEIGHT - diff[ROAD_END_SEQUENCE], SCREEN_WIDTH / 15, SCREEN_HEIGHT - BOX_HEIGHT - diff[ROAD_START_SEQUENCE] + diff[ROAD_END_SEQUENCE], green, green);
			}
		}
	}
	else if (gameData->road == narrow) {
		for (int i = 0; i < ROAD_SIDE_COUNT; i++)
		{
			if (gameData->distBeforeRoadChange < DIST_BEFORE_ROAD_RESET - i)
			{
				if (gameData->distBeforeRoadChange > DIST_BEFORE_ROAD_RESET - i - 1)
					diff[ROAD_START_SEQUENCE] = (SCREEN_HEIGHT - BOX_HEIGHT) * (gameData->distBeforeRoadChange - DIST_BEFORE_ROAD_RESET + i + 1);
				DrawRectangle(screen, (2 + i) * SCREEN_WIDTH / 20, BOX_HEIGHT, SCREEN_WIDTH / 20, SCREEN_HEIGHT - BOX_HEIGHT - diff[ROAD_START_SEQUENCE], green, green);
				DrawRectangle(screen, (17 - i) * SCREEN_WIDTH / 20, BOX_HEIGHT, SCREEN_WIDTH / 20, SCREEN_HEIGHT - BOX_HEIGHT - diff[ROAD_START_SEQUENCE], green, green);
			}
		}
	}
	else if (gameData->road == veryNarrow) {
		for (int i = 0; i < ROAD_SIDE_COUNT; i++)
		{
			if (gameData->distBeforeRoadChange < maxRoadLen - i && gameData->distBeforeRoadChange > maxRoadLen - DIST_BEFORE_ROAD_RESET + i)
			{
				if (gameData->distBeforeRoadChange > maxRoadLen - 1 - i)
					diff[ROAD_START_SEQUENCE] = (SCREEN_HEIGHT - BOX_HEIGHT) * (gameData->distBeforeRoadChange - maxRoadLen + i + 1);
				else if (gameData->distBeforeRoadChange < maxRoadLen - DIST_BEFORE_ROAD_RESET + i + 1)
					diff[ROAD_END_SEQUENCE] = (SCREEN_HEIGHT - BOX_HEIGHT) * (gameData->distBeforeRoadChange - maxRoadLen + DIST_BEFORE_ROAD_RESET - i - 1);
				DrawRectangle(screen, (4 + i) * SCREEN_WIDTH / 20, BOX_HEIGHT - diff[ROAD_END_SEQUENCE], SCREEN_WIDTH / 20, SCREEN_HEIGHT - BOX_HEIGHT - diff[ROAD_START_SEQUENCE] + diff[ROAD_END_SEQUENCE], green, green);
				DrawRectangle(screen, (15 - i) * SCREEN_WIDTH / 20, BOX_HEIGHT - diff[ROAD_END_SEQUENCE], SCREEN_WIDTH / 20, SCREEN_HEIGHT - BOX_HEIGHT - diff[ROAD_START_SEQUENCE] + diff[ROAD_END_SEQUENCE], green, green);
			}
			if (gameData->distBeforeRoadChange > i)
			{
				if (gameData->distBeforeRoadChange < i + 1)
					diff[i + ROAD_SIDE_COUNT] = (SCREEN_HEIGHT - BOX_HEIGHT) * (gameData->distBeforeRoadChange - i - 1);
				DrawRectangle(screen, (2 + i) * SCREEN_WIDTH / 20, BOX_HEIGHT - diff[i + ROAD_SIDE_COUNT], SCREEN_WIDTH / 20, SCREEN_HEIGHT - BOX_HEIGHT + diff[i + ROAD_SIDE_COUNT], green, green);
				DrawRectangle(screen, (17 - i) * SCREEN_WIDTH / 20, BOX_HEIGHT - diff[i + ROAD_SIDE_COUNT], SCREEN_WIDTH / 20, SCREEN_HEIGHT - BOX_HEIGHT + diff[i + ROAD_SIDE_COUNT], green, green);
			}
		}
	}
}


void WriteContents(SDL_Surface* screen, struct assets gameAssets, SDL_Texture* scrtex, SDL_Renderer* renderer, struct logic* gameData, struct traffic* cars)
{
	char text[128];

	// color map definitions
	const int green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	const int red = SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00);
	const int blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	const int black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	const int white = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);

	// fill screen with black color
	SDL_FillRect(screen, NULL, black);

	// draw grass borders
	DrawBorders(screen, gameData);

	// draw player's car or an explosion if player died
	if (gameData->deathTimer)
		DrawSurface(screen, gameAssets.explosion, gameData->xPosition, 2 * SCREEN_HEIGHT / 3);
	else
		DrawSurface(screen, gameAssets.player, gameData->xPosition, 2 * SCREEN_HEIGHT / 3);

	// draw traffic cars
	struct traffic* h = cars->next;
	while (h != NULL)
	{
		if (!h->speed)
			DrawSurface(screen, gameAssets.explosion, h->xPosition, h->yPosition);
		else if (h->type == civil)
			DrawSurface(screen, gameAssets.civil, h->xPosition, h->yPosition);
		else
			DrawSurface(screen, gameAssets.opponent, h->xPosition, h->yPosition);
		if (gameData->deathTimer)
			h->yPosition += (-h->speed) * SCREEN_HEIGHT * gameData->time.delta;
		else
			h->yPosition += (gameData->speed - h->speed) * SCREEN_HEIGHT * gameData->time.delta;
		h = h->next;
	}

	// generate random trees
	int random = RNG(RNG_LEFT_SIDE, RNG_MIDDLE_SIDE);
	if (!gameData->tree.left
		&& random == RNG_LEFT_SIDE
		&& (int)(gameData->distance * SCREEN_HEIGHT) % SCREEN_HEIGHT == 0)
		gameData->tree.left = true;
	if (!gameData->tree.right
		&& random == RNG_RIGHT_SIDE
		&& (int)(gameData->distance * SCREEN_HEIGHT) % SCREEN_HEIGHT == 0)
		gameData->tree.right = true;
	if (!gameData->tree.middle && gameData->road == middleGrass
		&& random == RNG_MIDDLE_SIDE && gameData->distBeforeRoadChange > 2
		&& gameData->distBeforeRoadChange < 8
		&& (int)(gameData->distance * SCREEN_HEIGHT) % SCREEN_HEIGHT == 0)
		gameData->tree.middle = true;

	// draw generated trees
	if (gameData->tree.left)
		DrawSurface(screen, gameAssets.tree, SCREEN_WIDTH / 20, (int)(gameData->distance * SCREEN_HEIGHT) % SCREEN_HEIGHT);
	if (gameData->tree.right)
		DrawSurface(screen, gameAssets.tree, SCREEN_WIDTH - SCREEN_WIDTH / 20, (int)(gameData->distance * SCREEN_HEIGHT) % SCREEN_HEIGHT);
	if (gameData->tree.middle)
		DrawSurface(screen, gameAssets.tree, SCREEN_WIDTH / 2, (int)(gameData->distance * SCREEN_HEIGHT) % SCREEN_HEIGHT);

	// draw generated power-up icon
	if (gameData->ammo.xPosition != NOT_PLACED)
		DrawSurface(screen, gameAssets.powerup, gameData->ammo.xPosition, gameData->ammo.yPosition = gameData->ammo.yPosition + gameData->speed * gameData->time.delta * 400);

	// draw bullets, either normal or secondary
	if (gameData->shot.xPosition && 2 * SCREEN_HEIGHT / 3 - CAR_HEIGHT / 2 - (gameData->time.t1 - gameData->shot.time) > 0)
	{
		DrawRectangle(screen, gameData->shot.xPosition - 3, (int)(2 * SCREEN_HEIGHT / 3 - CAR_HEIGHT / 2 - (gameData->time.t1 - gameData->shot.time)), BULLET_WIDTH, BULLET_HEIGHT, white, white);
		if (gameData->ammo.amount > NO_SECONDARY_AMMO)
		{
			DrawRectangle(screen, gameData->shot.xPosition - CAR_WIDTH / 2, (int)(2 * SCREEN_HEIGHT / 3 - CAR_HEIGHT / 2 - (gameData->time.t1 - gameData->shot.time)), BULLET_WIDTH, BULLET_HEIGHT, white, white);
			DrawRectangle(screen, gameData->shot.xPosition + CAR_WIDTH / 2 - BULLET_WIDTH, (int)(2 * SCREEN_HEIGHT / 3 - CAR_HEIGHT / 2 - (gameData->time.t1 - gameData->shot.time)), BULLET_WIDTH, BULLET_HEIGHT, white, white);
		}
	}

	// info text
	DrawRectangle(screen, 0, 0, SCREEN_WIDTH, BOX_HEIGHT, red, blue);
	DrawRectangle(screen, SCREEN_WIDTH - TEXT_WIDTH, SCREEN_HEIGHT - BOX_HEIGHT, TEXT_WIDTH, BOX_HEIGHT, red, blue);

	// top box, first row
	sprintf(text, "Jakub Wojtkowiak, 193546");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 6, text, gameAssets.charset);

	// top box, second row
	if (gameData->time.toEnd >= 0)
		if (gameData->ammo.amount > 0)
			sprintf(text, "czas - %d s | punkty: %d | naboi dodatkowych: %d", (int)ceil(gameData->time.toEnd), gameData->points, gameData->ammo.amount);
		else
			sprintf(text, "czas - %d s | punkty: %d", (int)ceil(gameData->time.toEnd), gameData->points);
	else
		if (gameData->ammo.amount > 0)
			sprintf(text, "pozostalo zyc: %d | punkty: %d | naboi dodatkowych: %d", gameData->lives, gameData->points, gameData->ammo.amount);
		else
			sprintf(text, "pozostalo zyc: %d | punkty: %d", gameData->lives, gameData->points);
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 22, text, gameAssets.charset);

	// bottom right box
	sprintf(text, "abcdefgghijkmn");
	DrawString(screen, screen->w - 60 - strlen(text) * 8 / 2, SCREEN_HEIGHT - 22, text, gameAssets.charset);

	SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
	//		SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, scrtex, NULL, NULL);
	SDL_RenderPresent(renderer);
}

// time delta calculator
void CalculateDelta(struct logic* gameData) {
	gameData->time.t2 = SDL_GetTicks();

	gameData->time.delta = (gameData->time.t2 - gameData->time.t1) * 0.001;
	gameData->time.t1 = gameData->time.t2;
}

// points, timer, penalty and
void CalculateTimeAndPoints(struct logic* gameData)
{
	CalculateDelta(gameData);
	gameData->time.toEnd -= gameData->time.delta;

	if (gameData->timeBeforeNewEnemy > 0)
	{
		gameData->timeBeforeNewEnemy -= gameData->time.delta;
		if (gameData->timeBeforeNewEnemy < 0) gameData->timeBeforeNewEnemy = 0;
	}

	if (gameData->civilKillTimer > 0)
	{
		gameData->civilKillTimer -= gameData->time.delta;
		if (gameData->civilKillTimer < 0) gameData->civilKillTimer = 0;
	}

	// calculate points if player is alive
	if (!gameData->deathTimer)
	{
		// check if off-road
		double accelPenalty = DEFAULT_SPEED;
		if (gameData->isOffroad || gameData->civilKillTimer)
		{
			if (gameData->isOffroad)
				accelPenalty /= ACCEL_PENALTY;
			gameData->penalty += gameData->speed * gameData->time.delta * accelPenalty;
		}
		gameData->distBeforeRoadChange -= gameData->speed * gameData->time.delta * accelPenalty;
		if (gameData->distBeforeRoadChange < 0) gameData->distBeforeRoadChange = 0;

		// road type change
		if (gameData->distBeforeRoadChange == 0)
		{
			gameData->distBeforeRoadChange = DIST_BEFORE_ROAD_RESET;
			if (gameData->road != middleGrass) {
				int roadInt = (int)(gameData->road) + 1;
				gameData->road = (enum roadWidth)roadInt;
				if (gameData->road == veryNarrow) gameData->distBeforeRoadChange += V_NARROW_ADD_TIME;
			}
			else
				gameData->road = wide;
		}

		// reset tree state after every 1 screen height
		if (((int)((gameData->distance + gameData->speed * gameData->time.delta) * SCREEN_HEIGHT) % SCREEN_HEIGHT) < ((int)(gameData->distance * SCREEN_HEIGHT) % SCREEN_HEIGHT))
		{
			gameData->tree.left = false;
			gameData->tree.right = false;
			gameData->tree.middle = false;
		}

		gameData->distance += gameData->speed * gameData->time.delta * accelPenalty;

		// calculate player's lives
		if (gameData->time.toEnd >= 0)
			gameData->lives = (int)(gameData->points / NEW_CAR_ADDITION);
		else
		{
			if (gameData->lives <= 0)
				gameData->gameOver = true;
			else {
				int pt1 = gameData->points / NEW_CAR_ADDITION;
				int pt2 = ((gameData->distance - gameData->penalty) * POINT_ADDITION + OPPONENT_KILL_PTS * gameData->opponentKills) / NEW_CAR_ADDITION;
				if (pt1 != pt2)
					gameData->lives++;
			}
		}

		gameData->points = (int)(gameData->distance - gameData->penalty) * POINT_ADDITION + OPPONENT_KILL_PTS * gameData->opponentKills;
	}
	else {
		// death time calculation
		gameData->deathTimer -= gameData->time.delta;
		if (gameData->deathTimer < 0) {
			gameData->deathTimer = 0;
			if (gameData->road == middleGrass)
			{
				// respawn on the side if grass is in the middle
				int randomSide = RNG(0, 1) * GRASS_MULTIPLIER;
				gameData->xPosition = (INITIAL_GRASS_SHIFT + randomSide) * SCREEN_WIDTH / 60;
			}
			else
				gameData->xPosition = SCREEN_WIDTH / 2;
		}
	}
}

// shooting start
void Shoot(struct logic* gameData)
{
	gameData->shot.xPosition = gameData->xPosition;
	gameData->shot.time = SDL_GetTicks();
	if (gameData->ammo.amount > NO_SECONDARY_AMMO) gameData->ammo.amount--;
}

void ShotDetection(struct traffic* cars, struct logic* gameData)
{
	if (gameData->ammo.amount > 0)
	{
		if (2 * SCREEN_HEIGHT / 3 - CAR_HEIGHT / 2 - (gameData->time.t1 - gameData->shot.time) <= 0)
		{
			gameData->shot.time = 0;
			gameData->shot.xPosition = 0;
		}
	}
	else if (2 * SCREEN_HEIGHT / 3 - CAR_HEIGHT / 2 - (gameData->time.t1 - gameData->shot.time) <= SCREEN_HEIGHT / 3)
	{
		gameData->shot.time = 0;
		gameData->shot.xPosition = 0;
	}
	struct traffic* h = cars;
	struct traffic* tmp;
	while (h->next != NULL) {
		tmp = h->next;
		if (2 * SCREEN_HEIGHT / 3 - CAR_HEIGHT / 2 - (gameData->time.t1 - gameData->shot.time) <= tmp->yPosition + CAR_HEIGHT / 2
			&& tmp->speed
			&& ((abs((int)tmp->xPosition - gameData->shot.xPosition) <= CAR_WIDTH / 2 + 3) || (gameData->ammo.amount > NO_SECONDARY_AMMO && abs((int)tmp->xPosition - gameData->shot.xPosition) <= CAR_WIDTH)))
		{
			tmp->killTimer = KILL_TIMER_VALUE;
			tmp->speed = 0;
			if (tmp->type == civil)
				gameData->civilKillTimer = CIVIL_KILL_TIMER;
			else if(!gameData->civilKillTimer)
				gameData->opponentKills++;
		}
		h = h->next;
	}
}

// check collision with grass
void CheckIfColliding(SDL_Surface* screen, struct logic* gameData)
{
	SDL_Color rgbLeft, rgbRight;
	Uint32 leftGrass = GetPixel(screen, gameData->xPosition - CAR_WIDTH / 2, 2 * SCREEN_HEIGHT / 3);
	Uint32 rightGrass = GetPixel(screen, gameData->xPosition + CAR_WIDTH / 2, 2 * SCREEN_HEIGHT / 3);
	SDL_GetRGB(leftGrass, screen->format, &rgbLeft.r, &rgbLeft.g, &rgbLeft.b);
	SDL_GetRGB(rightGrass, screen->format, &rgbRight.r, &rgbRight.g, &rgbRight.b);
	if (rgbLeft.g == FULL_COLOR || rgbRight.g == FULL_COLOR)
	{
		if (!gameData->deathTimer)
		{
			gameData->isOffroad = true;
			if (rgbLeft.g == FULL_COLOR && rgbRight.g == FULL_COLOR)
			{
				gameData->deathTimer = KILL_TIMER_VALUE;
				if (gameData->time.toEnd < 0) gameData->lives--;
			}

		}
	}
	else {
		gameData->isOffroad = false;
	}
}

// don't spawn a vehicle when there is one at the top
bool VerticalTrafficCheck(struct traffic* cars)
{
	struct traffic* h = cars->next;
	while (h != NULL)
	{
		if (abs(h->yPosition) <= CAR_HEIGHT)
			return false;
		h = h->next;
	}
	return true;
}

// check if power-up isn't outside the window or has been taken by the player
void CheckWeapon(struct logic* gameData)
{
	if (gameData->ammo.yPosition > SCREEN_HEIGHT)
	{
		gameData->ammo.xPosition = NOT_PLACED;
		gameData->ammo.yPosition = NOT_PLACED;
	}
	else if (abs(gameData->ammo.xPosition - gameData->xPosition) <= CAR_WIDTH && abs(gameData->ammo.yPosition - 2 * SCREEN_HEIGHT / 3) <= CAR_HEIGHT / 2)
	{
		gameData->ammo.xPosition = NOT_PLACED;
		gameData->ammo.yPosition = NOT_PLACED;
		gameData->ammo.amount = SECONDARY_AMMO;
	}
}

// add powerup and traffic cars
void GenerateTrafficAndWeapon(struct traffic* cars, struct logic* gameData)
{
	int random = RNG(0, 5);
	if (random == 0 && gameData->timeBeforeNewEnemy == 0 && gameData->ammo.amount == NO_SECONDARY_AMMO && gameData->ammo.xPosition == NOT_PLACED)
	{
		gameData->timeBeforeNewEnemy = 2;
		gameData->ammo.xPosition = HorPosition(gameData->road);
		gameData->ammo.yPosition = -CAR_HEIGHT / 4;
	}
	else if (random >= 2 && gameData->timeBeforeNewEnemy == 0 && VerticalTrafficCheck(cars))
	{
		gameData->timeBeforeNewEnemy = 2;
		struct traffic* x = new struct traffic;
		if (random >= 4) x->type = civil;
		else x->type = opponent;
		if (x->xPosition == NOT_PLACED)
			x->xPosition = HorPosition(gameData->road);
		x->next = NULL;
		struct traffic* h = cars;
		while (h->next != NULL)
		{
			h = h->next;
		}
		h->next = x;
	}
}

// move traffic cars if they don't fit on the road
void MoveTraffic(struct logic* gameData, struct traffic* cars)
{
	struct traffic* h = cars->next;
	while (h != NULL)
	{
		if (gameData->road == middleGrass)
		{
			if (h->xPosition >= SCREEN_WIDTH / 2 && h->xPosition <= 9 * SCREEN_WIDTH / 15 + CAR_WIDTH / 2 + 1)
				h->xPosition += (int)(gameData->time.delta * DELTA_MULTIPLIER);
			else if (h->xPosition >= 6 * SCREEN_WIDTH / 15 - CAR_WIDTH / 2 + 1 && h->xPosition < SCREEN_WIDTH / 2)
				h->xPosition -= (int)(gameData->time.delta * DELTA_MULTIPLIER);
		}
		else if (gameData->road == narrow)
		{
			if (h->xPosition <= 4 * SCREEN_WIDTH / 20 + CAR_WIDTH / 2 + 1)
				h->xPosition += (int)(gameData->time.delta * DELTA_MULTIPLIER);
			else if (h->xPosition >= 16 * SCREEN_WIDTH / 20 - CAR_WIDTH / 2 + 1)
				h->xPosition -= (int)(gameData->time.delta * DELTA_MULTIPLIER);
		}
		else if (gameData->road == veryNarrow)
		{
			if (h->xPosition <= 6 * SCREEN_WIDTH / 20 + CAR_WIDTH / 2 + 1)
				h->xPosition += (int)(gameData->time.delta * DELTA_MULTIPLIER);
			else if (h->xPosition >= 14 * SCREEN_WIDTH / 20 - CAR_WIDTH / 2 + 1)
				h->xPosition -= (int)(gameData->time.delta * DELTA_MULTIPLIER);
		}
		h = h->next;
	}
}

// detect player's car collision with traffic cars
void DetectTraffic(struct traffic* cars, struct logic* gameData)
{
	struct traffic* h = cars;
	struct traffic* tmp;
	while (h->next != NULL)
	{
		tmp = h->next;

		// delete killed vehicle
		if (tmp->killTimer <= 0 && !tmp->speed)
		{
			h->next = tmp->next;
			delete tmp;
		}
		else {
			// measure time of the killed car
			if (!tmp->speed)
			{
				tmp->killTimer -= gameData->time.delta;
				h = tmp;
			}
			else {
				// move attacking vehicle
				if (tmp->attackMode)
				{
					if (tmp->yPosition <= 2 * SCREEN_HEIGHT / 3)
					{
						tmp->speed = TRAFFIC_SPEED;
					}
					if (tmp->speed == ATTACK_SPEED)
					{
						if (tmp->targetX > tmp->xPosition)
						{
							tmp->xPosition += (ATTACK_SPEED - DEFAULT_SPEED) * (double)(tmp->targetX - tmp->startX) * gameData->time.delta;
						}
						else if (tmp->targetX < tmp->xPosition) {
							tmp->xPosition -= (ATTACK_SPEED - DEFAULT_SPEED) * (double)(tmp->startX - tmp->targetX) * gameData->time.delta;
						}
					}
				}
				// check if vehicle is out of range
				if (tmp->yPosition >= 3 * SCREEN_HEIGHT / 2 && tmp->speed)
				{
					// turn on attack mode for opponent for once or kill the civilian/opponent
					if (tmp->type == opponent && tmp->attackMode == false)
					{
						tmp->attackMode = true;
						tmp->speed = ATTACK_SPEED;
						tmp->startX = tmp->xPosition;
						tmp->targetX = gameData->xPosition;
						h = tmp;
					}
					else {
						h->next = tmp->next;
						delete tmp;
					}
				}
				// check collision and speeds between player and traffic car:
				// kill player if slower than traffic OR a traffic car if player is faster
				else if (abs((int)(gameData->xPosition - tmp->xPosition)) < CAR_WIDTH && abs((int)(2 * SCREEN_HEIGHT / 3 - tmp->yPosition)) < CAR_HEIGHT)
				{
					if (tmp->speed > gameData->speed && !gameData->deathTimer)
					{
						gameData->deathTimer = KILL_TIMER_VALUE;
						if (gameData->time.toEnd < 0) gameData->lives--;
						h = tmp;
					}
					else
					{
						if (!gameData->deathTimer)
						{
							if (tmp->type == civil) gameData->civilKillTimer = CIVIL_KILL_TIMER;
							else if (tmp->speed && !gameData->civilKillTimer) gameData->opponentKills++;
							tmp->killTimer = KILL_TIMER_VALUE;
							tmp->speed = 0;
						}
						h = tmp;
					}
				}
				else
					h = tmp;
			}
		}
	}
}

// car list deletion
void DeleteAllCars(struct traffic* cars)
{
	struct traffic* h = cars;
	struct traffic* tmp;
	while (h->next != NULL)
	{
		tmp = h->next;
		h->next = tmp->next;
		delete tmp;
	}

}

// game reset when hitting new game button
void ResetGame(struct logic* gameData, struct traffic* cars)
{
	gameData->time.toEnd = START_TIME;
	gameData->time.t1 = SDL_GetTicks();
	gameData->xPosition = SCREEN_WIDTH / 2;
	gameData->speed = DEFAULT_SPEED;
	gameData->points = 0;
	gameData->lives = 0;
	gameData->deathTimer = 0;
	gameData->civilKillTimer = 0;
	gameData->timeBeforeNewEnemy = 0;
	gameData->distance = 0;
	gameData->penalty = 0;
	gameData->isOffroad = false;
	gameData->opponentKills = 0;
	gameData->road = DEFAULT_ROAD;
	gameData->distBeforeRoadChange = DIST_BEFORE_ROAD_RESET;
	gameData->tree.left = false;
	gameData->tree.right = false;
	gameData->tree.middle = false;
	gameData->shot.time = 0;
	gameData->shot.xPosition = 0;
	gameData->ammo.xPosition = NOT_PLACED;
	gameData->ammo.yPosition = NOT_PLACED;
	gameData->ammo.amount = NO_SECONDARY_AMMO;
	DeleteAllCars(cars);
}

// exiting the game program
void QuitGame(SDL_Surface* charset, SDL_Surface* screen, SDL_Texture* scrtex, SDL_Window* window, SDL_Renderer* renderer, struct traffic* cars)
{
	// freeing all surfaces
	DeleteAllCars(cars);
	FreeSurfaces(charset, screen, scrtex, window, renderer);
	SDL_Quit();
}

// game pause after hitting "p" button
void PauseGame(SDL_Surface* screen, SDL_Surface* charset, SDL_Texture* scrtex, SDL_Renderer* renderer, SDL_Event* event, SDL_Window* window, int* t1, int* quit, struct traffic* cars)
{
	char text[128];
	int green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	int black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	bool pauseState = true;

	while (pauseState)
	{
		SDL_FillRect(screen, NULL, black);
		DrawRectangle(screen, 0, SCREEN_HEIGHT / 2 - 18, SCREEN_WIDTH, BOX_HEIGHT, green, blue);
		sprintf(text, "Gra przerwana - nacisnij przycisk 'p', aby grac dalej.");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, screen->h / 2 - 4, text, charset);
		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);
		while (SDL_PollEvent(event)) {
			switch (event->type) {
			case SDL_KEYDOWN:
				if (event->key.keysym.sym == SDLK_p)
				{
					*t1 = SDL_GetTicks();
					pauseState = false;
				}
				break;
			case SDL_QUIT:	//e.g. Alt+F4
			{
				pauseState = false;
				*quit = 1;
				QuitGame(charset, screen, scrtex, window, renderer, cars);
			}
			break;
			};
		};
	}
}

// saving the game progress to .sav file
void SaveGame(struct logic* gameData, struct traffic* cars)
{
	FILE* file;
	time_t timeNow;
	struct tm* timeInfo;
	char timeString[32];

	time(&timeNow);
	timeInfo = localtime(&timeNow);
	strftime(timeString, 32, "Saves/%Y-%m-%d_%H-%M-%S.sav", timeInfo);

	file = fopen(timeString, "wb");
	fwrite(gameData, sizeof(struct logic), 1, file);

	struct traffic* h = cars;
	while (h->next != NULL)
	{
		fwrite(h->next, sizeof(struct traffic), 1, file);
		h = h->next;
	}
	fclose(file);
}

// choosing file to open using Win32 API
bool ReadSave(char* fileName)
{
	OPENFILENAMEA open;
	memset(&open, 0, sizeof(open));
	open.lStructSize = sizeof(open);
	open.lpstrFilter = "Plik zapisu (*.sav)\0*.sav\0\0";
	open.lpstrFile = fileName;
	open.nMaxFile = MAX_PATH;
	open.lpstrTitle = "Wybierz plik zapisu";
	open.lpstrInitialDir = "Saves";
	open.Flags = OFN_FILEMUSTEXIST |
		OFN_HIDEREADONLY |
		OFN_NOCHANGEDIR;

	if (!GetOpenFileNameA(&open))
		return false;
	return true;
}

// loading save state from file
void LoadSave(SDL_Surface* screen, SDL_Surface* charset, SDL_Texture* scrtex, SDL_Renderer* renderer, SDL_Event* event, SDL_Window* window, struct logic* gameData, struct traffic* cars)
{
	char fileName[MAX_PATH] = { '\0' };
	if (ReadSave(fileName))
	{
		for (int i = 0; i < MAX_PATH; i++)
			if (fileName[i] == '\\') fileName[i] = '/';

		FILE* file;
		file = fopen(fileName, "rb");

		fread(gameData, sizeof(logic), 1, file);

		struct traffic* h = cars;
		struct traffic* tmp;
		bool good = true;
		while (good)
		{
			tmp = new struct traffic;
			if (fread(tmp, sizeof(traffic), 1, file))
				h->next = tmp;
			else
			{
				delete tmp;
				good = false;
			}
			h = h->next;
		}
		fclose(file);
	}
	gameData->time.t1 = SDL_GetTicks();
}

// last message before exiting the game
void GameOver(SDL_Surface* screen, SDL_Surface* charset, SDL_Texture* scrtex, SDL_Renderer* renderer, SDL_Event* event, SDL_Window* window, struct logic* gameData, int* quit, struct traffic* carsList)
{
	char text[128];
	int green = SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00);
	int blue = SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC);
	int black = SDL_MapRGB(screen->format, 0x00, 0x00, 0x00);
	bool pauseState = true;

	while (pauseState)
	{
		SDL_FillRect(screen, NULL, black);
		DrawRectangle(screen, 0, SCREEN_HEIGHT / 2 - 18, SCREEN_WIDTH, BOX_HEIGHT, green, blue);
		sprintf(text, "Koniec gry - nie masz wiecej aut. Zdobyles %d pkt. w %d s. Aby zagrac ponownie, zrestartuj gre.", gameData->points, (int)floor(START_TIME - gameData->time.toEnd));
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, screen->h / 2 - 4, text, charset);
		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);
		while (SDL_PollEvent(event)) {
			switch (event->type) {
			case SDL_KEYDOWN:
			case SDL_QUIT:	// e.g. Alt+F4
			{
				pauseState = false;
				*quit = 1;
				QuitGame(charset, screen, scrtex, window, renderer, carsList);
			}
			break;
			};
		};
	}

}

// main function controller
void MainController(SDL_Surface* screen, struct assets gameAssets, SDL_Texture* scrtex, SDL_Renderer* renderer, struct logic* gameData, struct traffic* carsList, SDL_Event* event, SDL_Window* window, int* quit)
{
	if (!gameData->gameOver)
	{
		GenerateTrafficAndWeapon(carsList, gameData);
		WriteContents(screen, gameAssets, scrtex, renderer, gameData, carsList);
		CalculateTimeAndPoints(gameData);
		CheckWeapon(gameData);
		CheckIfColliding(screen, gameData);
		MoveTraffic(gameData, carsList);
		DetectTraffic(carsList, gameData);
		ShotDetection(carsList, gameData);
	}
	else {
		GameOver(screen, gameAssets.charset, scrtex, renderer, event, window, gameData, quit, carsList);
	}
}

// car movement function
void MoveCar(SDL_Event* event, struct assets gameAssets, SDL_Surface* screen, SDL_Texture* scrtex, SDL_Renderer* renderer, struct logic* gameData, struct traffic* cars, SDL_Window* window, int* quit, bool toTheLeft)
{
	bool state = true;
	while (state)
	{
		MainController(screen, gameAssets, scrtex, renderer, gameData, cars, event, window, quit);
		if (!gameData->deathTimer)
		{
			if (toTheLeft)
				gameData->xPosition -= DELTA_MULTIPLIER * gameData->time.delta / 2;
			else
				gameData->xPosition += DELTA_MULTIPLIER * gameData->time.delta / 2;
		}
		while (SDL_PollEvent(event)) {
			switch (event->type) {
			case SDL_KEYDOWN:
				if (event->key.keysym.sym == SDLK_UP) gameData->speed = ACCEL_SPEED;
				else if (event->key.keysym.sym == SDLK_DOWN) gameData->speed = DECEL_SPEED;
				break;
			case SDL_KEYUP:
				gameData->speed = DEFAULT_SPEED;
				state = false;
				break;
			};
		};
	}
}

#ifdef __cplusplus
extern "C"
#endif

int main(int argc, char** argv) {
	int quit = 0;
	struct logic gameData;
	struct traffic* carsList = new struct traffic;
	struct assets gameAssets;
	SDL_Event event;
	SDL_Surface* screen;
	SDL_Texture* scrtex;
	SDL_Window* window;
	SDL_Renderer* renderer;
	bool isWindowed = true;

	GameInit(&window, &renderer);

	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32,
		0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		SCREEN_WIDTH, SCREEN_HEIGHT);

	CrashWhenNotFound(gameAssets.charset, screen, scrtex, window, renderer);
	SDL_SetColorKey(gameAssets.charset, true, 0x000000);

	CrashWhenNotFound(gameAssets.player, screen, scrtex, window, renderer, gameAssets.charset);

	while (!quit) {
		MainController(screen, gameAssets, scrtex, renderer, &gameData, carsList, &event, window, &quit);

		// obs³uga zdarzeñ (o ile jakieœ zasz³y) / handling of events (if there were any)
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) quit = 1;
				else if (event.key.keysym.sym == SDLK_UP) gameData.speed = ACCEL_SPEED;
				else if (event.key.keysym.sym == SDLK_DOWN) gameData.speed = DECEL_SPEED;
				else if (event.key.keysym.sym == SDLK_LEFT) MoveCar(&event, gameAssets, screen, scrtex, renderer, &gameData, carsList, window, &quit, DIRECTION_LEFT);
				else if (event.key.keysym.sym == SDLK_RIGHT) MoveCar(&event, gameAssets, screen, scrtex, renderer, &gameData, carsList, window, &quit, DIRECTION_RIGHT);
				else if (event.key.keysym.sym == SDLK_n) ResetGame(&gameData, carsList);
				else if (event.key.keysym.sym == SDLK_p) PauseGame(screen, gameAssets.charset, scrtex, renderer, &event, window, &gameData.time.t1, &quit, carsList);
				else if (event.key.keysym.sym == SDLK_s) SaveGame(&gameData, carsList);
				else if (event.key.keysym.sym == SDLK_l) LoadSave(screen, gameAssets.charset, scrtex, renderer, &event, window, &gameData, carsList);
				else if (event.key.keysym.sym == SDLK_f) gameData.gameOver = true;
				else if (event.key.keysym.sym == SDLK_SPACE) Shoot(&gameData);
				else if (event.key.keysym.sym == SDLK_F11) FullScreen(window, &isWindowed);
				break;
			case SDL_KEYUP:
				gameData.speed = DEFAULT_SPEED;
				break;
			case SDL_QUIT:
				quit = 1;
				break;
			};
		};
	};
	return 0;
};
