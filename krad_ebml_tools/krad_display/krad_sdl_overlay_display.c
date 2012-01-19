#include <krad_sdl_overlay_display.h>



void krad_overlay_display_render_rgb(krad_overlay_display_t *krad_overlay_display, unsigned char *pRGBData, int src_w, int src_h) {

	int rgb_stride_arr[3] = {4*src_w, 0, 0};
	const uint8_t *rgb_arr[4];

	unsigned char *dst[4];
	int dst_stride_arr[4];
	
	rgb_arr[0] = pRGBData;


	dst_stride_arr[0] = krad_overlay_display->overlay->pitches[0];
	dst_stride_arr[1] = krad_overlay_display->overlay->pitches[1];
	dst_stride_arr[2] = krad_overlay_display->overlay->pitches[2];

	dst[0] = krad_overlay_display->overlay->pixels[0];
	dst[1] = krad_overlay_display->overlay->pixels[2];
	dst[2] = krad_overlay_display->overlay->pixels[1];
	
    SDL_LockYUVOverlay(krad_overlay_display->overlay);
    
    sws_scale(krad_overlay_display->sws_context, rgb_arr, rgb_stride_arr, 0, src_h, dst, dst_stride_arr);
    
	SDL_UnlockYUVOverlay(krad_overlay_display->overlay);
	SDL_DisplayYUVOverlay(krad_overlay_display->overlay, &krad_overlay_display->rect);
    
}

void krad_overlay_display_render(krad_overlay_display_t *krad_overlay_display, unsigned char *y, int ys, unsigned char *u, int us, unsigned char *v, int vs) {

    SDL_LockYUVOverlay(krad_overlay_display->overlay);

	int h;
	h = 0;
	
	while (h < krad_overlay_display->overlay->h) {
	
		memcpy(krad_overlay_display->overlay->pixels[0] + (krad_overlay_display->overlay->pitches[0] * h), y + (h * ys), krad_overlay_display->overlay->pitches[0]);
		h++;
	}
	
	
	h = 0;
	while (h < (krad_overlay_display->overlay->h / 2)) {
		memcpy(krad_overlay_display->overlay->pixels[2] + (krad_overlay_display->overlay->pitches[2] * h), u + (h * us), krad_overlay_display->overlay->pitches[2]);
		memcpy(krad_overlay_display->overlay->pixels[1] + (krad_overlay_display->overlay->pitches[1] * h), v + (h * vs), krad_overlay_display->overlay->pitches[1]);
		h++;
	}

	SDL_UnlockYUVOverlay(krad_overlay_display->overlay);
	SDL_DisplayYUVOverlay(krad_overlay_display->overlay, &krad_overlay_display->rect);
}


void krad_overlay_display_test(krad_overlay_display_t *krad_overlay_display) {

	int x, y;
	
	x = 80;
	y = 80;

    SDL_LockYUVOverlay(krad_overlay_display->overlay);
	
	memset(krad_overlay_display->overlay->pixels[0],0,krad_overlay_display->overlay->pitches[0]*krad_overlay_display->overlay->h);
	memset(krad_overlay_display->overlay->pixels[1],128,krad_overlay_display->overlay->pitches[1]*((krad_overlay_display->overlay->h+1)/2));
	memset(krad_overlay_display->overlay->pixels[2],128,krad_overlay_display->overlay->pitches[2]*((krad_overlay_display->overlay->h+1)/2));
	
	*(krad_overlay_display->overlay->pixels[0] + y * krad_overlay_display->overlay->pitches[0] + x) = 33;
	*(krad_overlay_display->overlay->pixels[1] + y/2 * krad_overlay_display->overlay->pitches[1] + x/2) = 22;
	*(krad_overlay_display->overlay->pixels[2] + y/2 * krad_overlay_display->overlay->pitches[2] + x/2) = 44; 

    SDL_UnlockYUVOverlay(krad_overlay_display->overlay);
 
	SDL_DisplayYUVOverlay(krad_overlay_display->overlay, &krad_overlay_display->rect);
    
}

krad_overlay_display_t *krad_overlay_display_create(int displaywidth, int displayheight, int width, int height)
{

    int i;

	krad_overlay_display_t *krad_overlay_display;
	
	if ((krad_overlay_display = calloc (1, sizeof (krad_overlay_display_t))) == NULL) {
		fprintf(stderr, "mem alloc fail\n");
        exit(1);
	}

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Couldn't initialize SDL: %s\n", SDL_GetError());
        krad_overlay_display_destroy(krad_overlay_display);
        exit(1);
    }

	krad_overlay_display->width = width;
	krad_overlay_display->height = height;

	krad_overlay_display->displaywidth = displaywidth;
	krad_overlay_display->displayheight = displayheight;

	krad_overlay_display->rect.x = 0;
	krad_overlay_display->rect.y = 0;
	krad_overlay_display->rect.w = displaywidth;
	krad_overlay_display->rect.h = displayheight;
    
    krad_overlay_display->colormode = 422;
    
    if (krad_overlay_display->colormode == 420) {
    	krad_overlay_display->overlay_format = SDL_YV12_OVERLAY;
    } else {
		krad_overlay_display->overlay_format = SDL_YUY2_OVERLAY;
	}
	
	krad_overlay_display->video_flags = 0;

	krad_overlay_display->video_flags |= SDL_FULLSCREEN;
	krad_overlay_display->video_flags |= SDL_HWSURFACE;

    krad_overlay_display->screen = SDL_SetVideoMode(krad_overlay_display->displaywidth, krad_overlay_display->displayheight, 32, krad_overlay_display->video_flags);

    if (krad_overlay_display->screen == NULL) {
        fprintf(stderr, "Couldn't set %dx%dx%d video mode: %s\n", krad_overlay_display->displaywidth, krad_overlay_display->displayheight, 32, SDL_GetError());
        exit(1);
    }

    printf("Set%s %dx%dx%d mode\n",
           krad_overlay_display->screen->flags & SDL_FULLSCREEN ? " fullscreen" : "",
           krad_overlay_display->screen->w, krad_overlay_display->screen->h, krad_overlay_display->screen->format->BitsPerPixel);

    printf("(video surface located in %s memory)\n", (krad_overlay_display->screen->flags & SDL_HWSURFACE) ? "video" : "system");

    SDL_WM_SetCaption("Krad Overlay Display", "kradod");
    
    /* Create the overlay */
    krad_overlay_display->overlay = SDL_CreateYUVOverlay(krad_overlay_display->width, krad_overlay_display->height, krad_overlay_display->overlay_format, krad_overlay_display->screen);
    
    if (krad_overlay_display->overlay == NULL) {
        fprintf(stderr, "Couldn't create overlay: %s\n", SDL_GetError());
        exit(1);
    }
    
    printf("Created %dx%dx%d %s %s overlay\n", krad_overlay_display->overlay->w, krad_overlay_display->overlay->h,
           krad_overlay_display->overlay->planes, krad_overlay_display->overlay->hw_overlay ? "hardware" : "software",
           krad_overlay_display->overlay->format == SDL_YV12_OVERLAY ? "YV12" : krad_overlay_display->overlay->format ==
           SDL_IYUV_OVERLAY ? "IYUV" : krad_overlay_display->overlay->format ==
           SDL_YUY2_OVERLAY ? "YUY2" : krad_overlay_display->overlay->format ==
           SDL_UYVY_OVERLAY ? "UYVY" : krad_overlay_display->overlay->format ==
           SDL_YVYU_OVERLAY ? "YVYU" : "Unknown");

    for (i = 0; i < krad_overlay_display->overlay->planes; i++) {
        printf("  plane %d: pitch=%d\n", i, krad_overlay_display->overlay->pitches[i]);
    }
    
	printf("Krad Display created\n");
 
 
    if (krad_overlay_display->colormode == 420) {
		krad_overlay_display->sws_context = sws_getContext ( krad_overlay_display->width, krad_overlay_display->height, PIX_FMT_RGB32, krad_overlay_display->displaywidth, krad_overlay_display->displayheight, PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);    
	} else {
		krad_overlay_display->sws_context = sws_getContext ( krad_overlay_display->width, krad_overlay_display->height, PIX_FMT_RGB32, krad_overlay_display->displaywidth, krad_overlay_display->displayheight, PIX_FMT_YUYV422, SWS_BILINEAR, NULL, NULL, NULL);	
	}

	krad_overlay_display->screen = SDL_SetVideoMode(krad_overlay_display->displaywidth, krad_overlay_display->displayheight, 32, krad_overlay_display->video_flags);
    
    return krad_overlay_display;

}


void krad_overlay_display_destroy(krad_overlay_display_t *krad_overlay_display) {

	printf("Krad Display Destroy\n");

	sws_freeContext (krad_overlay_display->sws_context);	

    SDL_FreeYUVOverlay(krad_overlay_display->overlay);

    SDL_Quit();
    
    free(krad_overlay_display);

}

