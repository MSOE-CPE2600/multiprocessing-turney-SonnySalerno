/*******************************************************************
* Filename: mandelmovie.c
* Description: envolks mandel for generating frames to create 
* fractal movie with different child processes and threads.
* Author: Sonny Salerno (Modified for Lab 12)
* Date: 11/24/2025
* Note: make clean, make, 
* ./mandelmovie -x -.3678 -y .64988 -s .05 -m 6000 -n 12 -f 240 -t 4, 
* ffmpeg -i -framerate 60 mandel%d.jpg mandel.mpg
*********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <string.h>

// Prints command line help
void show_help() {
	printf("Use: mandelmovie [options]\n");
	printf("Where options are:\n");
	printf("-n <child>  Number of processes (default=1)\n");
    printf("-f <frames> Number of frames (default=50)\n");
	printf("-m <max>    The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>  X coordinate of image center point. (default=0)\n");
	printf("-y <coord>  Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>  Scale of the image in Mandlebrot coordinates (X-axis). (default=4)\n");
	printf("-W <pixels> Width of the image in pixels. (default=1000)\n");
	printf("-H <pixels> Height of the image in pixels. (default=1000)\n");
	printf("-o <file>   Set output file. (default=mandel.bmp)\n");
    printf("-t <threads> Number of threads per process. (default=1)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}

// Scales value for specific frame index reducing with multiplyer for each frame
double scale_at(int index, double start, double reduction) {
    double v = start;
    for (int k = 0; k < index; k++)
        v *= reduction; // Shrinks the scale for zooming effect
    return v;
}

// Launches a single mandelbrot image using execv() to envlok mandel
pid_t launch_mandel(const char *mandel_path, const char *foutfile, double xcenter, 
	double ycenter, double xscale, int image_width, int image_height, int max, int threads) {
        
    pid_t pid = fork(); // Creates a new child
    if (pid < 0) {
        printf("fork failed\n");
        return -1;
    }

    if (pid == 0) {
        // Converts args that are numbers into strings
        char sx[64], sy[64], ss[64], sw[32], sh[32], sm[32], st[32];

        snprintf(sx, sizeof(sx), "%g", xcenter);
        snprintf(sy, sizeof(sy), "%g", ycenter);
        snprintf(ss, sizeof(ss), "%g", xscale);
        snprintf(sw, sizeof(sw), "%d", image_width);
        snprintf(sh, sizeof(sh), "%d", image_height);
        snprintf(sm, sizeof(sm), "%d", max);
        snprintf(st, sizeof(st), "%d", threads);

        // Builds the arg list to envolk mandel
        // IMPORTANT: Increased array size to accommodate new -t args
        char *args[20];
        int i = 0;
        args[i++] = (char *)mandel_path;
        args[i++] = "-x"; 
		args[i++] = sx;
        args[i++] = "-y"; 
		args[i++] = sy;
        args[i++] = "-s"; 
		args[i++] = ss;
        args[i++] = "-W"; 
		args[i++] = sw;
        args[i++] = "-H"; 
		args[i++] = sh;
        args[i++] = "-m"; 
		args[i++] = sm;
        args[i++] = "-o"; 
		args[i++] = (char *)foutfile;
        args[i++] = "-t";
        args[i++] = st;
        args[i] = NULL;

        execv(mandel_path, args);   // Replaces processed image with mandel

        // If execv fails
        printf("ERROR: exec failed\n");
        _exit(127);	// Used to handel if execv fails
    }
    // Parent returns childs PID
    return pid;
}

int main(int argc, char *argv[]) {
    char c;

    int num_children = 1;   // Defaults 1 process
    int frames = 50;        // Defaults 50 frames
	double reduction = 0.95;
    int num_threads = 1;    // Defaults 1 thread


	const char *outfile = "mandel";
	double xcenter = 0.0;
	double ycenter = 0.0;
	double xscale = 4.0;
	int    image_width = 1000;
	int    image_height = 1000;
	int    max = 1000;

    // Exec time
	double time;
    struct timeval tstart;
	struct timeval tend;
    // Starts time
    gettimeofday(&tstart, NULL);

    // Parsing command line
    // Added 't' to optstring
    while ((c = getopt(argc, argv, "n:f:x:y:s:W:H:m:o:h:t:")) != -1) {
        switch (c) {
            case 'n':
				num_children = atoi(optarg);
				break;
            case 'f':
				frames = atoi(optarg);
				break;
            case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				xscale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'o':
				outfile = optarg;
				break;
            case 't': // Parse thread count
                num_threads = atoi(optarg);
                break;
			case 'h':
				show_help();
				exit(1);
				break;
        }
    }

    if (num_children < 1)
		num_children = 1;
    if (frames < 1) 
		frames = 1;
    // Cap threads to reasonable limit or strict 20 if desired
    if (num_threads < 1) num_threads = 1;
    
    const char *mandel_path = "./mandel";
    
    // Loops over each frame 
    int running = 0;    // Number of active children
    for (int i = 0; i < frames; i++) {
        while (running >= num_children) {
            pid_t done = wait(NULL);
            if (done > 0)
                running--;
        }
        // Gets the scale for frame
        double scale = scale_at(i, xscale, reduction);
        // Creates output
        char foutfile[128];
        snprintf(foutfile, sizeof(foutfile), "%s%d.jpg", outfile, i);
        // Launches child process
        pid_t p = launch_mandel(mandel_path, foutfile, xcenter, ycenter, scale, 
			image_width, image_height, max, num_threads);
        
        if (p < 0) {
            printf("failed launching child\n");
            while (wait(NULL) > 0) {}
            return 1;
        }
        running++;
        printf("Launched frame %d (pid %d)\n", i, (int)p);
    }
    // Waits for remaining children
    while (running > 0) {
        pid_t done = wait(NULL);
        if (done > 0)
            running--;
    }
    // Stops timer
    gettimeofday(&tend, NULL);
    // Calcs exe time
	time = (tend.tv_sec - tstart.tv_sec) + 
		(tend.tv_usec - tstart.tv_usec) / 1000000.0;
        
    printf("mandelmovie: children=%d threads=%d frames=%d\n", num_children, num_threads, frames);
    printf("All %d frames completed in %.3f seconds.\n", frames, time);
    return 0;
}