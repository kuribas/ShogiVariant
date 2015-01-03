#include <ppm.h>
#include <stdio.h>
#include <getopt.h>

int main(int argc, char **argv)
{
   pixel bgcolor, border;
   pixel **image;
   int width, height;
   int x, y;
   int top, left, mid_x, mid_y;
   pixval *maxval;

   ppm_init(&argc, argv);
   image = ppm_readppm(stdin, &width, &height, &maxval);

   bgcolor = image[0][0];
   x = width / 2;
   y = 0;
   while(PPM_EQUAL(image[y][x], bgcolor))y++;

   border = image[y][x];
   top = y;

   while(1)
   {
       x--;
       if(PPM_EQUAL(image[y][x], bgcolor)) {
	   y++;
	   if(PPM_EQUAL(image[y][x - 1], bgcolor)) {
	       mid_x = x;
	       mid_y = y;
	       break;
	   }
       }
   }
   x = 0;
   y = height - 1;
   while(PPM_EQUAL(image[y][x], bgcolor))
      x++;

   left = x;
   
   printf("%i %i ", left, height - 1);
   printf("%i %i ", mid_x, mid_y);
   printf("%i %i ", width / 2, top);
   printf("%i %i ", width - mid_x - 1, mid_y);
   printf("%i %i ", width - left - 1, height - 1);
   
   ppm_freearray(image, height);
   return 0;
}
