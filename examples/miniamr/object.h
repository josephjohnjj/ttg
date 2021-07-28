
extern int N;

#ifndef KEY_H
#define KEY_H
#include "key.h"
#endif /* KEY_H */

struct Object{
    //int type;
    int bounce;
    double cen[3];
    double orig_cen[3];
    double move[3];
    double orig_move[3];
    double size; //distance in each direction from the centre
    double orig_size;
    double inc; 
    Object() = default;
    Object(int b, double *c, double *oc, double *m, double *om, double s, double os, double inc ):
       bounce(b), cen({c[0], c[1], c[2]}), orig_cen({oc[0], oc[1], oc[2]}), move({m[0], m[1], m[2]}),
       orig_move({om[0], om[1], om[2]}), size(s), orig_size(os), inc(inc) {}

    int check_object_intersection(Key key)
    {
      int D = N / pow(2, key.l);
      auto [x, y, z, l, ts, r, h] = key;
    
      double block_x_min = x;
      double block_x_max = x + D;
      double block_y_min = y;
      double block_y_max = y + D;
      double block_z_min = z;
      double block_z_max = z + D;
    
      double object_x_min = cen[0] - size;
      double object_x_max = cen[0] + size;
      double object_y_min = cen[1] - size;
      double object_y_max = cen[1] + size;
      double object_z_min = cen[0] - size;
      double object_z_max = cen[0] + size;
    
      if( object_x_min >  block_x_min && object_x_max < block_x_max &&
          object_y_min >  block_y_min && object_y_max < block_y_max &&
          object_z_min >  block_z_min && object_z_max < block_z_max )
    
        return 1;
    
      return 0;  
    }

void move_object(double delta)
{
  for (int j = 0; j < 3; j++) {
     cen[j] += move[j];
     if (bounce)
        if (cen[j] >= 1.0) {
          cen[j] = 2.0 - cen[j];
          move[j] = -move[j];
        } else if (cen[j] <= 0.0) {
           cen[j] = 0.0 - cen[j];
           move[j] = -move[j];
        }  
  }
  size += delta * inc;
}
    


};