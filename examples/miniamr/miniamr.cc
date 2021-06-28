#include <array>
#include <cmath>
#include <cstdio>
#include "ttg.h"
using namespace ttg;
#include <madness/world/world.h>


/* 
 *  Default parameter values
 */

int d = 6; 
int mask; // pow(2, d-1) - 1;
int N; //pow(2, d); //dimension of a blocks is NxNxN.


inline void inverse_morton_3d(uint32_t& x, uint32_t& y, uint32_t& z,  uint32_t& l, uint32_t input)
{
  l = input & mask;
  input = input >> (d-1);

  x = input &        0x09249249;
  y = (input >> 1) & 0x09249249;
  z = (input >> 2) & 0x09249249;

  x = ((x >> 2) | x) & 0x030C30C3;
  x = ((x >> 4) | x) & 0x0300F00F;
  x = ((x >> 8) | x) & 0x030000FF;
  x = ((x >>16) | x) & 0x000003FF;

  y = ((y >> 2) | y) & 0x030C30C3;
  y = ((y >> 4) | y) & 0x0300F00F;
  y = ((y >> 8) | y) & 0x030000FF;
  y = ((y >>16) | y) & 0x000003FF;

  z = ((z >> 2) | z) & 0x030C30C3;
  z = ((z >> 4) | z) & 0x0300F00F;
  z = ((z >> 8) | z) & 0x030000FF;
  z = ((z >>16) | z) & 0x000003FF;
}



struct Key {
  // (x, y, z, l) where (x, y, z) is the cartesian coordinate of the anchor
  //                    l is the level of the block in the octree
  //                    ts is the timestep
    

  int x = 0, y = 0, z = 0, l = 0, ts = 0;
  madness::hashT hash_val;

  Key() { morton_3d_rehash(); }
  Key(int x, int y, int z, int l, int ts) : x(x), y(y), z(z), l(l), ts(ts) { morton_3d_rehash(); }

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

    uint32_t morton = X | (Y << 1) | (Z << 2);
    hash_val = ( morton << (d-1) ) | l;

    return 1;
  }


  // Equality test
  bool operator==(const Key& b) const { return x == b.x && y == b.y && z == b.z; }
  // Inequality test
  bool operator!=(const Key& b) const { return !((*this) == b); }

  //less than operator
  bool operator<(const Key& b) const 
  { 
    int p, q, r;

    p = x^b.x;
    q = y^b.y;
    r = z^b.z;

    int max = std::max({p, q, r});

    if(max == p)
    {
      if( x < b.x )
        return true;
      else 
        return false;
    }
    else if(max == q)
    {
      if( y < b.y )
        return true;
      else 
        return false;
    }
    else if(max == q)
    {
      if( z < b.z )
        return true;
      else 
        return false;
    }

  }

  template <typename Archive>
  void serialize(Archive& ar) {
    ar& madness::archive::wrap((unsigned char*)this, sizeof(*this));
  }
};

struct Octant_Aggregator_Data
{
  Key octant_key;
  Key parent_key;
  int next_level;

  Octant_Aggregator_Data() = default;
  Octant_Aggregator_Data(Key key1, Key key2, int level) : 
    octant_key(key1), parent_key(key2), next_level(level){}

  bool operator==(const Octant_Aggregator_Data& b) const 
      { return octant_key == b.octant_key && parent_key == b.parent_key && next_level == b.next_level; }
      
  bool operator!=(const Octant_Aggregator_Data& b) const { return !((*this) == b); }

  template <typename Archive>
  void serialize(Archive& ar) {
    ar& madness::archive::wrap((unsigned char*)this, sizeof(*this));
  }

};


namespace std {
  // specialize std::hash for Key
  template <>
  struct hash<Key> {
    std::size_t operator()(const Key& s) const noexcept { return s.hash(); }
  };
}  // namespace std

std::ostream& operator<<(std::ostream& s, const Key& key) {
  s << "Key(" << key.x << ", " << key.y << ", " << key.z << ", " << key.l << ")";
  return s;
}

auto make_aggregator(ttg::Edge<Key, Octant_Aggregator_Data>& octant_aggregator, 
  ttg::Edge<Key, Key>& aggregator_aggregator, ttg::Edge<Key, Key>& inject_task) 
{

  auto f = [=](const Key& key,  Octant_Aggregator_Data& octant_aggregator1,
                                Octant_Aggregator_Data& octant_aggregator2,
                                Octant_Aggregator_Data& octant_aggregator3,
                                Octant_Aggregator_Data& octant_aggregator4,
                                Octant_Aggregator_Data& octant_aggregator5,
                                Octant_Aggregator_Data& octant_aggregator6,
                                Octant_Aggregator_Data& octant_aggregator7,
                                Octant_Aggregator_Data& octant_aggregator8, //std::tuple<ttg::Out<Key, Key>,
                                                                            //    ttg::Out<Key, Key>>& out)
                                                                            std::tuple<>& out)
            {
              auto [x, y, z, l, ts, h] = key;

              printf("\n MINIMAR: AGGREGATOR(x=%d, y=%d, z=%d, l=%d, ts=%d)\n", x, y, z, l, ts);
            
              //if( ts == 1)
              //{
                //printf("MINIMAR: I AM HERE \n");
//
                //Key octant_key = octant_aggregator1.octant_key;
                //int D = N / pow(2, octant_key.l+1);

                //ttg::send<1>(Key{octant_key.x, octant_key.y, octant_key.z, octant_key.l+1, octant_key.ts+1}, octant_key, out);
                //ttg::send<1>(Key{octant_key.x+D, octant_key.y, octant_key.z, octant_key.l+1, octant_key.ts+1}, octant_key, out);
                //ttg::send<1>(Key{octant_key.x, octant_key.y+D, octant_key.z, octant_key.l+1, octant_key.ts+1}, octant_key, out);
                //ttg::send<1>(Key{octant_key.x+D, octant_key.y+D, octant_key.z, octant_key.l+1, octant_key.ts+1}, octant_key, out);
                //ttg::send<1>(Key{octant_key.x, octant_key.y, octant_key.z+D, octant_key.l+1, octant_key.ts+1}, octant_key, out);
                //ttg::send<1>(Key{octant_key.x+D, octant_key.y, octant_key.z+D, octant_key.l+1, octant_key.ts+1}, octant_key, out);
                //ttg::send<1>(Key{octant_key.x, octant_key.y+D, octant_key.z+D, octant_key.l+1, octant_key.ts+1}, octant_key, out);
                //ttg::send<1>(Key{octant_key.x+D, octant_key.y+D, octant_key.z+D, octant_key.l+1, octant_key.ts+1}, octant_key, out);

                //Key injected = octant_aggregator2.octant_key;
                //ttg::send<1>(Key{injected.x, injected.y, injected.z, injected.l, injected.ts+1}, injected, out);
//
                //injected = octant_aggregator3.octant_key;
                //ttg::send<1>(Key{injected.x, injected.y, injected.z, injected.l, injected.ts+1}, injected, out);
//
                //injected = octant_aggregator4.octant_key;
                //ttg::send<1>(Key{injected.x, injected.y, injected.z, injected.l, injected.ts+1}, injected, out);
//
                //injected = octant_aggregator5.octant_key;
                //ttg::send<1>(Key{injected.x, injected.y, injected.z, injected.l, injected.ts+1}, injected, out);
//
                //injected = octant_aggregator6.octant_key;
                //ttg::send<1>(Key{injected.x, injected.y, injected.z, injected.l, injected.ts+1}, injected, out);
//
                //injected = octant_aggregator7.octant_key;
                //ttg::send<1>(Key{injected.x, injected.y, injected.z, injected.l, injected.ts+1}, injected, out);
//
                //injected = octant_aggregator8.octant_key;
                //ttg::send<1>(Key{injected.x, injected.y, injected.z, injected.l, injected.ts+1}, injected, out);

              //}

            };

  //return ttg::wrap<Key>(f, ttg::edges(octant_aggregator,                                                                       
  //                                    octant_aggregator, 
  //                                    octant_aggregator, 
  //                                    octant_aggregator, 
  //                                    octant_aggregator, 
  //                                    octant_aggregator,
  //                                    octant_aggregator,
  //                                    octant_aggregator), ttg::edges(aggregator_aggregator,
  //                                                                            inject_task),"AGREGATOR", {"octant_key1",
  //                                                                                                       "octant_key2",
  //                                                                                                       "octant_key3",
  //                                                                                                       "octant_key4",
  //                                                                                                       "octant_key5",
  //                                                                                                       "octant_key6",
  //                                                                                                       "octant_key7",
  //                                                                                                       "octant_key8"}, {"par_aggregator",
  // 
  
  return ttg::wrap<Key>(f, ttg::edges(octant_aggregator,                                                                       
                                      octant_aggregator, 
                                      octant_aggregator, 
                                      octant_aggregator, 
                                      octant_aggregator, 
                                      octant_aggregator,
                                      octant_aggregator,
                                      octant_aggregator), ttg::edges(),"AGREGATOR", {"octant_key1",
                                                                                                         "octant_key2",
                                                                                                         "octant_key3",
                                                                                                         "octant_key4",
                                                                                                         "octant_key5",
                                                                                                         "octant_key6",
                                                                                                         "octant_key7",
                                                                                                         "octant_key8"}, {});
}

void stencil(Key oct_key, Key parent_key)
{
  printf("\n MINIMAR: Call OCTANT(x=%d, y=%d, z=%d, l=%d, ts=%d)--->AGGREGATOR(x=%d, y=%d, z=%d, l=%d, ts=%d) \n", 
    oct_key.x, oct_key.y, oct_key.z, oct_key.l, oct_key.ts,
    parent_key.x, parent_key.y, parent_key.z, parent_key.l, parent_key.ts+1);
}

auto make_octant(ttg::Edge<Key, Key>& treeParent_treeChild, ttg::Edge<Key, Octant_Aggregator_Data>& octant_aggregator) 
{

  auto f = [=](const Key& key,  Key& parent_key, std::tuple<ttg::Out<Key, Octant_Aggregator_Data>>& out)
            {
              auto [x, y, z, l, ts, h] = key;
              auto [p, q, r, s, t, u] = parent_key;
              
              stencil(key, parent_key);
              
              ttg::send<0>(Key{parent_key.x, parent_key.y, parent_key.z, parent_key.l, parent_key.ts+1}, 
                Octant_Aggregator_Data{key, parent_key, l}, out);

            };

  return ttg::wrap<Key>(f, ttg::edges(treeParent_treeChild), ttg::edges(octant_aggregator), "OCTANT", 
    {"parOctant"}, {"octant_aggregator"});
}     

auto make_initiator(ttg::Edge<Key, Key>& initiator) 
{

  auto f = [=](const Key& key, 
               std::tuple<ttg::Out<Key, Key>>& out)
            {
              std::cout<<"Initiated.....\n";

              int D = N / pow(2, key.l+1);

              ttg::send<0>(Key{key.x, key.y, key.z, key.l+1, key.ts+1}, key, out);
              ttg::send<0>(Key{key.x+D, key.y, key.z, key.l+1, key.ts+1}, key, out);
              ttg::send<0>(Key{key.x, key.y+D, key.z, key.l+1, key.ts+1}, key, out);
              ttg::send<0>(Key{key.x+D, key.y+D, key.z, key.l+1, key.ts+1}, key, out);
              ttg::send<0>(Key{key.x, key.y, key.z+D, key.l+1, key.ts+1}, key, out);
              ttg::send<0>(Key{key.x+D, key.y, key.z+D, key.l+1, key.ts+1}, key, out);
              ttg::send<0>(Key{key.x, key.y+D, key.z+D, key.l+1, key.ts+1}, key, out);
              ttg::send<0>(Key{key.x+D, key.y+D, key.z+D, key.l+1, key.ts+1}, key, out);
            };

  return ttg::wrap<Key>(f, ttg::edges(), ttg::edges(initiator), "INITIATOR", {}, {"initiator"});
} 


int main(int argc, char** argv) 
{
  mask = pow(2, d-1) - 1;
  N = pow(2, d); //dimension of a blocks is NxNxN.

  printf("MINIMAR: d = %d, mask = %d, N = %d \n", d, mask, N);

  ttg::ttg_initialize(argc, argv, -1);
  auto world = ttg::ttg_default_execution_context();

  ttg::Edge<Key, Key> treeParent_treeChild("treeParent_treeChild");  
  ttg::Edge<Key, Octant_Aggregator_Data> octant_aggregator("octant_aggregator"); 
  ttg::Edge<Key, Key> aggregator_aggregator("aggregator_aggregator"); 
 
  auto op_initiator = make_initiator(treeParent_treeChild);
  auto op_octant = make_octant(treeParent_treeChild, octant_aggregator);   
  auto op_aggregator = make_aggregator(octant_aggregator, aggregator_aggregator, treeParent_treeChild);             
  
  //auto keymap = [=](const Key& key) { return key.z %  world.size(); }; 
  //op_octant->set_keymap(keymap);

  auto connected = make_graph_executable(op_initiator.get());
  assert(connected);
  TTGUNUSED(connected);
  std::cout << "Graph is connected: " << connected << std::endl;

  if (world.rank() == 0) 
  {
    op_initiator->invoke(Key{0, 0, 0, 0, 0});
  }

  ttg::ttg_execute(world);
  ttg::ttg_fence(world);
  ttg::ttg_finalize();

  return 0;
}