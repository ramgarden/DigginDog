//Josh Pritt (ramgarden)
//Brenna Kaminski (snackthunder)
//May 7th, 2021
//Diggin' Dog
//Kinda like mario bros but is a dog digging up
//bones.

#include <Arduboy2.h>
#include <ArduboyTones.h>
#include "bitmaps.h"
#include "levels.h"

#define ARRAY_LEN(a) (sizeof(a) / sizeof((a)[0]))

//game states
#define LOGO 0
#define INTRO 1
#define TITLE 2
#define GAME 3
#define GAMEOVER 4
#define WIN 5

//player states
enum class Player : uint8_t {
      Standing = 0,
      Walking = 1,
      Digging = 2,
      Barking = 3,
      Jumping = 4
    };

struct Wave {
  uint8_t x;
  uint8_t y;
  bool facingLeft;
};

Wave waves[4];

//player facing
#define LEFT   0
#define RIGHT  1

//tile types from bitmaps.h tiles array
#define GRASS      0
#define WATER      1
#define TREES      2
#define STONE      3
#define GNDTOP1    4
#define GNDTOP2    5
#define GNDBTM1    6
#define GNDBTM2    7
#define SKY1       8
#define SKY2       9
#define SKY3      10
#define TILE_SIZE  16

Arduboy2 arduboy;
ArduboyTones sound(arduboy.audio.enabled);
#define ARDBITMAP_SBUF arduboy.getBuffer()
#include <ArdBitmap.h>
ArdBitmap<WIDTH, HEIGHT> ardbitmap;

//config type variables
const int fieldWidth = 5;
const int fieldHeight = 5;
const int logoTime = 4000; //how long to show the logo
const int GROUND_LEVEL = 19; //Y coord to put dog on the ground
const int BARK_TIME = 500; //time in ms to show bark sprite
const int WAVE_SPEED = 2; //pixels per frame

//global variables
int playerx = 0;
int playery = 0;
int playerDrawX;
int wavex = 0;
int wavey = 0;
int score = 0;
unsigned long initTime;
unsigned long myTime;
unsigned long lastBarkTime;
int gamestate = LOGO;
Player playerState;
int playerFacing = RIGHT;
bool facingLeft = false; // true means pyoro is facing left, false means he is facing right
//bark wave facing when launched
//TODO: make this an array of bools to handle multiple waves on the screen at once.
bool waveFacingLeft = false;
bool waveOnScreen = false;


void setup() {
  // this code is run once when power is turned on
  arduboy.begin();
  arduboy.clear();
  arduboy.setFrameRate(60);
  initTime = millis();
  playery = GROUND_LEVEL;
  wavex = -200;
  playerState = Player::Standing;
  initWaves();
  arduboy.display();
}

void loop() {
  // main loop of the whole game
  if (!(arduboy.nextFrame()))
    return;

  arduboy.clear();
  arduboy.pollButtons();
  myTime = (millis()-initTime);
//  resize1 = abs(sin((millis()-initTime)/ (1000.0/(PI))));
//  resize2 = abs(sin((millis()-initTime - 3000)/ (4000.0/(PI))));
//  resize3 = abs(sin((millis()-initTime - 7000)/ (8000.0/(PI))));

  

  switch (gamestate) {

    case LOGO:
      //show team/company logo
      showLogo();
      if (arduboy.justPressed(A_BUTTON) || myTime % logoTime > (logoTime - 200)) {
        initTime = millis();
        gamestate++;
      }
      break;
    case INTRO:
      //intro movie
      doIntro();
      if (arduboy.justPressed(A_BUTTON) || myTime % logoTime > (logoTime - 200)) {
        initTime = millis();
        gamestate++;
      }
      break;
    case TITLE:
      //Title screen
      doTitleScreen();
      if (arduboy.justPressed(A_BUTTON)) {
        initTime = millis();
        lastBarkTime = initTime;
        gamestate++;
      }
      break;  
    case GAME:
      //Gameplay screen
      drawGameSideView();
      break;  
    case WIN:
      //Win screen
      arduboy.print("You Win! \nPress A to restart.");
      if (arduboy.justPressed(A_BUTTON)) {
        gamestate = LOGO;
        return;
      }
      break;  
    case GAMEOVER:
      //Game over screen
      arduboy.print("GAME OVER! \nPress A to restart.");
      if (arduboy.justPressed(A_BUTTON)) {
        gamestate = LOGO;
        return;
      }
      break;
  }

  if (arduboy.pressed(A_BUTTON) && arduboy.pressed(B_BUTTON) &&
           arduboy.pressed(UP_BUTTON)) {
    //reset game with UP + A + B;
    initTime = millis();
    gamestate = LOGO;
  }

  if (gamestate == GAME) {
    if (arduboy.pressed(LEFT_BUTTON)) {
      playerx = playerx - 1;
      playerState = Player::Walking;
      playerFacing = 0;
      facingLeft = true;
    }
    else if (arduboy.pressed(RIGHT_BUTTON)) {
      playerx = playerx + 1;
      playerState = Player::Walking;
      playerFacing = 1;
      facingLeft = false;
    }
    else if (arduboy.pressed(UP_BUTTON)) {
      playery = playery - 1;
      //Pause?
    }
    else if (arduboy.pressed(DOWN_BUTTON)) {
      playery = playery + 1;
      //DIG!
    }
    else
    {
      //no directions pressed, standing still
      playerState = Player::Standing;
    }
    
    if (arduboy.justPressed(A_BUTTON)) {
      //Bark!
      
      if (myTime > BARK_TIME * 4) {
        lastBarkTime = myTime;  
        playerState = Player::Barking;
        createNewWave(playerDrawX, playery, facingLeft);
        sound.tone(NOTE_C4H,30, NOTE_G4,30, NOTE_C5,30);
      }      
    }
    else if (arduboy.justPressed(B_BUTTON)) {
      //Jump!
    }
    
    
  }

  //DEBUG INFO
//  arduboy.print(playerx);
//  arduboy.print(",");
//  arduboy.print(playery);

//  arduboy.print("myTime: ");
//  arduboy.print(myTime);
//  arduboy.print("\nlastBarkTime: ");
//  arduboy.print(lastBarkTime);
  
  arduboy.display();
}

void setContrast(uint8_t contrast){

  arduboy.LCDCommandMode();
  arduboy.SPItransfer(0xd9);
  arduboy.SPItransfer(0x2f);
  arduboy.SPItransfer(0xdb);
  arduboy.SPItransfer(0x00);
    
  arduboy.SPItransfer(0x81); // contrast command
  arduboy.SPItransfer(contrast);
  arduboy.LCDDataMode();
}

////////////////////////
//intro animation
////////////////////////
void doIntro()
{
  ardbitmap.drawCompressedResized(WIDTH/2, HEIGHT, DOG[ (myTime/80)% ARRAY_LEN(DOG)], WHITE, ALIGN_H_CENTER | ALIGN_V_BOTTOM, MIRROR_NONE, 0.9 );
}

void showLogo()
{
  Sprites::drawOverwrite(0, 0, RAMGardenLogo, 0);
}

void doTitleScreen()
{  
  Sprites::drawOverwrite(0, 0, TitleScreen, 0);
}

void drawGameSideView() {  
  drawWorld();
  drawPlayer();
  drawBarkWave();
}


void drawWorld() {
  
  for (int y = 0; y < WORLD_HEIGHT; y++) {
    for (int x = 0; x < WORLD_WIDTH; x++) {
      Sprites::drawOverwrite(x * TILE_SIZE - playerx, y * TILE_SIZE, tiles, world1[y][x]);      
    }
  }
}


void drawPlayer() {

  //if we barked we need to keep it in that state for BARK_TIME.
  if (myTime - lastBarkTime < BARK_TIME && myTime > BARK_TIME) {
    playerState = Player::Barking;
  }

  //limit player X to the middle tiles
  if (playerx > TILE_WIDTH * 4 || playerx < TILE_WIDTH * 4) {
    playerDrawX = TILE_WIDTH * 4;
  }
  
  switch (playerState) {
    case Player::Standing: {
      ardbitmap.drawBitmap(playerDrawX, playery, DogStanding2, 16, 16, BLACK, ALIGN_NONE, facingLeft?MIRROR_HORIZONTAL:MIRROR_NONE);      
      break;
    }
    case Player::Walking: {
      ardbitmap.drawBitmap(playerDrawX, playery, DogStanding2, 16, 16, BLACK, ALIGN_NONE, facingLeft?MIRROR_HORIZONTAL:MIRROR_NONE);
      break;      
    }
    case Player::Barking: {
      ardbitmap.drawBitmap(playerDrawX, playery, DogBarking2, 16, 16, BLACK, ALIGN_NONE, facingLeft?MIRROR_HORIZONTAL:MIRROR_NONE);
      break;
    }
  }  
}

//draws the wave projectile for each bark
void drawBarkWave() {
  Wave thisWave;
  for (int i = 0; i < 4; i++) {
    thisWave = waves[i];
    //for each wave that is active draw it.
    if (thisWave.x != 255) {
      ardbitmap.drawBitmap(thisWave.x, thisWave.y, BarkWave2, 16, 16, BLACK, ALIGN_NONE, thisWave.facingLeft?MIRROR_HORIZONTAL:MIRROR_NONE);
      if(thisWave.facingLeft) {
        thisWave.x = thisWave.x - WAVE_SPEED;
      }
      else {
        thisWave.x = thisWave.x + WAVE_SPEED;
      }

      //reset/clear the wave once it's off the screen
      if (thisWave.x >= WIDTH || thisWave.x <= -WIDTH) {
        thisWave.x = 255;
      }
      
      waves[i] = thisWave;
    }

//    arduboy.print(i);
//    arduboy.print(":");
//    arduboy.print(thisWave.x);
//    arduboy.print(",");
  }
}

void createNewWave(int x, int y, bool facingLeft)
{
  Wave newWave;
  for (int i = 0; i < 4; i++) {
    newWave = waves[i];
    if (newWave.x == 255) {
      newWave.x = x;
      newWave.y = y;
      newWave.facingLeft = facingLeft;
      waves[i] = newWave;
      break;
    }
  }
}

void initWaves() {
  for (int i = 0; i < 4; i++) {
    waves[i].x = 255;
  }
}

void drawGameTopDown()
{
  ///////////////////////////
  //draw the background tiles
  ///////////////////////////
  //For each column on the screen
  for (int backgroundx = 0; backgroundx < 128; backgroundx = backgroundx + 8) {
  //For each row in the column
    for ( int backgroundy = 0; backgroundy < 64; backgroundy = backgroundy + 8) {
      //Draw a background tile
      Sprites::drawOverwrite(backgroundx, backgroundy, DirtTile, 0);
    }
  }

  /////////////////////
  //Draw player sprite
  /////////////////////
  Sprites::drawOverwrite(playerx, playery, DogStanding2, 0);
  arduboy.display();
}
