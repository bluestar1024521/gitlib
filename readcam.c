#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <malloc.h>
#include <sys/mman.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <time.h>
#define COLS 640
#define ROWS 480

int fd;
guchar rgbbuf[COLS*ROWS*3];

struct
{
	guchar *start;
 	size_t length;
}*buffers;

GtkWidget *drawarea;
unsigned f = 0;

int on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{ 
	munmap(buffers[0].start, buffers[0].length);
 	free(buffers);
    close(fd); 
    gtk_main_quit();
 	return FALSE;
}

int draw_image(GtkWidget *widget, GdkEvent *event, gpointer data)
{ 
	gdk_draw_rgb_image(widget->window, widget->style->fg_gc[GTK_STATE_NORMAL], 0, 0, COLS, ROWS, GDK_RGB_DITHER_NONE, rgbbuf, COLS*3);
	return FALSE;
}

void YUY2_RGB4(guchar *YUY2buff, guchar *RGBbuff, int Width, int Height)
{
	guchar *orgRGBbuff = RGBbuff;
	int i, j;
	for(i=0; i<Height; ++i)
	{ 
		for(j=0; j<Width; j+=2)
		{          
		    //Y0 U0 Y1 V0
			float Y0 = *YUY2buff++;
			float U = *YUY2buff++;
			float Y1 = *YUY2buff++;
 			float V = *YUY2buff++;

			float R, G, B;
  
			R = 1.164f*(Y0-16) + 1.159f*(V-128);
			G = 1.164f*(Y0-16) - 0.38f*(U-128) - 0.813f*(V-128);  
			B = 1.164f*(Y0-16) + 2.018f*(U-128);
			if(R<0) 
			{
				R = 0;
			} 
			if(R>255) 
			{
				R = 255;
			} 
			if(G<0) 
			{
				G = 0;
			} 
			if(G>255) 
			{
				G = 255;
			}
			if(B<0)
			{
				B = 0;
			}
			if(B>255) 
			{
				B = 255;
			}

			*orgRGBbuff++ = (guchar)R;         
			*orgRGBbuff++ = (guchar)G;
			*orgRGBbuff++ = (guchar)B;
  
			R = 1.164f*(Y1-16) + 1.159f*(V-128);
			G = 1.164f*(Y1-16) - 0.38f*(U-128) - 0.813f*(V-128);  
			B = 1.164f*(Y1-16) + 2.018f*(U-128);
			if(R<0) 
			{
				R = 0;
			}
			if(R>255) 
			{
				R = 255;
			}
			if(G<0)
 			{
				G = 0;
 			}				 
			if(G>255) 
			{
				G = 255;
			}
			if(B<0) 
			{
				B = 0;
			}
  			if(B>255) 
			{
				B = 255;
			}

			*orgRGBbuff++ = (guchar)R;         
			*orgRGBbuff++ = (guchar)G;
			*orgRGBbuff++ = (guchar)B;
		}
	}
}

int read_data(GtkWidget *widget, GdkEvent *event, gpointer data)
{    
	struct v4l2_buffer buf;
   	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
 
	if(ioctl(fd, VIDIOC_DQBUF, &buf) == -1)
	{
		printf("Dqbuf failed\n");
	}
	
	// process and draw image
	YUY2_RGB4(buffers[buf.index].start, rgbbuf, COLS, ROWS);
	gdk_threads_enter();
	gdk_draw_rgb_image(drawarea->window, drawarea->style->fg_gc[GTK_STATE_NORMAL], 0, 0, COLS, ROWS, GDK_RGB_DITHER_NONE, rgbbuf, COLS*3);
	gdk_threads_leave();

	f %= 100000;
	printf("The number now is :%d\n", ++f);
	//go to qbuf
	if(ioctl(fd, VIDIOC_QBUF,&buf) == -1)
	{
		printf("Go to qbuf failed\n");
	}
    	
	return TRUE;
}

int main(int argc, char *argv[])
{
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	struct v4l2_requestbuffers reqbuf;
  	enum v4l2_buf_type type;
	// open the video
  	fd = open("/dev/video0", O_RDWR);
  	if(fd<0)
   	{
		printf("Can't open it!\n");
		fd = open("/dev/video1", O_RDWR);
		if(fd>0) 
		{
			printf("Open video1\n");
		}
	}
	else
    {
    	printf("Successfull!\n");
    } 

	// get the capability of the video
  	if(ioctl(fd, VIDIOC_QUERYCAP, &cap) == -1)  
 	{
	 	printf("Can't get cap!\n");
 	}
  	if(cap.capabilities&V4L2_CAP_VIDEO_CAPTURE) 
 	{
	 	printf("It is a video caprure device!\n");
 	}
  	if(cap.capabilities&V4L2_CAP_STREAMING) 
  	{
	  	printf("It support streaming i/0\n");
  	}
  	if(cap.capabilities&V4L2_CAP_READWRITE) 
  	{
	  	printf("It support read and write i/o\n");
  	}
	// set up the image format
   	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   	fmt.fmt.pix.width = COLS;
   	fmt.fmt.pix.height = ROWS;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	if ( ioctl(fd,VIDIOC_S_FMT,& fmt) == -1) 
	{
		printf ("format failed\n");
	}
    else
 	{
	 	printf("This format is allowed!\n");
 	}
	// set up the numbers of driver buffer
   	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   	reqbuf.count = 30;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	if(ioctl(fd, VIDIOC_REQBUFS, & reqbuf) == -1) 
	{
		printf ("Buffer request error\n"); 
	}
	//allocate the approcation buffers 
	buffers = calloc (reqbuf.count, sizeof(*buffers));
	if(!buffers) 
	{
		printf("Buffers out of memory\n");
	}

	//map the buffers between driver and approcation
	unsigned int i;
	for(i=0; i<reqbuf.count; i++)
	{
		struct v4l2_buffer buffer;
        buffer.type = reqbuf.type;
		buffer.memory = V4L2_MEMORY_MMAP;
       	buffer.index = i;
        
      	if(ioctl(fd, VIDIOC_QUERYBUF, &buffer) == -1)
		{
			printf("querybuf failed\n");
		}
       	else
		{
			printf("Querybuf successlly!\n");
		} 
		
      	buffers[i].length = buffer.length;     /* remember for munmap() */
       	buffers[i].start = mmap(NULL, buffer.length,
                                PROT_READ | PROT_WRITE,   /* recommended */
                                MAP_SHARED,                /* recommended */
                                fd, buffer.m.offset);
        if(MAP_FAILED == buffers[i].start)
        {
        	printf ("Mmap failde\n");
        }
		else 
		{
			printf("Mmap it successlly!\n");
		}
	}

	// start capture:enqueue all the buffers! 
	int a;
	for(a=0; a<reqbuf.count; a++)
  	{ 
		struct v4l2_buffer buf;
    	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    	buf.memory = V4L2_MEMORY_MMAP;
    	buf.index = a;
   
   		if(ioctl(fd, VIDIOC_QBUF,&buf) == -1)
   		{
		   	 printf("Enqueue buffers error\n");
   		} 
  	}

	// Stream on the buffers,begin to capture
   	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
   	if (-1 == ioctl(fd, VIDIOC_STREAMON, &type))
   	{ 
   		printf("Stream on error\n");
   	} 
  
	//Read a frame to expose
    struct v4l2_buffer buf;
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    
  	if(ioctl(fd, VIDIOC_DQBUF, &buf) == -1)
    {
		printf("Dqbuf failed\n");
	} 
  	if(buf.index >= reqbuf.count)
  	{ 
    	printf("idex wrong\n");
  	} 
	// process image
	YUY2_RGB4(buffers[buf.index].start, rgbbuf, COLS, ROWS);
	// goto qbuf
	if(ioctl(fd, VIDIOC_QBUF,&buf) == -1)
	{
		printf("Go to qbuf failed\n");
	}
	
	printf("Init and read a frame!\n");

	/* init Gtk */
 	GtkWidget *window;
 
	g_thread_init(NULL);
	gdk_threads_init();
	gdk_threads_enter();
	gtk_init(&argc, &argv);
	gdk_init(&argc, &argv);


	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "PIVOT");
	g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(on_delete_event), NULL);

	gtk_widget_push_visual(gdk_rgb_get_visual());
	gtk_widget_push_colormap(gdk_rgb_get_cmap());
	drawarea = gtk_drawing_area_new();
	gtk_widget_pop_visual();
	gtk_widget_pop_colormap();
	gtk_container_add(GTK_CONTAINER(window), drawarea);

	g_signal_connect(G_OBJECT(drawarea), "expose_event", G_CALLBACK(draw_image), NULL);
	gtk_widget_set_size_request(GTK_WIDGET(drawarea), COLS,ROWS);

	guint id = gtk_idle_add((GtkFunction)read_data, NULL);

	gtk_widget_show_all(window);
	gtk_main();
	gdk_threads_leave();

 	return 0;
}
