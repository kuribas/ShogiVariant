global_settings { assumed_gamma 2.2 }
include "colors.inc"
#include "textures.inc"
#include "shapes.inc"
#include "woods.inc"

camera {
   location <0, 0, -200>
   direction z*10
   right x*1
}

light_source {<500, 500, -1000> color White}
background { color Gray30 }

#declare My_wood = 
   texture { pigment { P_WoodGrain2A color_map { M_Wood10A }}}
   texture { pigment { P_WoodGrain2B color_map { M_Wood10B }}}

plane { <0, 0, 1>, 0 
  texture { My_wood rotate <87, 33, 0> translate <4.2 2.1 0> }
}