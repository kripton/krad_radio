#include "krad_gui.h"
#include "krad_gui_gtk.h"


//the global pixmap that will serve as our buffer
static GdkPixmap *pixmap = NULL;
int size;

gboolean on_window_configure_event(GtkWidget * da, GdkEventConfigure * event, gpointer user_data){
    static int oldw = 0;
    static int oldh = 0;
    //make our selves a properly sized pixmap if our window has been resized
    if (oldw != event->width || oldh != event->height){
        //create our new pixmap with the correct size.
        GdkPixmap *tmppixmap = gdk_pixmap_new(da->window, event->width,  event->height, -1);
        //copy the contents of the old pixmap to the new pixmap.  This keeps ugly uninitialized
        //pixmaps from being painted upon resize
        int minw = oldw, minh = oldh;
        if( event->width < minw ){ minw =  event->width; }
        if( event->height < minh ){ minh =  event->height; }
        gdk_draw_drawable(tmppixmap, da->style->fg_gc[GTK_WIDGET_STATE(da)], pixmap, 0, 0, 0, 0, minw, minh);
        //we're done with our old pixmap, so we can get rid of it and replace it with our properly-sized one.
        g_object_unref(pixmap); 
        pixmap = tmppixmap;
    }
    oldw = event->width;
    oldh = event->height;
    return TRUE;
}

gboolean on_window_expose_event(GtkWidget * da, GdkEventExpose * event, gpointer user_data){
    gdk_draw_drawable(da->window,
        da->style->fg_gc[GTK_WIDGET_STATE(da)], pixmap,
        // Only copy the area that was exposed.
        event->area.x, event->area.y,
        event->area.x, event->area.y,
        event->area.width, event->area.height);
    return TRUE;
}


static int currently_drawing = 0;
//do_draw will be executed in a separate thread whenever we would like to update
//our animation
void *do_draw(void *ptr){


	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)ptr;



    //prepare to trap our SIGALRM so we can draw when we recieve it!
    siginfo_t info;
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);

	int width, height;
	//int count = 0;
	int prepared = 0;

	cairo_surface_t *cst;
	cairo_t *cr;

    while(kradgui_gtk->shutdown == 0){
        //wait for our SIGALRM.  Upon receipt, draw our stuff.  Then, do it again!
        while (sigwaitinfo(&sigset, &info) > 0) {

            currently_drawing = 1;



			if (!(prepared)) {
		        gdk_threads_enter();
		        gdk_drawable_get_size(pixmap, &width, &height);
				kradgui_set_size(kradgui_gtk->kradgui, width, height);
		        gdk_threads_leave();
		        
		        //create a gtk-independant surface to draw on
		        cst = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
		        
				prepared = 1;
		    }
		        cr = cairo_create(cst);

				kradgui_gtk->kradgui->cr = cr;



			

			//kradgui_set_current_track_time_ms(kradgui, 3 * count);
			//kradgui_add_current_track_time_ms(kradgui, 6 * count);	
			//count++;
			kradgui_render(kradgui_gtk->kradgui);


			cairo_destroy(cr);


			/*
            //do some time-consuming drawing
            static int i = 0;
            ++i; i = i % 300;   //give a little movement to our animation
            cairo_set_source_rgb (cr, .9, .9, .9);
            cairo_paint(cr);
            int j,k;
            for(k=0; k<100; ++k){   //lets just redraw lots of times to use a lot of proc power
                for(j=0; j < 1000; ++j){
                    cairo_set_source_rgb (cr, (double)j/1000.0, (double)j/1000.0, 1.0 - (double)j/1000.0);
                    cairo_move_to(cr, i,j/2); 
                    cairo_line_to(cr, i+100,j/2);
                    cairo_stroke(cr);
                }
            }
            
            */

			

            //When dealing with gdkPixmap's, we need to make sure not to
            //access them from outside gtk_main().
            gdk_threads_enter();

            cairo_t *cr_pixmap = gdk_cairo_create(pixmap);
            cairo_set_source_surface (cr_pixmap, cst, 0, 0);
            cairo_paint(cr_pixmap);
            cairo_destroy(cr_pixmap);

            gdk_threads_leave();

            currently_drawing = 0;

        }
    }
    
	cairo_surface_destroy(cst);
    
	cairo_destroy(cr);
    
}


void *do_draw_alt (void *ptr) {


	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)ptr;
	
	//kradgui_add_item(kradgui, REEL_TO_REEL);
	//kradgui_add_item(kradgui, PLAYBACK_STATE_STATUS);
	//kradgui_set_total_track_time_ms(kradgui, 5 * 60 * 1000);

    //prepare to trap our SIGALRM so we can draw when we recieve it!
    siginfo_t info;
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);

	//int count = 0;

    while (kradgui_gtk->shutdown == 0){
        //wait for our SIGALRM.  Upon receipt, draw our stuff.  Then, do it again!
        while (sigwaitinfo(&sigset, &info) > 0) {

            currently_drawing = 1;

            int width, height;
            gdk_threads_enter();
            gdk_drawable_get_size(pixmap, &width, &height);
			kradgui_set_size(kradgui_gtk->kradgui, width, height);
            gdk_threads_leave();

            //create a gtk-independant surface to draw on
            cairo_surface_t *cst = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
            cairo_t *cr = cairo_create(cst);

			kradgui_gtk->kradgui->cr = cr;

			//kradgui_set_current_track_time_ms(kradgui, 3 * count);
			//kradgui_add_current_track_time_ms(kradgui, 6 * count);

			kradgui_render(kradgui_gtk->kradgui);

			/*
            //do some time-consuming drawing
            static int i = 0;
            ++i; i = i % 300;   //give a little movement to our animation
            cairo_set_source_rgb (cr, .9, .9, .9);
            cairo_paint(cr);
            int j,k;
            for(k=0; k<100; ++k){   //lets just redraw lots of times to use a lot of proc power
                for(j=0; j < 1000; ++j){
                    cairo_set_source_rgb (cr, (double)j/1000.0, (double)j/1000.0, 1.0 - (double)j/1000.0);
                    cairo_move_to(cr, i,j/2); 
                    cairo_line_to(cr, i+100,j/2);
                    cairo_stroke(cr);
                }
            }
            
            */
            cairo_destroy(cr);
			

            //When dealing with gdkPixmap's, we need to make sure not to
            //access them from outside gtk_main().
            gdk_threads_enter();

            cairo_t *cr_pixmap = gdk_cairo_create(pixmap);
            cairo_set_source_surface (cr_pixmap, cst, 0, 0);
            cairo_paint(cr_pixmap);
            cairo_destroy(cr_pixmap);

            gdk_threads_leave();

            cairo_surface_destroy(cst);

            currently_drawing = 0;

		    if (kradgui_gtk->shutdown == 1) {
		    	break;
		    }

        }
    }
    
    	printf("bye bye1\n");
    
}


void *do_draw_alt2 (void *ptr) {


	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)ptr;
	
	
	//kradgui_add_item(kradgui, REEL_TO_REEL);
	//kradgui_set_total_track_time_ms(kradgui, 5 * 60 * 1000);

    //prepare to trap our SIGALRM so we can draw when we recieve it!
    siginfo_t info;
    sigset_t sigset;

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);

	int width, height;
	//int count = 0;
	int prepared = 0;

	cairo_t *cr_pixmap = gdk_cairo_create(pixmap);
	cairo_surface_t *cst;

        //wait for our SIGALRM.  Upon receipt, draw our stuff.  Then, do it again!
        while (sigwaitinfo(&sigset, &info) > 0) {

            currently_drawing = 1;

			if (!(prepared)) {
			    gdk_threads_enter();
            	gdk_drawable_get_size(pixmap, &width, &height);
				kradgui_set_size(kradgui_gtk->kradgui, width, height);
            	gdk_threads_leave();
				cst = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
				prepared = 1;
			}

            //create a gtk-independant surface to draw on
            //cairo_surface_t *cst = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
            cairo_t *cr = cairo_create(cst);

			kradgui_gtk->kradgui->cr = cr;

			kradgui_gtk->kradgui->size = size;
			//kradgui_set_current_track_time_ms(kradgui, 3 * count);
			//kradgui_add_current_track_time_ms(kradgui, 6 * count);
			//count++;
			kradgui_render(kradgui_gtk->kradgui);

			/*
            //do some time-consuming drawing
            static int i = 0;
            ++i; i = i % 300;   //give a little movement to our animation
            cairo_set_source_rgb (cr, .9, .9, .9);
            cairo_paint(cr);
            int j,k;
            for (k=0; k<100; ++k) {   //lets just redraw lots of times to use a lot of proc power
                for(j=0; j < 1000; ++j){
                    cairo_set_source_rgb (cr, (double)j/1000.0, (double)j/1000.0, 1.0 - (double)j/1000.0);
                    cairo_move_to(cr, i,j/2); 
                    cairo_line_to(cr, i+100,j/2);
                    cairo_stroke(cr);
                }
            }
            
            */
            cairo_destroy(cr);
			

            //When dealing with gdkPixmap's, we need to make sure not to
            //access them from outside gtk_main().
            gdk_threads_enter();

            //cairo_t *cr_pixmap = gdk_cairo_create(pixmap);
            cairo_set_source_surface (cr_pixmap, cst, 0, 0);
            cairo_paint(cr_pixmap);
            //cairo_destroy(cr_pixmap);

            gdk_threads_leave();



            currently_drawing = 0;

        if (kradgui_gtk->shutdown == 1) {
        	break;
        }
    }


	printf("bye bye\n");

	//cairo_destroy(cr_pixmap);
	cairo_surface_destroy(cst);
    
}



gboolean timer_exe(kradgui_gtk_t *kradgui_gtk) {


    static int first_time = 1;
    //use a safe function to get the value of currently_drawing so
    //we don't run into the usual multithreading issues
    int drawing_status = g_atomic_int_get(&currently_drawing);

    //if this is the first time, create the drawing thread

    if(first_time == 1){
        int  iret;
        iret = pthread_create( &kradgui_gtk->drawing_thread, NULL, do_draw_alt, kradgui_gtk);
        iret += 1; // just to kill a warning, does nothing
    }

	if (kradgui_gtk->shutdown == 1) {
	
		if (drawing_status == 0) {
		    pthread_kill(kradgui_gtk->drawing_thread, SIGALRM);
		}
	
		while (drawing_status == 1) {
			usleep(5000);
			drawing_status = g_atomic_int_get(&currently_drawing);
		}
		    	
		printf("hi me \n");
	
		pthread_join(kradgui_gtk->drawing_thread, NULL);
		printf("hi me2 \n");
			gtk_main_quit();
		return FALSE;
	
	}


    //if we are not currently drawing anything, send a SIGALRM signal
    //to our thread and tell it to update our pixmap
    if(drawing_status == 0){
        pthread_kill(kradgui_gtk->drawing_thread, SIGALRM);
    }

    //tell our window it is time to draw our animation.
    int width, height;
    gdk_drawable_get_size(pixmap, &width, &height);
    gtk_widget_queue_draw_area(kradgui_gtk->window, 0, 0, width, height);


    first_time = 0;
    return TRUE;

}

void kradgui_gtk_start(kradgui_t *kradgui) {


	kradgui_gtk_t *kradgui_gtk = calloc(1, sizeof(kradgui_gtk_t));

	kradgui->gui_ptr = kradgui_gtk;
	kradgui_gtk->kradgui = kradgui;

	kradgui_gtk->width = kradgui->width;
	kradgui_gtk->height = kradgui->height;
	
	pthread_create( &kradgui_gtk->gui_thread, NULL, kradgui_gtk_init, kradgui_gtk);

}

void kradgui_gtk_end(kradgui_t *kradgui) {

	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)kradgui->gui_ptr;

	kradgui_gtk->shutdown = 1;


	printf("hi1\n");

	pthread_join(kradgui_gtk->gui_thread, NULL);

	printf("hi2\n");

	free(kradgui->gui_ptr);
	kradgui->gui_ptr = NULL;
	
	
	printf("gtk stuff finni\n");
	
	
	
}


static void
cb_speed_scale_changed( GtkWidget *widget,
            gpointer     data )
{

	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)data;
 

	kradgui_control_speed(kradgui_gtk->kradgui, gtk_range_get_value(GTK_RANGE(widget)));

}

static void speed_up( GtkWidget *widget,
                   gpointer   data )
{

	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)data;

	kradgui_control_speed_up(kradgui_gtk->kradgui);
}

static void speed_down( GtkWidget *widget,
                   gpointer   data )
{

	kradgui_gtk_t *kradgui_gtk = (kradgui_gtk_t *)data;

	kradgui_control_speed_down(kradgui_gtk->kradgui);
}

void *kradgui_gtk_init(kradgui_gtk_t *kradgui_gtk) {


	int update_time;
	
	update_time = 1000 / 40;

    //Block SIGALRM in the main thread
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    pthread_sigmask(SIG_BLOCK, &sigset, NULL);

    //we need to initialize all these functions so that gtk knows
    //to be thread-aware
    if (!g_thread_supported ()){ g_thread_init(NULL); }
    gdk_threads_init();
    gdk_threads_enter();

    //gtk_init(&argc, &argv);

    gtk_init(NULL, NULL);

    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(G_OBJECT(window), "expose_event", G_CALLBACK(on_window_expose_event), NULL);
    g_signal_connect(G_OBJECT(window), "configure_event", G_CALLBACK(on_window_configure_event), NULL);

    //this must be done before we define our pixmap so that it can reference
    //the colour depth and such
    gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    gtk_window_set_default_size(GTK_WINDOW(window), kradgui_gtk->width, kradgui_gtk->height); 
    gtk_window_set_decorated (GTK_WINDOW(window), FALSE);
    gtk_window_set_has_resize_grip (GTK_WINDOW(window), FALSE);
    gtk_widget_show_all(window);

    //set up our pixmap so it is ready for drawing
    pixmap = gdk_pixmap_new(window->window, kradgui_gtk->width, kradgui_gtk->height,-1);
    //because we will be painting our pixmap manually during expose events
    //we can turn off gtk's automatic painting and double buffering routines.
    gtk_widget_set_app_paintable(window, TRUE);
    gtk_widget_set_double_buffered(window, FALSE);


	kradgui_gtk->window = window;


    (void)g_timeout_add(update_time, (GSourceFunc)timer_exe, kradgui_gtk);




 	GtkWidget *window2;
    GtkWidget *button;
    GtkWidget *button2;
GtkWidget *box1;
GtkWidget *speed_scale;

  /* create a new window */
    window2 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    

    
    /* Sets the border width of the window. */
    gtk_container_set_border_width (GTK_CONTAINER (window2), 10);
    
    
 	 box1 = gtk_hbox_new (FALSE, 0);

    /* Put the box into the main window. */
    gtk_container_add (GTK_CONTAINER (window2), box1);
    
/*
    /* Creates a new button with the label "Hello World". 
    button = gtk_button_new_with_label ("Speed Up");
    
    /* When the button receives the "clicked" signal, it will call the
     * function hello() passing it NULL as its argument.  The hello()
     * function is defined above. 
    g_signal_connect (button, "clicked",
		      G_CALLBACK (speed_up), kradgui_gtk);
    
    /* This will cause the window to be destroyed by calling
     * gtk_widget_destroy(window) when "clicked".  Again, the destroy
     * signal could come from here, or the window manager. 
    g_signal_connect_swapped (button, "clicked",
			      G_CALLBACK (gtk_widget_destroy),
                              window2);
    */
    /* This packs the button into the window (a gtk container). 
	gtk_box_pack_start (GTK_BOX(box1), button, TRUE, TRUE, 0);
    
    /* The final step is to display this newly created widget. 

 
 
 
 
 
    
    /* Creates a new button with the label "Hello World". */
    button2 = gtk_button_new_with_label ("Speed Down");
    
    /* When the button receives the "clicked" signal, it will call the
     * function hello() passing it NULL as its argument.  The hello()
     * function is defined above. 
    g_signal_connect (button2, "clicked",
		      G_CALLBACK (speed_down), kradgui_gtk);
    
    /* This will cause the window to be destroyed by calling
     * gtk_widget_destroy(window) when "clicked".  Again, the destroy
     * signal could come from here, or the window manager. 
    g_signal_connect_swapped (button, "clicked",
			      G_CALLBACK (gtk_widget_destroy),
                              window2);
    */
    /* This packs the button into the window (a gtk container). 
	gtk_box_pack_start (GTK_BOX(box1), button2, TRUE, TRUE, 0);
    
    /* The final step is to display this newly created widget. 
    gtk_widget_show (button);
    gtk_widget_show (button2);
    */
    
    
	speed_scale = gtk_hscale_new_with_range(1.0, 700.0, 1.0);
	gtk_range_set_value(GTK_RANGE(speed_scale), 100.0);

	gtk_box_set_spacing(GTK_BOX( box1 ), 10);

	gtk_container_add( GTK_CONTAINER( box1 ), speed_scale );
	gtk_scale_set_value_pos(GTK_SCALE(speed_scale), GTK_POS_LEFT);
	gtk_scale_set_digits (GTK_SCALE(speed_scale), 0);
	//gtk_range_set_round_digits (GTK_RANGE(kradlink->gtk->buffer_scale), 1);
    g_signal_connect( GTK_RANGE( speed_scale ), "value-changed",
                      G_CALLBACK( cb_speed_scale_changed ), kradgui_gtk );
    
    
        gtk_widget_show (speed_scale);
    
       gtk_widget_show (box1);
    /* and the window */
    
     
   
   gtk_window_move (window2, gdk_screen_width() - 1480, gdk_screen_height() - 250);
   
    gtk_window_set_default_size(GTK_WINDOW(window2), 1280, 200); 
    gtk_window_set_decorated (GTK_WINDOW(window2), FALSE);
    gtk_window_set_has_resize_grip (GTK_WINDOW(window2), FALSE);
    
    
    gtk_widget_show (window2);






    gtk_main();
    gdk_threads_leave();

    return NULL;
}


