#include <madness/world/world.h>

extern int d;

struct Key {
  // (x, y, z, l, ts) where (x, y, z) is the cartesian coordinate of the anchor
  //                    l is the level of the block in the octree
  //                    ts is the timestep
    

  int x = -1, y = -1, z = -1, l = -1, ts = -1;
  madness::hashT hash_val;

  Key() { morton_3d_rehash(); }
  Key(int x, int y, int z, int l, int ts) : x(x), y(y), z(z), l(l), ts(ts) 
  { 
    morton_3d_rehash(); 
    //rehash();
  }

  madness::hashT hash() const { return hash_val; }


  inline int morton_3d_rehash()
  {
    int X = x, Y = y, Z = z;
    X = (X | (X << 16)) & 0x030000FF;
    X = (X | (X <<  8)) & 0x0300F00F;
    X = (X | (X <<  4)) & 0x030C30C3;
    X = (X | (X <<  2)) & 0x09249249;

    Y = (Y | (Y << 16)) & 0x030000FF;
    Y = (Y | (Y <<  8)) & 0x0300F00F;
    Y = (Y | (Y <<  4)) & 0x030C30C3;
    Y = (Y | (Y <<  2)) & 0x09249249;

    Z = (Z | (Z << 16)) & 0x030000FF;
    Z = (Z | (Z <<  8)) & 0x0300F00F;
    Z = (Z | (Z <<  4)) & 0x030C30C3;
    Z = (Z | (Z <<  2)) & 0x09249249;

    int morton = X | (Y << 1) | (Z << 2);
    hash_val = ( morton << (d-1) ) | l;
    return 1;
  }


  // Equality test
  bool operator==(const Key& b) const { return (x == b.x && y == b.y && z == b.z && l == b.l && ts == b.ts); }
  // Inequality test
  bool operator!=(const Key& b) const { return !((*this) == b); }

  //less than operator
  bool operator<(const Key& b) const 
  { 
    if( (hash_val + ts) < (b.hash_val + b.ts) )
      return true;
    else
      return false;

  }

  template <typename Archive>
  void serialize(Archive& ar) {
    ar& madness::archive::wrap((unsigned char*)this, sizeof(*this));
  }
};
