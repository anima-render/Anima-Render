#include <GL\glew.h>
#ifdef __APPLE__
#include <GLUT\glut.h>
#else
#include <GL\glut.h>
#endif

#include <boost\program_options.hpp>

#include <iostream>

#include "scene\Scene.h"
#include "scene\scene_parser.h"

using namespace std;

namespace po = boost::program_options;

Scene *scn;

GLfloat fbo_vertices[] = {
	-1.0f, -1.0f, -1.0f,
     1.0f, -1.0f, -1.0f,
    -1.0f,  1.0f, -1.0f,
     1.0f,  1.0f, -1.0f,
};

GLfloat fbo_st[] = { 
     0.0f, 0.0f, 
     1.0f, 0.0f,
	 0.0f, 1.0f,
	 1.0f, 1.0f
};

GLushort fbo_elements[] = { 0, 1, 2, 3 };

GLuint fbo, fbo_texture, rbo_depth;
GLuint vbo_fbo_vertices, vbo_fbo_st, fbo_elements_buf;

static void render(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	scn->getActiveCamera().cameraMove();

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt(
		scn->getActiveCamera().getPosition().x, 
		scn->getActiveCamera().getPosition().y, 
		scn->getActiveCamera().getPosition().z,
		scn->getActiveCamera().getDirection().x,
		scn->getActiveCamera().getDirection().y,
		scn->getActiveCamera().getDirection().z,
		scn->getActiveCamera().getUp().x,
		scn->getActiveCamera().getUp().y,
		scn->getActiveCamera().getUp().z
		);
	scn->previsitLights();
	scn->render();
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glLoadIdentity();
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
	glUseProgram(scn->getActiveCamera().postprocessData.program);	

	glUniform1f(scn->getActiveCamera().textureHeightUniformLocation, (float)glutGet(GLUT_WINDOW_HEIGHT));

	glUniform1f(scn->getActiveCamera().textureWidthUniformLocation, (float)glutGet(GLUT_WINDOW_WIDTH));

	glUniform1i(scn->getActiveCamera().textureUniformLocation, 9);
	glActiveTexture(GL_TEXTURE9);
	glBindTexture(GL_TEXTURE_2D, fbo_texture);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_fbo_vertices);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(
		3,
		GL_FLOAT,
		sizeof(GLfloat)*3,
		(void*)0
		);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_fbo_st);
    glClientActiveTexture(GL_TEXTURE9);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(
        2,                                /* size */
        GL_FLOAT,                         /* type */
        sizeof(GLfloat)*2,               				  /* stride */
        (void*)0                          /* array buffer offset */
    );

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, fbo_elements_buf);
    
	glDrawElements(
        GL_TRIANGLE_STRIP,  /* mode */
        4,                  /* count */
        GL_UNSIGNED_SHORT,  /* type */
        (void*)0            /* element array buffer offset */
    );
	glutSwapBuffers();
}

static void reshape(int width, int height)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glViewport(0, 0, width, height);
	gluPerspective(scn->getActiveCamera().getFovY()*2, (float)width/(float)height, 0.01, 50.0);
	glMatrixMode(GL_MODELVIEW);

	// Rescale FBO and RBO as well
	glBindTexture(GL_TEXTURE_2D, fbo_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, width, height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
}

//-------------------------------------------------------------------------------------
//-- Keyboard -------------------------------------------------------------------------
//-------------------------------------------------------------------------------------

//Imposta il movimento della camera nella direzione del pulsante premuto
static void processNormalKeys(unsigned char key, int xx, int yy) { 

		if (key == 27)
			  exit(0);

		switch (key){
		case 'q':			// Roll verso sinistra
			scn->getActiveCamera().setCameraRoll(1);
			break;
		case 'e':			// Roll verso destra
			scn->getActiveCamera().setCameraRoll(-1);
			break;
		case 'w':			// Movimento avanti
			scn->getActiveCamera().setMoveAhead(1);
			break;
		case 's':			// Movimento indietro
			scn->getActiveCamera().setMoveAhead(-1);
			break;
		case 'a':			// Movimento a sinistra
			scn->getActiveCamera().setMoveLateral(-1);
			break;
		case 'd':			// Movimento a destra
			scn->getActiveCamera().setMoveLateral(1);
			break;
		case 'z':			// Movimento in su
			scn->getActiveCamera().setMoveUp(-1);
			break;
		case 'x':			// Movimento in gi�
			scn->getActiveCamera().setMoveUp(1);
			break;
		case 'o':			// Camera precedente
			scn->prevCamera();
			break;
		case 'p':			// Camera successiva
			scn->nextCamera();
			break;
		}

		glutPostRedisplay();
} 

//Ferma il movimento della camera nella direzione del pulsante rilasciato
static void keyUp(unsigned char key, int xx, int yy){
	
	switch (key){
		case 'w':
		case 's':			
			scn->getActiveCamera().setMoveAhead(0);
			break;
		case 'a':
		case 'd':			
			scn->getActiveCamera().setMoveLateral(0);
			break;
		case 'q':
		case 'e':			
			scn->getActiveCamera().setCameraRoll(0);
			break;
		case 'z':
		case 'x':			
			scn->getActiveCamera().setMoveUp(0);
			break;
		}
}

//-------------------------------------------------------------------------------------
//-- Mouse ----------------------------------------------------------------------------
//-------------------------------------------------------------------------------------


static void mouseMove(int x, int y)
{
	// Rotazione del mouse
	scn->getActiveCamera().mouseMove(x, y);
}

static void mouseButton(int button, int state, int x, int y) {

	// Viene effettuato solo se il pulsante sinistro del mouse � premuto
	if (button == GLUT_LEFT_BUTTON) {

		if (state == GLUT_UP)
		{	
			scn->getActiveCamera().resetMouseMove();
		}
		else  {
			scn->getActiveCamera().setMouseMove(x,y);
		}
	}
}

bool init_framebuffer()
{
	glActiveTexture(GL_TEXTURE9);
	glGenTextures(1, &fbo_texture);
	glBindTexture(GL_TEXTURE_2D, fbo_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT), 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);


	glGenRenderbuffers(1, &rbo_depth);
	glBindRenderbuffer(GL_RENDERBUFFER, rbo_depth);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT));
	glBindRenderbuffer(GL_RENDERBUFFER, 0);


	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fbo_texture, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo_depth);


	GLenum status;
	if ((status = glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE) 
	{
		fprintf(stderr, "glCheckFramebufferStatus: error %p", status);
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	vbo_fbo_vertices = make_buffer(GL_ARRAY_BUFFER, fbo_vertices, sizeof(fbo_vertices));
	vbo_fbo_st = make_buffer(GL_ARRAY_BUFFER, fbo_st, sizeof(fbo_st));
	fbo_elements_buf = make_buffer(GL_ELEMENT_ARRAY_BUFFER, fbo_elements, sizeof(fbo_elements));
	

	return true;
}


//-------------------------------------------------------------------------------------
//-- Main -----------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	int width, height;
	unsigned int glutOptions = GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH;
	string scenefile;

	//Parsing della riga di comando
	po::options_description desc("Available options");
	desc.add_options()
		( "help", "show help")
		( "width", po::value<int>(&width)->default_value(500), "window width")
		( "height", po::value<int>(&height)->default_value(500), "windows height")
		( "scene", po::value<string>(), "file to render")
		;
	po::positional_options_description pos;
	pos.add("scene", 1);
	po::variables_map vm;
	po::store(po::command_line_parser(argc, argv).options(desc).positional(pos).run(), vm);
	po::notify(vm);    
	//Fine del parsing
	
	//Controllo delle opzioni della riga di comando
	if (vm.count("help")) 
	{
		cout << desc << endl;
		return EXIT_FAILURE;
	}

	if (vm.count("height")) 
	{
		height = vm["height"].as<int>();
	}

	if (vm.count("width")) 
	{
		width = vm["width"].as<int>();
	}

	if (vm.count("scene")) 
	{
		scenefile = vm["scene"].as<string>();
	}
	else
	{
		cout << desc << endl;
		cout << "A scene must be specified." << endl;
		return EXIT_FAILURE;
	}

	//Inizializzazione di glut
	glutInit(&argc, argv);
	glutInitDisplayMode(glutOptions);
	glutInitWindowSize(width, height);
	glutCreateWindow("Anima Render");
	glutDisplayFunc(&render);
	glutReshapeFunc(&reshape);

	glutIdleFunc(&render); //Evitiamo le chiamate a glutPostRedisplay()

	//Gestione della keyboard
	glutIgnoreKeyRepeat(1);
	glutKeyboardFunc(&processNormalKeys);
	glutKeyboardUpFunc(&keyUp);

	//Gestione del mouse
	glutMouseFunc(&mouseButton);
	glutMotionFunc(&mouseMove);

	glewInit();
	if(!GLEW_VERSION_2_0)
	{
		cout << "OpenGL 2.0 not available, check if the current driver supports it." << endl;
		return EXIT_FAILURE;
	}

	//Vediamo se il caricamento � effettivamente riuscito
#ifndef _DEBUG
	try
	{
#endif
	scn = Scene::load(scenefile);
#ifndef _DEBUG
	}
	catch(ParseException e)
	{
		cout << e.what() << endl;
		return EXIT_FAILURE;
	}
#endif
	glEnable(GL_TEXTURE_2D);
	if(!init_framebuffer())
	{
		return EXIT_FAILURE;
	}

	//Render loop principale
	glutMainLoop();

	delete scn;

	return EXIT_SUCCESS;
}