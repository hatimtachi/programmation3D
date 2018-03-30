/*!\file window.c
 *
 * \author Far�s BELHADJ, hatim tachi
 * \date March 07 2017
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <GL4D/gl4du.h>
#include <GL4D/gl4dg.h>
#include <GL4D/gl4duw_SDL2.h>
#include <SDL_image.h>


/* fonctions externes dans noise.c */
extern void initNoiseTextures(void);
extern void useNoiseTextures(GLuint pid, int shift);
extern void unuseNoiseTextures(int shift);
extern void freeNoiseTextures(void);


static void quit(void);
static void init(void);
static void resize(int w, int h);
static void idle(void);
static void keydown(int keycode);
static void keyup(int keycode);
static void draw(void);
static GLfloat heightMapAltitude(GLfloat x, GLfloat z);

/*!\brief largeur de la fen�tre */
static int _windowWidth = 1000;
/*!\brief haiteur de la fen�tre */
static int _windowHeight =800;
/*!\brief largeur de la heightMap g�n�r�e */
static int _landscape_w = 1000;
/*!\brief heuteur de la heightMap g�n�r�e */
static int _landscape_h = 1000;
/*!\brief scale en x et z du mod�le de terrrain */
static GLfloat _landscape_scale_xz =250.0f;
/*!\brief scale en y du mod�le de terrain */
static GLfloat _landscape_scale_y = 10.0f;
/*!\brief heightMap du terrain g�n�r� */
static GLfloat * _heightMap = NULL;
/*!\brief identifiant d'une sph�re (soleil) */
static GLuint _sphere = 0;
/*!\brief identifiant du terrain g�n�r� */
static GLuint _landscape = 0;
/*!\brief identifiant GLSL program du terrain */
static GLuint _landscape_pId  = 0;
/*!\brief identifiant GLSL program du soleil */
static GLuint _sun_pId = 0;
/*!\brief identifiant de la texture de d�grad� de couleurs du terrain */
static GLuint _terrain_tId = 0;
static GLuint _terrain_tId2 = 0;
static GLuint _terrain_tId3 = 0;
static GLuint _terrain_tId4 = 0;
/*!\brief identifiant de la texture de d�grad� de couleurs du soleil */
static GLuint _yellowred_tId = 0;
/*!\brief identifiant de la texture de plasma */
static GLuint _plasma_tId = 0;
/* variable de cycle pour le plasma */
static GLfloat _cycle = 0.0;

static GLuint _smTex=0;
static GLuint _plan = 0;
static GLuint _landscape2=0;
static GLuint _landscape_pId2  = 0;
static GLuint _landscape3=0;
static GLuint _landscape_pId3  = 0;
static GLuint _pId = 0;
static GLuint _landspace_Id=0;
static GLboolean _fog = GL_FALSE;

static char moduleViewTree[]="module/Pine/Tree2.obj";
static char moduleViewStone[]="module/stones/obj/stone_3.obj";
static char moduleViewGrass1[]="module/Ferns_OBJ/Ferns.obj";
static char moduleViewGrass[]="module/Grass pack/Grass_02.obj";
#define MAX_SHADOW 1024

static int STONE=1,TREE=2,GRASS=3;
/*!\brief indices des touches de clavier */
enum kyes_t {
  KLEFT = 0,
  KRIGHT,
  KUP,
  KDOWN
};

/*!\brief clavier virtuel */
static GLuint _keys[] = {0, 0, 0, 0};

typedef struct cam_t cam_t;
/*!\brief structure de donn�es pour la cam�ra */
struct cam_t {
  GLfloat x, z;
  GLfloat theta;
};

/*!\brief la cam�ra */
static cam_t _cam = {0, 0, 0};

/*!\brief cr�ation de la fen�tre, param�trage et initialisation,
 * lancement de la boucle principale */
int main(int argc, char ** argv) {
  if(!gl4duwCreateWindow(argc, argv, "Landscape", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                         _windowWidth, _windowHeight, SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN))
    return 1;
  init();
  atexit(quit);
  gl4duwResizeFunc(resize);
  gl4duwKeyUpFunc(keyup);
  gl4duwKeyDownFunc(keydown);
  gl4duwDisplayFunc(draw);
  gl4duwIdleFunc(idle);
  gl4duwMainLoop();
  return 0;
}

/*!\brief param�trage OpenGL et initialisation des donn�es */
static void init(void) {
  SDL_Surface * t;
  /* pour g�n�rer une chaine al�atoire diff�rente par ex�cution */
  //srand(time(NULL));
  /* param�tres GL */
  glClearColor(0.0f, 0.4f, 0.5f, 0.0f);
  glEnable(GL_DEPTH_TEST);
  //glDisable(GL_BACK);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  /* chargement et compilation des shaders */
  _pId             = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basictexture.fs", NULL);
  _landscape_pId   = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  _landspace_Id    = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic_eau.fs", NULL);
  _landscape_pId2  = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  _landscape_pId3  = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/basic.fs", NULL);
  _sun_pId         = gl4duCreateProgram("<vs>shaders/basic.vs", "<fs>shaders/sun.fs", NULL);
  
  /* cr�ation des matrices de model-view et projection */
  gl4duGenMatrix(GL_FLOAT, "modelViewMatrix");
  gl4duGenMatrix(GL_FLOAT, "projectionMatrix");
  gl4duGenMatrix(GL_FLOAT, "lightprojectionMatrix");
  gl4duGenMatrix(GL_FLOAT, "lightViewMatrix");

  /* appel forc� � resize pour initialiser le viewport et les matrices */
  resize(_windowWidth, _windowHeight);
  /* cr�ation de la g�om�trie de la sph�re */
  _plan = gl4dgGenQuadf();
  _sphere = gl4dgGenSpheref(20, 20);
  /* g�n�ration de la heightMap */
  _heightMap = gl4dmTriangleEdge(_landscape_w, _landscape_h, 0.5);
  /* cr�ation de la g�om�trie du terrain en fonction de la heightMap */
  _landscape = gl4dgGenGrid2dFromHeightMapf(_landscape_w, _landscape_h, _heightMap);
  _landscape2 =gl4dgGenGrid2dFromHeightMapf(400, 400, gl4dmTriangleEdge(400, 400, 0.5));
  _landscape3 =gl4dgGenGrid2dFromHeightMapf(1000, 1000, gl4dmTriangleEdge(1000, 1000, 0.5));



  glViewport(0,0,_landscape_w,_landscape_h);
  gl4duBindMatrix("lightprojectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _windowHeight / _windowWidth, 0.5 * _windowHeight / _windowWidth, 1.0, 1000.0);
  gl4duBindMatrix("modelViewMatrix");

  /* cr�ation, param�trage, chargement et transfert de la texture
     contenant le d�grad� de couleurs selon l'altitude (texture 1D) */
  glGenTextures(1, &_terrain_tId);
  glBindTexture(GL_TEXTURE_1D, _terrain_tId);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  t = IMG_Load("alt.png");
  assert(t);
#ifdef __APPLE__
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_BGR : GL_BGRA, GL_UNSIGNED_BYTE, t->pixels);
#else
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
#endif
  SDL_FreeSurface(t);

  glGenTextures(1, &_terrain_tId2);
  glBindTexture(GL_TEXTURE_1D, _terrain_tId2);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  t = IMG_Load("alt2.png");
  assert(t);
#ifdef __APPLE__
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_BGR : GL_BGRA, GL_UNSIGNED_BYTE, t->pixels);
#else
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
#endif
  SDL_FreeSurface(t);


glGenTextures(1, &_terrain_tId3);
  glBindTexture(GL_TEXTURE_1D, _terrain_tId3);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  t = IMG_Load("alt2.png");
  assert(t);
#ifdef __APPLE__
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_BGR : GL_BGRA, GL_UNSIGNED_BYTE, t->pixels);
#else
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
#endif
  SDL_FreeSurface(t);


glGenTextures(1, &_terrain_tId4);
  glBindTexture(GL_TEXTURE_1D, _terrain_tId4);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  t = IMG_Load("alt3.png");
  assert(t);
#ifdef __APPLE__
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_BGR : GL_BGRA, GL_UNSIGNED_BYTE, t->pixels);
#else
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
#endif
  SDL_FreeSurface(t);

  /* cr�ation, param�trage, chargement et transfert de la texture
     contenant le d�grad� de couleurs pour colorier le plasma
     (heightMap fractale) - texture 1D */
  glGenTextures(1, &_yellowred_tId);
  glBindTexture(GL_TEXTURE_1D, _yellowred_tId);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  t = IMG_Load("lave.png");
  assert(t);
#ifdef __APPLE__
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_BGR : GL_BGRA, GL_UNSIGNED_BYTE, t->pixels);
#else
  glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, t->w, 0, t->format->BytesPerPixel == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, t->pixels);
#endif
  SDL_FreeSurface(t);
  /* cr�ation, param�trage et transfert de la texture contenant la
     heightMap comme texture */
  glGenTextures(1, &_plasma_tId);
  glBindTexture(GL_TEXTURE_2D, _plasma_tId);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, _landscape_w, _landscape_h, 0, GL_RED, GL_FLOAT, _heightMap);
  glBindTexture(GL_TEXTURE_2D, 0);


  glGenTextures(1, &_smTex);
  glBindTexture(GL_TEXTURE_2D, _smTex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, MAX_SHADOW, MAX_SHADOW, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
  assimpInit(moduleViewStone,1);
  assimpInit(moduleViewGrass1,2);
  assimpInit(moduleViewGrass,3);
  assimpInit_Grass(moduleViewTree);
  initObjet();
  initStone();
  initNoiseTextures();
  initGrass();
}

/*!\brief param�trage du viewport OpenGL et de la matrice de
 * projection en fonction de la nouvelle largeur et heuteur de la
 * fen�tre */
static void resize(int w, int h) {
  glViewport(0, 0, _windowWidth = w, _windowHeight = h);
  gl4duBindMatrix("projectionMatrix");
  gl4duLoadIdentityf();
  gl4duFrustumf(-0.5, 0.5, -0.5 * _windowHeight / _windowWidth, 0.5 * _windowHeight / _windowWidth, 1.0, 1000.0);
}

/*!\brief r�cup�ration du delta temps entre deux appels */
static double get_dt(void) {
  static double t0 = 0, t, dt;
  t = gl4dGetElapsedTime();
  dt = (t - t0) / 1000.0;
  t0 = t;
  return dt;
}

typedef struct {
  GLfloat x,z;
  int type;
}arbre;

float myRand() {
	return rand() / (RAND_MAX + 1.0);
}
arbre ab[20];
void initObjet(){
  for(int i=0;i<20;i++){
     ab[i].x=150.0 * (2.0 * myRand() - 1.0*myRand());
     ab[i].z=-(70.0 * (2.0 * myRand() - 1.0*myRand()))*2;
  }
  ab[0].x=0;
  ab[0].z=-10;
}
arbre stone[60];
void initStone(){
  for(int i=0;i<60;i++){
    //srand(time(NULL));
    if(i>20){
      stone[i].x=150.0 * (2.0 * myRand() - 1.0*myRand());
      stone[i].z=-(70.0 * (2.0 * myRand() - 1.0*myRand()))*2;
      stone[i].type=STONE;
    }else if((i>20)&&(i<40)){
      stone[i].x=150.0 * (2.0 * myRand() - 1.0*myRand());
      stone[i].z=-(70.0 * (2.0 * myRand() - 1.0*myRand()))*2;
      stone[i].type=GRASS;
    }
    else{
      stone[i].x=150.0 * (2.0 * myRand() - 1.0*myRand());
      stone[i].z=-(70.0 * (2.0 * myRand() - 1.0*myRand()))*2;
      stone[i].type=GRASS;
    }
  }
}
arbre Grass[250];
void initGrass(){
  for(int i=0;i<150;i++){
    Grass[i].x=150.0 * (2.0 * myRand() - 1.0*myRand());
      Grass[i].z=-(70.0 * (2.0 * myRand() - 1.0*myRand()))*2;
      Grass[i].type=GRASS;
  }
}
GLfloat nuit_B=0.5f;
GLfloat nuit_G=0.4f;
int count=0,tour,ok=0;
GLfloat land_y;
static void idle(void) {
  double dt, dtheta = M_PI, pas = 20.0;
  dt = get_dt();
  if((count>=10)&&(count<=10000)){
    _cycle += dt;
    tour++;
    count=0;
  if((tour>=45)&&(ok!=1)){
      nuit_B=nuit_B-0.02;      nuit_G=nuit_G-0.02;
      printf("B::%lf\nG::%lf\n",nuit_B,nuit_G);
      if((nuit_B>=0.12)&&(nuit_G>=0.02)){
        glClearColor(0.0f, nuit_G, nuit_B, 0.0f);
      }
  }
  if(tour==140){
    nuit_B=0.12;nuit_G=0.02;
  }
  if(tour>160){
    ok=1;
    nuit_B=nuit_B+0.02;      nuit_G=nuit_G+0.02;
      printf("B::%lf\nG::%lf\n",nuit_B,nuit_G);
      if((nuit_B<=0.5)&&(nuit_G<=0.4)){
            glClearColor(0.0f, nuit_G, nuit_B, 0.0f);
      }
  }
  if(tour==200){
    tour=0;
    ok=0;
  }
    printf("------>>>>>%d\n",tour);
  }
  if(_keys[KLEFT]) {
    _cam.theta += dt * dtheta;
  }
  if(_keys[KRIGHT]) {
    _cam.theta -= dt * dtheta;
  }
  if(_keys[KUP]) {
    count++;
    _cam.x += -dt * pas * sin(_cam.theta);
    _cam.z += -dt * pas * cos(_cam.theta);
    ///collision_Tree(_cam.x,_cam.z);
  }
  if(_keys[KDOWN]) {
    count++;
    _cam.x += dt * pas * sin(_cam.theta);
    _cam.z += dt * pas * cos(_cam.theta);
  }
}
int stop=1;
/*!\brief interception et gestion des �v�nements "down" clavier */
static void keydown(int keycode) {
  GLint v[2];
  switch(keycode) {
  case SDLK_LEFT:
    _keys[KLEFT] = 1;
    break;
  case SDLK_RIGHT:
    _keys[KRIGHT] = 1;
    break;
  case SDLK_UP:
    _keys[KUP] = stop;
    break;
  case SDLK_DOWN:
    _keys[KDOWN] = 1;
    break;
    case 'f':
    _fog = !_fog;
    break;
  case 'w':
    glGetIntegerv(GL_POLYGON_MODE, v);
    if(v[0] == GL_FILL)
      glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    else
      glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    break;
  case SDLK_ESCAPE:
  case 'q':
    exit(0);
  default:
    break;
  }
}

/*!\brief interception et gestion des �v�nements "up" clavier */
static void keyup(int keycode) {
  switch(keycode) {
  case SDLK_LEFT:
    _keys[KLEFT] = 0;
    break;
  case SDLK_RIGHT:
    _keys[KRIGHT] = 0;
    break;
  case SDLK_UP:
    _keys[KUP] = 0;
    break;
  case SDLK_DOWN:
    _keys[KDOWN] = 0;
    break;
  }
}
int ModuleViewStoneOk=0;
int ModuleViewGrassOk=0;
/*
void collision_Tree(GLfloat cam_x,GLfloat cam_z){
  for(int i=0;i<20;i++){
    printf("i::%d------********cam_x::%lf||cam_z::%lf******ab--x:%lf||ab--z::%lf\n",i,cam_x,cam_z,ab[i].x-3,ab[i].z+3);
    if((cam_x > (ab[i].x-3) && cam_x <= (ab[i].x+3)) && (cam_z > (ab[i].z) && cam_z <= (ab[i].z))){
      stop=0;
      printf("touchhhhhhhheeeeeeeerrrrrr\n");
    }
    
  }
}*/

void drawGrass(GLfloat _cam_x,GLfloat _cam_theta,GLfloat _cam_z,GLfloat landscape_y){
  int xm, ym;
  SDL_PumpEvents();
  SDL_GetMouseState(&xm, &ym);
  for(int i=0;i<20;i++){
        glUseProgram(_pId);
        glUniform1i(glGetUniformLocation(_pId, "fog"), _fog);
        gl4duBindMatrix("modelViewMatrix");
        gl4duLoadIdentityf();
        gl4duLookAtf(_cam_x, landscape_y + 2.0, _cam_z, 
            _cam_x - sin(_cam_theta), landscape_y + 2.0 - (ym - (_windowHeight >> 1)) / (GLfloat)_windowHeight, _cam_z - cos(_cam_theta), 
            0.0, 1.0,0.0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _pId);
        gl4duTranslatef(ab[i].x,heightMapAltitude(ab[i].x, ab[i].z)+20, ab[i].z);
        assimpDrawScene_Grass();
  }
}

void drawG(GLfloat _cam_x,GLfloat _cam_theta,GLfloat _cam_z,GLfloat landscape_y){
  int xm, ym;
  SDL_PumpEvents();
  SDL_GetMouseState(&xm, &ym);
  for(int i=0;i<150;i++){
        glUseProgram(_pId);
        glUniform1i(glGetUniformLocation(_pId, "fog"), _fog);
        gl4duBindMatrix("modelViewMatrix");
        gl4duLoadIdentityf();
        gl4duLookAtf(_cam_x, landscape_y + 2.0, _cam_z, 
            _cam_x - sin(_cam_theta), landscape_y + 2.0 - (ym - (_windowHeight >> 1)) / (GLfloat)_windowHeight, _cam_z - cos(_cam_theta), 
            0.0, 1.0,0.0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _pId);
        gl4duTranslatef(Grass[i].x,heightMapAltitude(Grass[i].x, Grass[i].z)+1, Grass[i].z);
        gl4duScalef(20, 20, 20);
        assimpDrawScene(3);
  }
}


void drawStone(GLfloat _cam_x,GLfloat _cam_theta,GLfloat _cam_z,GLfloat landscape_y){
  int xm, ym;
  SDL_PumpEvents();
  SDL_GetMouseState(&xm, &ym);
  for (int i=0;i<60;i++){
        glUseProgram(_pId);
        glUniform1i(glGetUniformLocation(_pId, "fog"), _fog);
        gl4duBindMatrix("modelViewMatrix");
        gl4duLoadIdentityf();
        gl4duLookAtf(_cam.x, landscape_y + 2.0, _cam.z, 
            _cam.x - sin(_cam.theta), landscape_y + 2.0 - (ym - (_windowHeight >> 1)) / (GLfloat)_windowHeight, _cam.z - cos(_cam.theta), 
            0.0, 1.0,0.0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, _pId);
        gl4duTranslatef(stone[i].x,heightMapAltitude(stone[i].x, stone[i].z)+1,stone[i].z);
        if(i<30){
        assimpDrawScene(1);
        }
        else{
            gl4duTranslatef(stone[i].x,heightMapAltitude(stone[i].x, stone[i].z)-3,stone[i].z);
            gl4duScalef(20, 20, 20);
          assimpDrawScene(2);
        }
   }
}

void drawPlateau(GLfloat _cam_x,GLfloat _cam_theta,GLfloat _cam_z,GLfloat landscape_y,GLfloat temp[],GLfloat lumpos[],GLfloat *mat){
  int xm, ym;
    GLfloat cam[2] = {0, 0};
  SDL_PumpEvents();
  SDL_GetMouseState(&xm, &ym);
gl4duBindMatrix("modelViewMatrix");
  gl4duLoadIdentityf();
  gl4duLookAtf(_cam.x, landscape_y + 2.0, _cam.z, 
	       _cam.x - sin(_cam.theta), (landscape_y + 2.0 - (ym - (_windowHeight >> 1)) / (GLfloat)_windowHeight), _cam.z - cos(_cam.theta), 
	       0.0, 1.0,0.0);
  /* utilisation du shader soleil */
  glUseProgram(_sun_pId);
  gl4duPushMatrix(); {
    gl4duTranslatef(temp[0], temp[1], temp[2]);
    gl4duScalef(15, 15, 15);
    gl4duSendMatrices();
  } gl4duPopMatrix();
  glUniform1f(glGetUniformLocation(_sun_pId, "cycle"), _cycle);
  glUniform1i(glGetUniformLocation(_sun_pId, "plasma"), 0);
  glUniform1i(glGetUniformLocation(_sun_pId, "lave"), 1);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _plasma_tId);
  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_1D, _yellowred_tId);
  glBindTexture(GL_TEXTURE_1D, _smTex);
  glActiveTexture(GL_TEXTURE0);
  gl4dgDraw(_sphere);
  glBindTexture(GL_TEXTURE_1D, 0);
  glActiveTexture(GL_TEXTURE0);
  /* utilisation du shader de terrain */
  glUseProgram(_landscape_pId);
  mat = gl4duGetMatrixData();
  MMAT4XVEC4(lumpos, mat, temp);
  gl4duScalef(_landscape_scale_xz, _landscape_scale_y, _landscape_scale_xz);
  gl4duSendMatrices();
  glUniform4fv(glGetUniformLocation(_landscape_pId, "lumpos"), 1, lumpos);
  glUniform1i(glGetUniformLocation(_landscape_pId, "degrade"), 0);
  glUniform1i(glGetUniformLocation(_landscape_pId, "fog"), _fog);
  glUniform1i(glGetUniformLocation(_landscape_pId, "eau"), 0);
  cam[0] = _cam.x / (2.0 * 100);
  cam[1] = -_cam.z / (2.0 * 100);
  glUniform2fv(glGetUniformLocation(_pId, "cam"), 1, cam);
  glBindTexture(GL_TEXTURE_1D, _terrain_tId);
  gl4dgDraw(_landscape);


  useNoiseTextures(_landscape_pId,1);
  gl4duRotatef(-90, 1, 0, 0);
  MMAT4XVEC4(lumpos, mat, temp);
  gl4duTranslatef(0,0,-.3);
  gl4duSendMatrices();
  glUniform1i(glGetUniformLocation(_landspace_Id, "eau"), 1);
  glBindTexture(GL_TEXTURE_1D, _terrain_tId4);
  gl4dgDraw(_plan);
  unuseNoiseTextures(1);
}

void drawMontagne(GLfloat _cam_x,GLfloat _cam_theta,GLfloat _cam_z,GLfloat landscape_y,GLfloat temp[],GLfloat lumpos[],GLfloat *mat){
   int xm, ym;
  SDL_PumpEvents();
  SDL_GetMouseState(&xm, &ym);
gl4duBindMatrix("modelViewMatrix");
  gl4duLoadIdentityf();
  gl4duLookAtf(_cam.x, landscape_y + 2.0, _cam.z, 
	       _cam.x - sin(_cam.theta), landscape_y + 2.0 - (ym - (_windowHeight >> 1)) / (GLfloat)_windowHeight, _cam.z - cos(_cam.theta), 
	       0.0, 1.0,0.0);
  glBindTexture(GL_TEXTURE_1D, 0);
  glActiveTexture(GL_TEXTURE0);
  glUseProgram(_landscape_pId2);
  mat = gl4duGetMatrixData();
  MMAT4XVEC4(lumpos, mat, temp);
  gl4duScalef(1000, 200, 1000);
  gl4duRotatef(180,0,0,0);
  gl4duTranslatef(0,0.40,0.68);
  gl4duSendMatrices();
  glUniform4fv(glGetUniformLocation(_landscape_pId, "lumpos"), 1, lumpos);
  glUniform1i(glGetUniformLocation(_landscape_pId, "degrade"), 0);
    glUniform1i(glGetUniformLocation(_landscape_pId, "fog"), _fog);
  glBindTexture(GL_TEXTURE_1D, _terrain_tId2);
  gl4dgDraw(_landscape2);

  gl4duBindMatrix("modelViewMatrix");
  gl4duLoadIdentityf();
  gl4duLookAtf(_cam.x, landscape_y + 2.0, _cam.z, 
	       _cam.x - sin(_cam.theta), landscape_y + 2.0 - (ym - (_windowHeight >> 1)) / (GLfloat)_windowHeight, _cam.z - cos(_cam.theta), 
	       0.0, 1.0,0.0);
  glBindTexture(GL_TEXTURE_1D, 0);
  glActiveTexture(GL_TEXTURE0);
 glUseProgram(_landscape_pId3);
  mat = gl4duGetMatrixData();
  MMAT4XVEC4(lumpos, mat, temp);
  gl4duScalef(1000, 300, 1000);
  gl4duRotatef(180,0,0,0);
  gl4duTranslatef(0,0.5,-0.8);
  gl4duSendMatrices();
  glUniform4fv(glGetUniformLocation(_landscape_pId, "lumpos"), 1, lumpos);
  glUniform1i(glGetUniformLocation(_landscape_pId, "degrade"), 0);
  glUniform1i(glGetUniformLocation(_landscape_pId, "fog"), _fog);
  glBindTexture(GL_TEXTURE_1D, _terrain_tId3);
  gl4dgDraw(_landscape3);
}
/*!\brief dessin de la frame */
static void draw(void) {
  /* coordonn�es de la souris */
if(tour>160){
      ok=1;
      _fog = GL_FALSE;
  }
  if((tour>=60)&&(ok!=1)){
        _fog = !GL_FALSE;
  }
  int xm, ym;
  SDL_PumpEvents();
  SDL_GetMouseState(&xm, &ym);
  /* position de la lumi�re (temp et lumpos), altitude de la cam�ra et matrice courante */
  GLfloat temp[4] = {(10 - 1) * cos(_cycle), (10 - 1) * cos(_cycle), -900, 1.0}, lumpos[4], landscape_y, *mat;
  landscape_y = heightMapAltitude(_cam.x, _cam.z);
  temp[0]=100;//(1000 - 1) * cos(_cycle);
  temp[2]=-800;//(1000 - 1) * cos(_cycle);
  temp[1] = (1000 - 1) * cos(_cycle);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  drawPlateau(_cam.x,_cam.theta,_cam.z,landscape_y,temp,lumpos,mat);
  drawMontagne(_cam.x,_cam.theta,_cam.z,landscape_y,temp,lumpos,mat);
  drawGrass(_cam.x,_cam.theta,_cam.z,landscape_y);
  drawStone(_cam.x,_cam.theta,_cam.z,landscape_y);
  drawG(_cam.x,_cam.theta,_cam.z,landscape_y);
    unuseNoiseTextures(1);
}

/*!\brief lib�ration des ressources utilis�es */
static void quit(void) {
  if(_heightMap) {
    free(_heightMap);
    _heightMap = NULL;
  }
  if(_terrain_tId) {
    glDeleteTextures(1, &_terrain_tId);
    _terrain_tId = 0;
  }
  if(_plasma_tId) {
    glDeleteTextures(1, &_plasma_tId);
    _plasma_tId = 0;
    glDeleteTextures(1, &_yellowred_tId);
    _yellowred_tId = 0;
    glDeleteTextures(1, &_terrain_tId);
    _terrain_tId = 0;
  }
  assimpQuit();
  assimpQuit_Grass();
  gl4duClean(GL4DU_ALL);
}

/*!\brief r�cup�ration de l'altitude y de la cam�ra selon son
 *  positionnement dans la heightMap. Cette fonction utilise les
 *  param�tres de scale pour faire la conversion d'un monde �
 *  l'autre. 
 *
 * Pour les �tudiants souhaitant r�aliser un projet type randonn�e, il
 * faudra �tendre cette fonction afin de r�cup�rer la position y
 * exacte quand x et z ne tombent pas exactement sur un sommet mais se
 * trouvent entre 3 sommets. Indice, un triangle poss�de une normale,
 * la normale (a, b, c) donnent les coefficients de l'�quation du plan
 * confondu avec le triangle par : ax + by + cz + d = 0. Reste plus
 * qu'� trouver d puis � chercher y pour x et z donn�s.
 */
static GLfloat heightMapAltitude(GLfloat x, GLfloat z) {
  x = (_landscape_w >> 1) + (x / _landscape_scale_xz) * (_landscape_w / 2.0); 
  z = (_landscape_h >> 1) - (z / _landscape_scale_xz) * (_landscape_h / 2.0);
  if(x >= 0.0 && x < _landscape_w && z >= 0.0 && z < _landscape_h)
    return (2.0 * _heightMap[((int)x) + ((int)z) * _landscape_w] - 1) * _landscape_scale_y;
  return 0;
}

