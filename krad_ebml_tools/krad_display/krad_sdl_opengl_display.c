#include <krad_sdl_opengl_display.h>

GLuint   program_object;  // a handler to the GLSL program used to update
GLuint   vertex_shader;   // a handler to vertex shader. This is used internally 
GLuint   fragment_shader; // a handler to fragment shader. This is used internally too

// a simple vertex shader source
// this just rotates a quad 45°
static const char *vertex_source = {
"varying vec4 color;"
"void main(){"
//"gl_FrontColor = gl_Color;"
"gl_TexCoord[0] = gl_MultiTexCoord0;"
"    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
//"color = gl_Color; "
"}"
};

// a simple fragment shader source
// this change the fragment's color by yellow color
static const char *fragment_source = {
"varying vec4 color;"
"void main(void){"
"   gl_FragColor = gl_Color;"
"}"
};

void krad_sdl_opengl_display_render(krad_sdl_opengl_display_t *krad_sdl_opengl_display, unsigned char *y, int ys, unsigned char *u, int us, unsigned char *v, int vs) {

	//printf("krad_sdl_opengl_display_render and scale called\n");

	int rgb_stride_arr[3] = {4*krad_sdl_opengl_display->width, 0, 0};

	const uint8_t *yv12_arr[4];
	
	yv12_arr[0] = y;
	yv12_arr[1] = u;
	yv12_arr[2] = v;


	int yv12_str_arr[4];
	
	yv12_str_arr[0] = ys;
	yv12_str_arr[1] = us;
	yv12_str_arr[2] = vs;



	unsigned char *dst[4];
	dst[0] = krad_sdl_opengl_display->rgb_frame_data;

	pthread_rwlock_wrlock (&krad_sdl_opengl_display->frame_lock);
	sws_scale(krad_sdl_opengl_display->sws_context, yv12_arr, yv12_str_arr, 0, krad_sdl_opengl_display->videoheight, dst, rgb_stride_arr);
	pthread_rwlock_unlock (&krad_sdl_opengl_display->frame_lock);
}

void krad_sdl_opengl_read_screen(krad_sdl_opengl_display_t *krad_sdl_opengl_display, void *buffer) {

	glReadPixels(0, 0, krad_sdl_opengl_display->width, krad_sdl_opengl_display->height, GL_BGRA, GL_UNSIGNED_BYTE, buffer);
	//glReadPixels(64, 64, 64, 64, GL_BGRA, GL_BYTE, buffer);
}


void krad_sdl_opengl_draw_screen(krad_sdl_opengl_display_t *krad_sdl_opengl_display) {


	float hud_aspectRatio = krad_sdl_opengl_display->hud_width / (float) krad_sdl_opengl_display->hud_height;

	// get the window aspect ratio
	float aspectRatio = krad_sdl_opengl_display->width / (float) krad_sdl_opengl_display->height;
	float windowAspectRatio = krad_sdl_opengl_display->width / (float) krad_sdl_opengl_display->height;
	float halfWindowAspectRatio = windowAspectRatio * 0.5f;
	// aspectRatio is the aspect ratio of your frame or texture
	float halfAspectRatio = aspectRatio * 0.5f;
	float left, right, top, bottom;
	if (aspectRatio < windowAspectRatio) {
		// top and bottom should be flush with window
		top = 0.5f; // remember this is the top of the texture
		left = -halfWindowAspectRatio;
	} else {
		// left and right should be flush with window
		left = -halfAspectRatio;
		// we are keeping top relative to the windowAspectRatio (normalizing here)
		top = halfAspectRatio / windowAspectRatio;
	}
	
	right = -left;
	bottom = -top;

	glBindTexture (GL_TEXTURE_2D, 4);
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	pthread_rwlock_rdlock(&krad_sdl_opengl_display->frame_lock);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, krad_sdl_opengl_display->width, krad_sdl_opengl_display->height, 0, GL_BGRA, GL_UNSIGNED_BYTE, krad_sdl_opengl_display->rgb_frame_data);
	pthread_rwlock_unlock(&krad_sdl_opengl_display->frame_lock);


	if (krad_sdl_opengl_display->hud_data != NULL) {
		glBindTexture (GL_TEXTURE_2D, 5);
		glPixelStorei (GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvf (GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, krad_sdl_opengl_display->hud_width, krad_sdl_opengl_display->hud_height, 0, GL_BGRA, GL_UNSIGNED_BYTE, krad_sdl_opengl_display->hud_data);
	}

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW); 	
	glLoadIdentity();

	static float speed = 0.035f;
	static float k = 0.0f;
	static float zoom = 0.0f;
	static float raise = 0.0f;
	static float lastzoom = 0.0f;
	static float zoomrange = 1.0f;
	static float rotaterange = 35.0f;
	
	static int animate_test = 0;
	static int zoom_in = 0;

	k += speed;

	if (animate_test) {
		if (!(zoom_in)) {
			//k += speed;
			zoomrange = 3.0f;
			zoom = (zoomrange * -1) - (-(zoomrange)*sinf(k));
			//printf("zoom is %f zoomrange is %f final is %f \n", zoom, zoomrange, zoom + -2.0f);	
			glTranslatef(0.0f, 0.0f, zoom + -1.5f);
			glRotatef(rotaterange*sinf(k), 1.0f, 0.3f, 0.4f);
			//glScalef(zoomrange*sinf(k) + zoomrange, zoomrange*sinf(k) + zoomrange, 1.0f);
		} else {
			if (zoom != zoomrange) {

				if (zoom == 0.0f) {
					zoomrange = 3.0f;
					zoom = zoomrange * 2.0f;
				}
				
				//k += speed;
				zoom = (zoomrange * -1) - (-(zoomrange)*sinf(k));
				
				if (lastzoom == 0.0f) {
					lastzoom = zoom;
				}
				
				if (lastzoom > zoom) {
					glTranslatef(0.0f, 0.0f, -1.0f);
					zoom_in = 0;
					animate_test = 0;
				} else {
					//printf("zoom is %f zoomrange is %f final is %f k is %f\n", zoom, zoomrange, zoom + -2.0f, k);	
					glTranslatef(0.0f, 0.0f, zoom + -1.0f);
					//glRotatef(rotaterange*sinf(k), 1.0f, 0.3f, 0.4f);
					raise += speed;
					if (raise > 1.0f) {
						raise = 1.0f;
					}
					glScalef(1.0f, raise, 1.0f);
				}
				
				lastzoom = zoom;
			}
		}
	} else {
		glTranslatef(0.0f, 0.0f, -1.0f);
	}

	
	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	glFrustum (left, right, bottom, top, 1.00, 50.0);
	glMatrixMode (GL_MODELVIEW);
	
	//glUseProgram(program_object);

	// reset color
	glColor3f(1.0f, 1.0f, 1.0f);
	
	
	// video display
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 4);

	glBegin(GL_QUADS);
	{
	 glTexCoord2f(0.0f, 0.0f);
	 glVertex2f(-aspectRatio * 0.5f, 0.5f); 
	 glTexCoord2f(0.0f, 1.0f);
	 glVertex2f(-aspectRatio  * 0.5f, -0.5f); 
	 glTexCoord2f(1.0f, 1.0f);
	 glVertex2f(aspectRatio  * 0.5f, -0.5f); 
	 glTexCoord2f(1.0f, 0.0f);
	 glVertex2f(aspectRatio  * 0.5f, 0.5f);
	}
	glEnd();
	
	glDisable(GL_TEXTURE_2D);



	if (krad_sdl_opengl_display->hud_data != NULL) {
		if (zoom_in == 0) {
			//ie zoom in is done or not enabled
			speed = 0.01f;
		}
		// hud display
		glTranslatef(-1.3f, -0.6f, -1.5f);
		glRotatef(9.0 * sinf(k), 0.3f, 0.4f, 0.5f);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 5);
		
		glBegin(GL_QUADS);
		{
		 glTexCoord2f(0.0f, 0.0f);
		 glVertex2f(-hud_aspectRatio * 0.5f, 0.5f); 
		 glTexCoord2f(0.0f, 1.0f);
		 glVertex2f(-hud_aspectRatio  * 0.5f, -0.5f); 
		 glTexCoord2f(1.0f, 1.0f);
		 glVertex2f(hud_aspectRatio  * 0.5f, -0.5f); 
		 glTexCoord2f(1.0f, 0.0f);
		 glVertex2f(hud_aspectRatio  * 0.5f, 0.5f);
		}
		glEnd();
	
		glDisable(GL_TEXTURE_2D);
	}
	
	
	/*
	// transparent blue square thing
	glTranslatef(0.0f, 0.0f, -1.3f);
	
	glColor4f(0.2f,0.8f,1.0f, 0.4f);
		
	glBegin(GL_QUADS);
	{
	 glVertex2f(-aspectRatio * 0.5f, 0.5f); 
	 glVertex2f(-aspectRatio  * 0.5f, -0.5f); 
	 glVertex2f(aspectRatio  * 0.5f, -0.5f); 
	 glVertex2f(aspectRatio  * 0.5f, 0.5f);
	}
	glEnd();
	*/
	//glUseProgram(0);
	//printf("it is %f %f %f %f\n", top, bottom, left, right);

    SDL_GL_SwapBuffers( );

}

void krad_sdl_opengl_shader_init(krad_sdl_opengl_display_t *krad_sdl_opengl_display) {
   
   program_object  = glCreateProgram();    // creating a program object
   vertex_shader   = glCreateShader(GL_VERTEX_SHADER);   // creating a vertex shader object
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER); // creating a fragment shader object
   //printProgramInfoLog(program_object);
   
   glShaderSource(vertex_shader, 1, &vertex_source, NULL); // assigning the vertex source
	glShaderSource(fragment_shader, 1, &fragment_source, NULL); // assigning the fragment source
  // printProgramInfoLog(program_object);   // verifies if all this is ok so far
   
   // compiling and attaching the vertex shader onto program
   glCompileShader(vertex_shader);
   glAttachShader(program_object, vertex_shader); 
  // printProgramInfoLog(program_object);   // verifies if all this is ok so far
   
   // compiling and attaching the fragment shader onto program
	glCompileShader(fragment_shader);
	glAttachShader(program_object, fragment_shader); 
  // printProgramInfoLog(program_object);   // verifies if all this is ok so far
   
   // Link the shaders into a complete GLSL program.
   glLinkProgram(program_object);
   //printProgramInfoLog(program_object);   // verifies if all this is ok so far
   
   // some extra code for checking if all this initialization is ok
   GLint prog_link_success;
   glGetObjectParameterivARB(program_object, GL_OBJECT_LINK_STATUS_ARB, &prog_link_success);
   if (!prog_link_success) {
      fprintf(stderr, "The shaders could not be linked\n");
      exit(1);
   } else {
		printf("Shaders compiled\n");
   }
    
}

void krad_sdl_opengl_init(krad_sdl_opengl_display_t *krad_sdl_opengl_display) {

    glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
	//glHint(GL_POLYGON_SMOOTH, GL_NICEST);

	//glEnable(GL_POLYGON_SMOOTH);
    
    glClearColor( 0, 0, 0, 0 );

    /* Setup our viewport. */
    glViewport( 0, 0, krad_sdl_opengl_display->width, krad_sdl_opengl_display->height );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity( );
 
	//krad_sdl_opengl_shader_init(krad_sdl_opengl_display);

    
}


void krad_sdl_opengl_display_set_input_format(krad_sdl_opengl_display_t *krad_sdl_opengl_display, enum PixelFormat format) {

	if (krad_sdl_opengl_display->sws_context != NULL) {
		sws_freeContext (krad_sdl_opengl_display->sws_context);	
	}

	printf("set format to %d\n", format);

	krad_sdl_opengl_display->sws_context = sws_getContext ( krad_sdl_opengl_display->videowidth, krad_sdl_opengl_display->videoheight, format, krad_sdl_opengl_display->width, krad_sdl_opengl_display->height, PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

}

krad_sdl_opengl_display_t *krad_sdl_opengl_display_create(int width, int height, int videowidth, int videoheight)
{

	krad_sdl_opengl_display_t *krad_sdl_opengl_display;
	
	if ((krad_sdl_opengl_display = calloc (1, sizeof (krad_sdl_opengl_display_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
        exit(1);
	}

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        krad_sdl_opengl_display_destroy(krad_sdl_opengl_display);
        exit(1);
    }

	krad_sdl_opengl_display->width = width;
	krad_sdl_opengl_display->height = height;

	krad_sdl_opengl_display->videowidth = videowidth;
	krad_sdl_opengl_display->videoheight = videoheight;

	krad_sdl_opengl_display->sws_context = sws_getContext ( krad_sdl_opengl_display->videowidth, krad_sdl_opengl_display->videoheight, PIX_FMT_YUV420P, krad_sdl_opengl_display->width, krad_sdl_opengl_display->height, PIX_FMT_RGB32, SWS_BICUBIC, NULL, NULL, NULL);

	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,        8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,      8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,       8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE,      8);
	 
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,      16);
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE,        32);
	 
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS,  1);
	//SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES,  2);

	krad_sdl_opengl_display->video_flags = 0;


	if ((krad_sdl_opengl_display->width == 1920) && (krad_sdl_opengl_display->height == 1080)) {
		krad_sdl_opengl_display->video_flags |= SDL_FULLSCREEN;
	}

	krad_sdl_opengl_display->video_flags |= SDL_HWSURFACE;
	krad_sdl_opengl_display->video_flags |= SDL_OPENGL;
	krad_sdl_opengl_display->video_flags |= SDL_GL_DOUBLEBUFFER;
    krad_sdl_opengl_display->screen = SDL_SetVideoMode(krad_sdl_opengl_display->width, krad_sdl_opengl_display->height, 32, krad_sdl_opengl_display->video_flags);

    if (krad_sdl_opengl_display->screen == NULL) {
        fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n", krad_sdl_opengl_display->width, krad_sdl_opengl_display->height, 32, SDL_GetError());
        exit(1);
    }

    printf("Set%s %dx%dx%d mode\n",
           krad_sdl_opengl_display->screen->flags & SDL_FULLSCREEN ? " fullscreen" : "",
           krad_sdl_opengl_display->screen->w, krad_sdl_opengl_display->screen->h, krad_sdl_opengl_display->screen->format->BitsPerPixel);

    printf("(video surface located in %s memory)\n", (krad_sdl_opengl_display->screen->flags & SDL_HWSURFACE) ? "video" : "system");

    SDL_WM_SetCaption("Krad OpenGL Display", "kradod");
    
	krad_sdl_opengl_init(krad_sdl_opengl_display);

	printf("Krad Display created\n");
    
	pthread_rwlock_init(&krad_sdl_opengl_display->frame_lock, NULL);
	pthread_rwlock_wrlock(&krad_sdl_opengl_display->frame_lock);
	krad_sdl_opengl_display->frame_byte_size = krad_sdl_opengl_display->height * krad_sdl_opengl_display->width * 4;

	krad_sdl_opengl_display->rgb_frame_data = calloc (1, krad_sdl_opengl_display->frame_byte_size);


    krad_sdl_opengl_display->screen = SDL_SetVideoMode(krad_sdl_opengl_display->width, krad_sdl_opengl_display->height, 32, krad_sdl_opengl_display->video_flags);
    
    
    return krad_sdl_opengl_display;

}


void krad_sdl_opengl_display_destroy(krad_sdl_opengl_display_t *krad_sdl_opengl_display) {

	printf("Krad Display Destroy\n");


  	pthread_rwlock_wrlock(&krad_sdl_opengl_display->frame_lock);

	free(krad_sdl_opengl_display->rgb_frame_data);
	
	sws_freeContext (krad_sdl_opengl_display->sws_context);	
     
	pthread_rwlock_unlock(&krad_sdl_opengl_display->frame_lock);
	pthread_rwlock_destroy(&krad_sdl_opengl_display->frame_lock); 




    SDL_Quit();
    
    free(krad_sdl_opengl_display);

}

