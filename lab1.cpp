/****** TO DO *****
 * create list of portal objects to check when collision occurs
 * maybe create list of all objects - walls, obstacles, etc
 * make sure to free used memory
 */


//cs335 Spring 2014
//
//program: lab1.c
//author:  Gordon Griesel
//date:    2014
//
//This program demonstrates the use of OpenGL and XWindows
//
//Assignment:
//
//Simplify this program by placing some of the functionality in other files.
//See the lab assignment web page for instructions.
//
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <X11/Xlib.h>
//#include <X11/Xutil.h>
#include <X11/keysym.h>
//#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/gl.h>
//#include <GL/glu.h>

#include "log.h"
#include "fonts.h"
#include "timing.h"
#include "xwin.h"
#include "const.h"
#include "objects.h"

using namespace std;

// defined types
typedef double Flt;
//typedef double Vec[3]; // defined in objects.h
typedef Flt	Matrix[4][4];

// X Windows variables
int xres = 800;
int yres = 600;
int zres = 1;

// Timing variables
const double physicsRate = 1.0 / 60.0;
const double oobillion = 1.0 / 1e9;
struct timespec timeStart, timeCurrent;
struct timespec timePause;
double physicsCountdown=0.0;
double timeSpan=0.0;
//unsigned int upause=0;

//macros
#define rnd() (((double)rand())/(double)RAND_MAX)
#define random(a) (rand()%a)

//function prototypes
void init_opengl(void);
void check_resize(XEvent *e);
void check_mouse(XEvent *e);
void check_keys(XEvent *e);
void init();
void init_sounds(void);
void physics(void);
void render(void);

void putWall(void);
void putOval(void);
void cleanupObjects(void);

void initObjects(void);

//global variables
int done=0;
int keys[65536];
int maxobj = 100;

int objcnt = 0;

object **objects;
portal ** portals;

// shapes
// instead of individual shapes, make a list of shapes
// to do a list of shapes (general), need to move to c++, no issue i think, try for pure c
// maybe struct obj with structs for every obj and integers for count of each object type, one int for count of total objects
Display *dpy;
Window win;
GLXContext glc;

static float pos[3]={200.0,200.0,0.0};
static float vel[3]={0.0,0.0,0.0};

int main(void)
{
	 logOpen();
	 Log("start of main\n");
	 initXWindows();
	 init_opengl();
	 init();
	 clock_gettime(CLOCK_REALTIME, &timePause);
	 clock_gettime(CLOCK_REALTIME, &timeStart);
	 while(!done) {
		  while(XPending(dpy)) {
				XEvent e;
				XNextEvent(dpy, &e);
				check_resize(&e);
				check_mouse(&e);
				check_keys(&e);
		  }
		  //
		  //Below is a process to apply physics at a consistent rate.
		  //1. Get the time right now.
		  clock_gettime(CLOCK_REALTIME, &timeCurrent);
		  //2. How long since we were here last?
		  timeSpan = timeDiff(&timeStart, &timeCurrent);
		  //3. Save the current time as our new starting time.
		  timeCopy(&timeStart, &timeCurrent);
		  //4. Add time-span to our countdown amount.
		  physicsCountdown += timeSpan;
		  //5. Has countdown gone beyond our physics rate? 
		  //       if yes,
		  //           In a loop...
		  //              Apply physics
		  //              Reducing countdown by physics-rate.
		  //              Break when countdown < physics-rate.
		  //       if no,
		  //           Apply no physics this frame.
		  while(physicsCountdown >= physicsRate) {
				//6. Apply physics
				physics();
				//7. Reduce the countdown by our physics-rate
				physicsCountdown -= physicsRate;
		  }
		  //Always render every frame.
		  render();
	 }
	 Log("in main, about to call cleanupXWindows\n");
	 cleanupXWindows();
	 //	 cleanup_fonts();
	 Log("in main, about to call cleanupObjects\n");
	 cleanupObjects();
	 logClose();
	 return 0;
}

void cleanupObjects()
{
	 Log("start of cleanupObjects\n");
	 delete portals;
	 delete objects;
	 Log("end of cleanupObjects\n");
}

void reshape_window(int width, int height)
{
	 setup_screen_res(width, height);
	 //window has been resized.
	 GLint mv [4];
	 glGetIntegerv(GL_VIEWPORT, mv);
	 Log("in reshape window\ngot %d x %d for width and height\n", mv[2], mv[3]);
	 glViewport(0, 0, (GLint)width, (GLint)height);
	 glMatrixMode(GL_PROJECTION); glLoadIdentity();
	 glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	 glOrtho(0, width, 0, height, -1, 1);
	 set_title();
}

void init_opengl(void)
{
	 //OpenGL initialization
	 glViewport(0, 0, xres, yres);
	 //Initialize matrices
	 glMatrixMode(GL_PROJECTION); glLoadIdentity();
	 glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	 //This sets 2D mode (no perspective)
	 glOrtho(0, xres, 0, yres, -1, 1);
	 //Disable some states for simple 2D animation
	 glDisable(GL_LIGHTING);
	 glDisable(GL_DEPTH_TEST);
	 glDisable(GL_FOG);
	 glDisable(GL_CULL_FACE);
	 //Set the clear color of the screen to white
	 glClearColor(1.0, 1.0, 1.0, 1.0);
	 //glClear(GL_COLOR_BUFFER_BIT);
	 //Do this to allow fonts
	 glEnable(GL_TEXTURE_2D);
	 //	 initialize_fonts();
}

void check_resize(XEvent *e)
{
	 //The ConfigureNotify is sent by the
	 //server if the window is resized.
	 if (e->type != ConfigureNotify)
		  return;
	 XConfigureEvent xce = e->xconfigure;
	 if (xce.width != xres || xce.height != yres) {
		  //Window size did change.
		  reshape_window(xce.width, xce.height);
	 }
}

void init()
{
	 Log("start of init()\n");
	 initObjects();

	 portals = new portal*[2];
	 portals[LEFT] = new portal();
	 portals[RIGHT] = new portal();

	 memset(keys, 0, 65536);
	 Log("end of init\n");
}

void resizeObjects(void)
{
	 Log("Start of resizeObjects()\n");
	 object ** tmp;
	 try
	 {
		  tmp = new object * [maxobj*2];
	 }
	 catch (bad_alloc)
	 {
		  printf("Insufficient memory! Aborting\n");
		  exit(1);
	 }
	 catch (...)
	 {
		  printf("An unknown error occured! Exiting\n");
		  exit(2);
	 }
	 for (int i = 0; i < objcnt; i++)
		  tmp[i] = objects[i];
	 maxobj *= 2;
}

void initObjects(void)
{
	 Log("Start of initObjects()\n");
	 try
	 {
		  objects = new object * [maxobj];
	 }
	 catch (bad_alloc)
	 {
		  printf("Insufficient memory! Aborting\n");
		  exit(1);
	 }
	 catch (...)
	 {
		  printf("An unknown error occured! Exiting\n");
		  exit(2);
	 }
	 try
	 {
		  objects[PLAYER] = new object(); // person
		  objects[GUN] = new object();
		  objects[2] = new object(); // floor
		  objects[3] = new object(); // left wall
		  objects[4] = new object(); // right wall
		  objects[5] = new object(); // ceiling
		  objects[6] = new object(); // a wall?
	 }
	 catch (bad_alloc)
	 {
		  printf("Insufficient memory! Aborting\n");
		  exit(1);
	 }
	 catch (...)
	 {
		  printf("An unknown error occured! Exiting\n");
		  exit(2);
	 }
	 //Log("about to add verticies to objects\n");

	 /* lower, left */
	 objects[PLAYER]->addVec(2 * player_width, 1 * player_height, 1);
	 objects[PLAYER]->addVec(2 * player_width, 2.5 * player_height, 1);

	 /* lower, right */
	 objects[PLAYER]->addVec(5 * player_width, 2.5 * player_height, 1);
	 objects[PLAYER]->addVec(5 * player_width, 1 * player_height, 1);

	 /* mid, left */
	 objects[PLAYER]->addVec(1 * player_width, 2.5 * player_height, 1);
	 objects[PLAYER]->addVec(1 * player_width, 5.5 * player_height, 1);

	 /* mid, right */
	 objects[PLAYER]->addVec(6 * player_width, 5.5 * player_height, 1);
	 objects[PLAYER]->addVec(6 * player_width, 2.5 * player_height, 1);

	 /* upper, left */
	 objects[PLAYER]->addVec(2.5 * player_width, 5.5 * player_height, 1);
	 objects[PLAYER]->addVec(2.5 * player_width, 6.5 * player_height, 1);

	 /* upper, right */
	 objects[PLAYER]->addVec(4.5 * player_width, 6.5 * player_height, 1);
	 objects[PLAYER]->addVec(4.5 * player_width, 5.5 * player_height, 1);

	 pos[0] = 200.0;
	 pos[1] = 200.0;
	 pos[2] = 0.0;
	 objects[PLAYER]->setPos(pos[0], pos[1], pos[2]);

	 objects[GUN]->addVec(0, 0, 1);
	 objects[GUN]->addVec(0, gun_height, 1);
	 objects[GUN]->addVec(gun_width, gun_height, 1);
	 objects[GUN]->addVec(gun_width, 0, 1);

	 objects[GUN]->setPos(pos[0], pos[1], pos[2]);

	 objects[2]->addVec(1,1,1);
	 objects[2]->addVec(1,51,1);
	 objects[2]->addVec(xres-1,51,1);
	 objects[2]->addVec(xres-1,1,1);

	 objects[3]->addVec(1, 2, 1);
	 objects[3]->addVec(1, yres-2, 1);
	 objects[3]->addVec(51, yres-2, 1);
	 objects[3]->addVec(51, 2, 1);

	 objects[4]->addVec(xres-51, 2, 1);
	 objects[4]->addVec(xres-51, yres-2, 1);
	 objects[4]->addVec(xres-1, yres-2, 1);
	 objects[4]->addVec(xres-1, 2, 1);

	 objects[5]->addVec(1,yres-51,1);
	 objects[5]->addVec(1,yres-1,1);
	 objects[5]->addVec(xres-1,yres-1,1);
	 objects[5]->addVec(xres-1,yres-51,1);

	 objects[6]->addVec(2*xres/4, 2, 1);
	 objects[6]->addVec(2*xres/4, 200, 1);
	 objects[6]->addVec(3*xres/4, 200, 1);
	 objects[6]->addVec(3*xres/4, 2, 1);

	 for (int i = 0; i < objcnt; i++)
	 {
		  objects[i]->initVectors();
		  objects[i]->setEdges();
	 }

	 // player and gun facing right
	 objects[PLAYER]->invertN();
	 objects[GUN]->invertN();

	 Log("Done with initObjects\n");
}

void check_mouse(XEvent *e)
{
	 //Did the mouse move?
	 //Was a mouse button clicked?
	 static int savex = 0;
	 static int savey = 0;
	 //
	 if (e->type == ButtonRelease) {
		  return;
	 }
	 if (e->type == ButtonPress) {
		  if (e->xbutton.button==1) {
				//Left button is down
		  }
		  if (e->xbutton.button==3) {
				//Right button is down
		  }
	 }
	 if (savex != e->xbutton.x || savey != e->xbutton.y) {
		  //Mouse moved
		  savex = e->xbutton.x;
		  savey = e->xbutton.y;
	 }
}

float makeDegree(const float & rad)
{
	 return ((rad / (2 * pi)) * 360.0);
}

static int jumped = 0;

void check_keys(XEvent *e)
{
	 //keyboard input?
	 static int shift=0;
	 int key = XLookupKeysym(&e->xkey, 0);
	 //Log("key: %i\n", key);
	 if (e->type == KeyRelease)
	 {
		  keys[key]=0;
		  if (key == XK_Shift_L || key == XK_Shift_R)
				shift=0;
		  if (key == XK_space)
		  {
				jumped = 0;
		  }
		  return;
	 }
	 if (e->type == KeyPress)
	 {
		  if (key == XK_Escape)
		  {
				done = 1;
				return;
		  }
		  keys[key]=1;
		  if (key == XK_Shift_L || key == XK_Shift_R) {
				shift=1;
				return;
		  }
		  if (key == XK_space) // doesn't help
		  {
				if (jumped == 1)
					 keys[XK_space] = 0;
		  }
	 }
	 else
	 {
		  return;
	 }
	 if (shift)
	 {
		  // run?
	 }
}

void physics(void)
{
	 Log("start of physics, jumped = %d\n", jumped);
	 int addgrav = 1;

	 pos[0] = objects[PLAYER]->getPos()[0][0];
	 pos[1] = objects[PLAYER]->getPos()[0][1];
	 pos[2] = objects[PLAYER]->getPos()[0][2];
	 if (keys[LEFT_PORTAL_KEY] || keys[XK_z])
	 {
		  portals[LEFT] = new portal();
		  portals[LEFT]->setN(objects[PLAYER]->getN()[0][0], objects[PLAYER]->getN()[0][1], objects[PLAYER]->getN()[0][2]);
		  portals[LEFT]->setVel(15, 15, 0);

	 }
	 if (keys[RIGHT_PORTAL_KEY] || keys[XK_x])
	 {
		  portals[RIGHT] = new portal();
		  portals[RIGHT]->setN(objects[PLAYER]->getN()[0][0], objects[PLAYER]->getN()[0][1], objects[PLAYER]->getN()[0][2]);
		  portals[RIGHT]->setVel(15, 15, 0);
	 }
	 if (keys[XK_d] || keys[XK_Right])
	 {
		  if ((objects[PLAYER]->getEdge(RIGHT)) >= (float)xres) // right edge
		  {
				// do nothing
		  }
		  else
		  {
				if (vel[0] < 15.0)
				{
					 vel[0] += 2.5;
				}
				pos[0] += vel[0];
		  }
	 }
	 if (keys[XK_a] || keys[XK_Left])
	 {
		  if ((objects[PLAYER]->getEdge(LEFT)) <= 0.0) // on left edge
		  {
				//do nothing
		  }
		  else
		  {
				if (vel[0] > -15.0)
					 vel[0] -= 2.5;
				//			pos[0] += vel[0];
		  }
	 }

	 if (keys[XK_w] || keys[XK_Up])
	 {
		  Log("up pressed\n");
		  if (objects[GUN]->getN()[0][1] + 0.015 <= 1.5)
		  {
				objects[GUN]->shiftN(0.0, 0.015, 0.0);
		  }
		  Log("done, angle = %.2f\n", objects[GUN]->getNormalAngle());
	 }

	 if (keys[XK_s] || keys[XK_Down])
	 {
		  Log("down pressed\n");
		  if (objects[GUN]->getN()[0][1] - 0.015 >= -1.5)
		  {
				objects[GUN]->shiftN(0.0, -0.015, 0.0);
		  }
	 }

	 if (keys[XK_space])
	 {
		  Log("space pressed\n");
		  if (jumped || vel[1] != 0.0)
		  {
				//Log("already jumped, doing nothing\n");
				// already jumped, do nothing
		  }
		  else if ((objects[PLAYER]->getEdge(TOP)) >= (float)yres) // top
		  {
				//		  do nothing
				if (vel[1] > 0.0)
				{
					 //	 Log("at top of screen, eliminating upward velocity\n");
					 vel[1] = 0.0;
				}
		  }
		  else
		  {
				//Log("adding jump factor and applying to pos[1]\n");
				vel[1] = 25;
				pos[1] += vel[1];
		  }
		  jumped = 1;
	 }
	 if (pos[0] <= 52.5)
	 {
		  pos[0] = 50.0;
	 }
	 if (pos[1] < 52.5)
	 {
		  pos[1] = 50.0;
		  //Log("setting vel[1] = 0.0, physics, 'if (pos[1] < 0)'\n");
		  vel[1] = 0.0;
		  addgrav = 0;
	 }
	 if (vel[1] < -10000.0) // concerned about possible issue
	 {
		  vel[1] = 0.0;
	 }
	 //Apply gravity
	 if (addgrav == 1 && pos[1] >= 52.5)
	 {
		  vel[1] -= 2.5;
		  //		  pos[1] += vel[1];
	 }
	 if (vel[0] > 0)
	 {
		  vel[0] -= 1;
	 }
	 else if (vel[0] < 0)
	 {
		  vel[0] += 1;
	 }
	 //	 Log("\npreapplying velocity\npos[0] = %f\npos[1] = %f\npos[2] = %f\n", pos[0], pos[1], pos[2]);
	 //	 Log("vel[0] = %f\nvel[1] = %f\nvel[2] = %f\n", vel[0], vel[1], vel[2]);

	 //pos[0]+=vel[0];
	 //pos[1]+=vel[1];
	 //pos[2]+=vel[2];

	 /* stop seizing */
	 if (vel[0] > 0.0 && vel[0] < 1.0)
	 {
		  vel[0] = 0.0;
	 }
	 /* both bellow work */
	 //objects[PLAYER]->setPos(pos);
	 Log("\nin physics, about to shift by x = %.2f, y = %.2f\n", vel[0], vel[1]);
	 objects[PLAYER]->shiftPos(vel[0], vel[1], vel[2]);
	 Log("8. vel[1] = %2.f\n", vel[1]);

	 //	 Log("post:\npos[0] = %f\npos[1] = %f\npos[2] = %f\n", pos[0], pos[1], pos[2]);
}

void putObj(int &i)
{
	 int j = 0;
	 glColor3ub (100, 25, 55);
	 glBegin(GL_POLYGON);
	 while ((objects[i]->getVert(j)) != 0)
	 {
		  //		  Log("\ngetting object[%d] vertex %d\n", i, j);
		  //		  Log("vertex = <%f, %f, %f>\n", (objects[i]->getVert(j)[0]), (objects[i]->getVert(j)[1]), (objects[i]->getVert(j)[2]));
		  glVertex3f ((objects[i]->getVert(j)[0]), (objects[i]->getVert(j)[1]), (objects[i]->getVert(j)[2]));
		  j++;
	 }
	 glEnd();
}

void putPlayer()
{
	 int j = 0;
	 glColor3ub (10, 250, 155);
	 pos[0] = (objects[PLAYER]->getPos()[0][0]);
	 pos[1] = (objects[PLAYER]->getPos()[0][1]);
	 pos[2] = (objects[PLAYER]->getPos()[0][2]);
	 //	 Log("\nin putPlayer\npos[0] = %f, pos[1] = %f\n", pos[0], pos[1]);
	 glTranslatef(pos[0], pos[1], pos[2]);
	 glBegin(GL_QUADS);
	 while ((objects[PLAYER]->getVert(j)) != 0)
	 {
		  //		  Log("\ngetting player, object[%d], vertex %d\n", PLAYER, j);
		  //		  Log("vertex = <%f, %f, %f>\n", (objects[PLAYER]->getVert(j)[0]), (objects[PLAYER]->getVert(j)[1]), (objects[PLAYER]->getVert(j)[2]));
		  glVertex3f ((objects[PLAYER]->getVert(j)[0]), (objects[PLAYER]->getVert(j)[1]), (objects[PLAYER]->getVert(j)[2]));
		  j++;
	 }
	 glEnd();
}

void putGun()
{
	 int j = 0;
	 glColor3ub (50, 70, 15);
	 pos[0] = (objects[GUN]->getPos()[0][0]);
	 pos[1] = (objects[GUN]->getPos()[0][1]);
	 pos[2] = (objects[GUN]->getPos()[0][2]);
	 //	 Log("\nin putGun\npos[0] = %f, pos[1] = %f\n", pos[0], pos[1]);
	 glTranslatef(50, 65, 0); // will be relative to last translate call
	 glRotatef (makeDegree(objects[GUN]->getNormalAngle()), 0.0, 0.0, 1.0);
	 glBegin(GL_QUADS);
	 while ((objects[GUN]->getVert(j)) != 0)
	 {
		  //		  Log("\ngetting gun, object[%d], vertex %d\n", GUN, j);
		  //		  Log("vertex = <%f, %f, %f>\n", (objects[GUN]->getVert(j)[0]), (objects[GUN]->getVert(j)[1]), (objects[GUN]->getVert(j)[2]));
		  glVertex3f ((objects[GUN]->getVert(j)[0]), (objects[GUN]->getVert(j)[1]), (objects[GUN]->getVert(j)[2]));
		  j++;
	 }
	 glEnd();
}

void putPortal (const int &i)
{
	 int j = 0;
	 if (i == LEFT)
	 {
		  glColor3ub (120, 200, 15);
	 }
	 else if (i == RIGHT)
	 {
		  glColor3ub (90, 7, 150);
	 }
	 else
	 {
		  glColor3ub (50, 70, 15);
	 }
	 pos[0] = (portals[i]->getPos()[0][0]);
	 pos[1] = (portals[i]->getPos()[0][0]);
	 pos[2] = (portals[i]->getPos()[0][0]);
	 //	 Log("\nin putGun\npos[0] = %f, pos[1] = %f\n", pos[0], pos[1]);
	 glRotatef (makeDegree(objects[GUN]->getNormalAngle()), 0.0, 0.0, 1.0);
	 glBegin(GL_QUADS);
	 if (0) // if (collision)
	 {
		  while ((portals[i]->getVert(j)) != 0)
		  {
				//		  Log("\ngetting gun, object[%d], vertex %d\n", GUN, j);
				//		  Log("vertex = <%f, %f, %f>\n", (objects[GUN]->getVert(j)[0]), (objects[GUN]->getVert(j)[1]), (objects[GUN]->getVert(j)[2]));
				glVertex3f ((portals[i]->getVert(j)[0]), (portals[i]->getVert(j)[1]), (portals[i]->getVert(j)[2]));
				j++;
		  }
	 }
	 else // no collision
	 {
		  pos[0] += portals[i]->getVel()[0][0];
		  pos[1] += portals[i]->getVel()[0][1];
		  pos[2] += portals[i]->getVel()[0][2];
		  glVertex3f (0.0, 0.0, 0.0);
		  glVertex3f (0.0, 5.0, 0.0);
		  glVertex3f (5.0, 5.0, 0.0);
		  glVertex3f (5.0, 0.0, 0.0);
	 }
	 glEnd();
}

void render(void)
{
	 //	 Log("in render, objcnt = %d\n", objcnt);
	 int i = 0;
	 Log("normal angle of PLAYER = %.2f, normal angle of GUN = %.2f\n", objects[PLAYER]->getNormalAngle(), objects[GUN]->getNormalAngle());
	 Log("directional angle of PLAYER = %.2f, directional angle of GUN = %.2f\n", objects[PLAYER]->getDirectionalAngle(), objects[GUN]->getDirectionalAngle());
	 glClear(GL_COLOR_BUFFER_BIT);
	 {
		  glPushMatrix(); // for data storage on vid card, only beneficial for static(ish) objects supposidly

		  for (i = 0; i < PLAYER; i++)
		  {
				putObj(i);
		  }
		  for (i = PLAYER+1; i < GUN; i++)
		  {
				putObj(i);
		  }
		  for (i = GUN+1; i < objcnt; i++)
		  {
				putObj(i);
		  }
		  putPlayer();
		  putGun();

//		  putPortal(LEFT);
//		  putPortal(RIGHT);

		  glPopMatrix();

		  glEnd();

	 }
	 glXSwapBuffers(dpy, win);
}

