//-----------------------------------------------------------------------------
// name: egg.cpp
// desc: hello egg
//
// author: Xingxing Yang (xingxing@ccrma.stanford.edu)
//   date: fall 2017
//   uses: RtAudio by Gary Scavone
//-----------------------------------------------------------------------------


#include "RtAudio/RtAudio.h"
#include "chuck.h"
#include "chuck_fft.h"
#include <stdlib.h>
#include <math.h>
#include <sstream>
#include <iostream>
using namespace std;

#ifdef __MACOSX_CORE__
#include <GLUT/glut.h>
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif


bool fullScreen = false;

void initOpenGL();
void reshape(int w, int h);

void render();
void backscene();
void spectrum(float size, float xpos, float ypos);

void keyboard(unsigned char c, int x, int y);
void mouse(int button, int state, int x, int y);

void help();

// our datetype
#define SAMPLE float
// corresponding format for RtAudio
#define MY_FORMAT RTAUDIO_FLOAT32
// sample rate
#define MY_SRATE 44100
// number of channels
#define MY_CHANNELS 1
// for convenience
#define MY_PIE 3.14159265358979

// width and height
long g_width = 1024;
long g_height = 720;

// global buffer
SAMPLE * g_buffer = NULL;
long g_bufferSize;

SAMPLE * g_buffer_playback = NULL;
long g_bufferPlaybackSize;

// global variables
bool g_draw_dB = false;
ChucK * the_chuck = NULL;

float resizex = 1;
float resizey = 1;
bool t = false;

int fc = 1760;

//-----------------------------------------------------------------------------
// name: help()
// desc: ...
//-----------------------------------------------------------------------------
void help() {
    fprintf(stderr, "----------------------------------------------------\n" );
    fprintf(stderr, "Hi egg(0.0)\n");
    fprintf(stderr, "Xingxing Yang\n");
    fprintf(stderr, "----------------------------------------------------\n" );
    fprintf(stderr, "'t' - toggle chuck ring modulation\n");
    fprintf(stderr, "'f' - toggle fullscreen\n");
    fprintf(stderr, "'Q/q' - quit\n");
    fprintf(stderr, "----------------------------------------------------\n" );
    fprintf(stderr, "\n");	
}


class Egg {
private:
	float x, y;
	float tilt;
	float tiltp;
	float angle;
	float scalar;

public:
	Egg(float xpos, float ypos, float t, float s): x(xpos), y(ypos), tiltp(t), scalar(s) {}
	void wobble();
	void display();
};

void Egg::wobble() {
	tilt = cos(angle) * 16;
	angle += tiltp;
	// cout << tilt << endl;
}

void Egg::display() {
	// resizex = glutGet(GLUT_WINDOW_WIDTH)/640.0;
	resizey = glutGet(GLUT_WINDOW_HEIGHT)/480.0;
	glTranslatef(x*glutGet(GLUT_WINDOW_WIDTH), y*glutGet(GLUT_WINDOW_HEIGHT), -0.2);
	glRotatef(tilt, 0, 0, 1);
	glScalef(scalar * resizey, scalar * resizey, 0);
    if(!t)
        glColor3f(1.0, 1.0, 1.0);
    else
        glColor3f(1.0, .6, .6);

	GLfloat ctrlpoints[13][3] = {
		{0, -100, 0},
		{25, -100, 0}, 
		{40, -65, 0}, 
		{40, -40, 0},
		{40, -15, 0},
		{25, 0, 0},
		{0, 0, 0},
		{-25, 0, 0},
		{-40, -15, 0},
		{-40, -40, 0},
		{-40, -65, 0},
		{-25, -100, 0},
		{0, -100, 0}
	};

	for (int i = 0; i < 12; i += 3) {
		glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 4, &ctrlpoints[i][0]);
		glEnable(GL_MAP1_VERTEX_3);
		
		glBegin(GL_TRIANGLE_FAN);
		for (int i = 0; i < 300; i++) 
			glEvalCoord1f((GLfloat)i/300.0);
		glEnd();
	}	

	glBegin(GL_POLYGON);
	for (int i = 0; i < 12; i += 3)
	   	glVertex3f(ctrlpoints[i][0], ctrlpoints[i][1], ctrlpoints[i][2]);
	glEnd();

	glLineWidth(2);
	glBegin(GL_LINES);
	for (int i = 3; i <= 12; i += 3) {
		glVertex3f(ctrlpoints[i][0], ctrlpoints[i][1], ctrlpoints[i][2]);
		glVertex3f(ctrlpoints[i-3][0], ctrlpoints[i-3][1], ctrlpoints[i-3][2]);
	}
	glEnd();

	// glPointSize(5.0);
 //  	glColor3f(0.0, 1.0, 1.0);
 //  	glBegin(GL_POINTS);
 //   	for (int i = 0; i < 12; i++) 
 //       glVertex3fv(&ctrlpoints[i][0]);
 //  	glEnd();
}

//-----------------------------------------------------------------------------
// name: callme()
// desc: audio callback
//-----------------------------------------------------------------------------
int callme( void * outputBuffer, void * inputBuffer, unsigned int numFrames,
            double streamTime, RtAudioStreamStatus status, void * data )
{
    // cast!
    SAMPLE * input = (SAMPLE *)inputBuffer;
    SAMPLE * output = (SAMPLE *)outputBuffer;
    
    if(t)
        the_chuck->run(input, output, numFrames);
    else
        for( int i = 0; i < numFrames; i++ ) {
            output[i] = 0;
        }

    // fill
    for( int i = 0; i < numFrames; i++ )
    {
        // assume mono
        g_buffer[i] = input[i];
        g_buffer_playback[i] = output[i];
    }
    
    return 0;
}


//-----------------------------------------------------------------------------
// name: initChucK()
// desc: initialize ChucK
//-----------------------------------------------------------------------------
bool initChucK()
{
    // NOTE: instantiate ChucK here...
    the_chuck = new ChucK();

    // NOTE: set params here...
    the_chuck->setParam(CHUCK_PARAM_SAMPLE_RATE, t_CKFLOAT(MY_SRATE));
    // the_chuck->setParam(CHUCK_PARAM_OUTPUT_CHANNELS, MY_CHANNELS);
    the_chuck->setParam(CHUCK_PARAM_INPUT_CHANNELS, t_CKINT(MY_CHANNELS));

    // NOTE: initialize ChucK here...
    the_chuck->init();
    
    return true;
}


//-----------------------------------------------------------------------------
// name: main()
// desc: entry point
//-----------------------------------------------------------------------------
int main(int argc, char** argv) {
	glutInit(&argc, argv);
	initOpenGL();

	// instantiate RtAudio object
    RtAudio audio;
    // variables
    unsigned int bufferBytes = 0;
    // frame size
    unsigned int bufferFrames = 1024;
    
    // check for audio devices
    if( audio.getDeviceCount() < 1 )
    {
        // nopes
        cout << "no audio devices found!" << endl;
        exit( 1 );
    }

    // let RtAudio print messages to stderr.
    audio.showWarnings( true );
    
    // set input and output parameters
    RtAudio::StreamParameters iParams, oParams;
    iParams.deviceId = audio.getDefaultInputDevice();
    iParams.nChannels = MY_CHANNELS;
    iParams.firstChannel = 0;
    oParams.deviceId = audio.getDefaultOutputDevice();
    oParams.nChannels = MY_CHANNELS;
    oParams.firstChannel = 0;
    
    // create stream options
    RtAudio::StreamOptions options;

    // go for it
    try {
        // open a stream
        audio.openStream( &oParams, &iParams, MY_FORMAT, MY_SRATE, &bufferFrames, &callme, (void *)&bufferBytes, &options );
    }
    catch( RtError& e )
    {
        // error!
        cout << e.getMessage() << endl;
        exit( 1 );
    }
    
    // compute
    bufferBytes = bufferFrames * MY_CHANNELS * sizeof(SAMPLE);
    // allocate global buffer
    g_bufferSize = bufferFrames;
    g_buffer = new SAMPLE[g_bufferSize];
    memset( g_buffer, 0, sizeof(SAMPLE)*g_bufferSize );

    // allocate playback buffer
    g_bufferPlaybackSize = bufferFrames;
    g_buffer_playback = new SAMPLE[g_bufferPlaybackSize];
    memset( g_buffer_playback, 0, sizeof(SAMPLE)*g_bufferPlaybackSize );

    if(!initChucK())
    exit( 1 );

    stringstream ss;
    ss << "adc => Gain g => dac; SinOsc foo => g; 3 => g.op; " << fc << " => foo.freq; while( true ) { 1::second => now;}";
    the_chuck->compileCode(ss.str(), "");
    the_chuck->start();

	// print usage
	help();
    
    // go for it
    try {
        // start stream
        audio.startStream();
        
        // let GLUT handle the current thread from here
        glutMainLoop();
        
        // stop the stream.
        audio.stopStream();
    }
    catch( RtError& e )
    {
        // print error message
        cout << e.getMessage() << endl;
        goto cleanup;
    }
    
cleanup:
    // close if open
    if( audio.isStreamOpen() )
        audio.closeStream();
    
    // done
    return 0;
}

void initOpenGL() {
	// void glClearColor(GLclampf red,  GLclampf green,  GLclampf blue,  GLclampf alpha);
	// alpha - opacity
	// glClearColor(0.8, 0.0, 0.0, 0.1);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(640, 480);
	glutCreateWindow("Simple GLUT Application");

	glutDisplayFunc(render);
	//  If enabled, the idle callback is continuously called when events are not being received. 
	glutIdleFunc(render);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);
	
	glEnable(GL_DEPTH_TEST);    
	glEnable(GL_POINT_SMOOTH);
	// GL_LEQUAL
 //    	Passes if the incoming depth value is less than or equal to the stored depth value. 
	glDepthFunc(GL_LEQUAL);
	glShadeModel(GL_FLAT);
}

void reshape(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0.0f, w, h, 0.0f, 0.0f, 1.0f);
	// gluPerspective(60, (float)w/(float)h, 1.0, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void keyboard(unsigned char c, int x, int y) {
	switch(c) {
		case 'q':
			exit(0);
		case 'f':
			fullScreen = !fullScreen;
			if (fullScreen) {
				glutFullScreen();
			} else {
				glutReshapeWindow(640, 480);
				glutPositionWindow(100, 100);
			}
			break;
        case 't':
            t = !t;
            break;
	}
	glutPostRedisplay();
}

void mouse(int button, int state, int x, int y) {
	if (button == GLUT_RIGHT_BUTTON) {
		exit(0);
	}
	glutPostRedisplay();
}

// The background scene
void backscene() {
	GLfloat x = 0;
	GLfloat w = glutGet(GLUT_WINDOW_WIDTH);
	GLfloat h = glutGet(GLUT_WINDOW_HEIGHT);
	GLfloat xinc = w*0.8/g_bufferSize;

	// color
    glColor3f( .7, .8, .8 );
    
    // start primitive
    for( int i = 0; i < g_bufferSize; i++ )
    {
        // line width
        // glLineWidth(float(g_buffer[i]) * 1000000);
        glLineWidth(2);
        // plot
        glBegin(GL_LINES);
            glVertex3f( x+w*0.1, h*0.1+100*g_buffer[i], -0.8 );
            glVertex3f( x+w*0.1, h*0.9+100*g_buffer[i+1], -0.8 );
        glEnd();

        // increment x
        x += xinc;
    }
}

// size 0.0-1.0, xpos 0.0-1.0, ypos 0.0-1.0
// spectrum 
void spectrum(float size, float xpos, float ypos) {
	SAMPLE g_window[g_bufferPlaybackSize];

    hanning( g_window, g_bufferPlaybackSize );
    apply_window( g_buffer_playback, g_window, g_bufferPlaybackSize );

    rfft( g_buffer_playback, g_bufferPlaybackSize/2, 1);
    complex * cbuf = (complex *)g_buffer_playback;

	GLfloat x = 0;
	GLfloat w = glutGet(GLUT_WINDOW_WIDTH);
	GLfloat h = glutGet(GLUT_WINDOW_HEIGHT);

	float f, inc;
	float f0 = 1 / (g_bufferPlaybackSize/2.0) * (MY_SRATE/2.0);

    GLfloat xinc = w*size/g_bufferPlaybackSize;
    // color
    glColor3f( 1, .6, .6 );
    
    glLineWidth(3);

    // start primitive
    glBegin( GL_LINE_STRIP );

    for (int i = 0; i < g_bufferPlaybackSize/2; i++) {
        // // plot
        // glVertex3f(x + w * xpos, h * ypos - 4000 * cmp_abs(cbuf[i]), 0);
        // // cout << x << endl;
        // // increment x
        // x += xinc * 2;
    	f = i / (g_bufferPlaybackSize/2.0) * (MY_SRATE/2.0);
        inc = log2(f/f0) * (w*size/log2(MY_SRATE/2.0/f0));
        if (i >=1 ) 
	        glVertex2f(inc + w * xpos, h * ypos - 4000 * cmp_abs(cbuf[i]));
    }

    // end primitive
    glEnd();
}

Egg Tom(0.4, 0.7, 0.1, 2);

void render() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glLoadIdentity();
	backscene();

	glLoadIdentity();
	spectrum(0.2, 0.6, 0.3);

	glLoadIdentity();
    if (t) {
    	Tom.wobble();
    }
	Tom.display();

	glutSwapBuffers();
	glFlush();
}