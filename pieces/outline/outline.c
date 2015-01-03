#include <pnm.h>
#include <math.h>
#include <pgm.h>
#include <stdio.h>
#include <getopt.h>

gray ** image;
int width, height;

void make_blank()
{
   int x, y;
   
   for(y = 0; y < height; y++)
      for(x = 0; x < width; x++)
	 image[y][x] = PGM_MAXMAXVAL;
}

void fill_image()
{
   int x, y;
   int min_x, max_x;
   int darkest;
   
   for(y = 0; y < height; y++)
   {
       darkest = PGM_MAXMAXVAL;
       for(min_x = 0; min_x < width; min_x++) {
	   if(image[y][min_x] > darkest)
	      break;
	   else
	      darkest = image[y][min_x];
       }

       darkest = PGM_MAXMAXVAL;
       for(max_x = width - 1; max_x >= 0; max_x--) {
	   if(image[y][max_x] > darkest)
	      break;
	   else
	      darkest = image[y][max_x];
       }

       for(min_x; min_x <= max_x; min_x++)
	  image[y][min_x] = 0;
   }
}
     

void draw_line(int x1, int y1, int x2, int y2)
{
   int tmp;
   int dx, dy, error;

   dx = x2 - x1;
   dy = y2 - y1;

   image[y1][x1] = 0;
   if(abs(dx) > abs(dy))
   {
       int up = 0;
       if(x1 > x2)
       {
	   up = 1;
	   tmp = x1; x1 = x2; x2 = tmp;
	   tmp = y1; y1 = y2; y2 = tmp;
	   dx = -dx; dy = -dy;
       }
       error = dx;
       while(x1 != x2)
       {
	   x1++;
	   error += dy;
	   if(error > dx) {
	       error -= dx;
	       y1++;
	   }
	   else if(error <= 0) {
	       error += dx;
	       y1--;
	   }
	   if(up || error == dx)
	      image[y1][x1] = min(image[y1][x1], (dx - error) * PGM_MAXMAXVAL / dx);
	   else
	      image[y1 - 1][x1] = min(image[y1 - 1][x1], error * PGM_MAXMAXVAL / dx);
       }
   } else
   {
       int right = 1;
       if(y1 > y2)
       {
	   tmp = x1; x1 = x2; x2 = tmp;
	   tmp = y1; y1 = y2; y2 = tmp;
	   dx = -dx; dy = -dy;
	   right = 0;
       }
       error = dy;
       while(y1 != y2)
       {
	   y1++;
	   error += dx;
	   if(error > dy) {
	       error -= dy;
	       x1++;
	   }
	   else if(error <= 0) {
	       error += dy;
	       x1--;
	   }
	   if(right || error == dy)
	      image[y1][x1] = min(image[y1][x1], (dy - error) * PGM_MAXMAXVAL / dy);
	   else
	      image[y1][x1 - 1] = min(image[y1][x1 - 1], error * PGM_MAXMAXVAL / dy);
       }
   }
}

int main(int argc, char *argv[])
{
   int start_x, start_y;
   int x1, y1, x2, y2;
   int i;

   if(argc <= 6)
      return 1;

   pgm_init(&argc, argv);
      
   width = strtol(argv[1], NULL, 0);
   height = strtol(argv[2], NULL, 0);

   image = pgm_allocarray(width, height);
   make_blank();

   x2 = strtol(argv[3], NULL, 0);
   y2 = strtol(argv[4], NULL, 0);
   start_x = x2;
   start_y = y2;

   for(i = 5; i+1 < argc; i += 2)
   {
       x1 = x2;
       y1 = y2;
       x2 = strtol(argv[i], NULL, 0);
       y2 = strtol(argv[i+1], NULL, 0);
       draw_line(x1, y1, x2, y2);
   }
   draw_line(x2, y2, start_x, start_y);
   fill_image();
   pgm_writepgm(stdout, image, width, height, PGM_MAXMAXVAL, 0);
   pgm_freearray(image, height);

   return 0;
}
