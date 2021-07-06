#include <array>
#include <cmath>
#include <cstdio>
#include "ttg.h"
using namespace ttg;
#include <madness/world/world.h>

int d = 6;

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

  Key get_octant_key() const { return octant_key; }
  Key get_parent_key() const { return parent_key; }
  int get_next_level() const { return next_level; }

  void set_octant_key(Key key)  { octant_key = key; }
  void set_parent_key(Key key)  { parent_key = key; }
  void set_next_level(int value) { next_level = value; }

  //template <typename Archive>
  //void serialize(Archive& ar) {
  //  ar& madness::archive::wrap((unsigned char*)this, sizeof(*this));
  //}

};

namespace madness {
  namespace archive {
    template <class Archive>
    struct ArchiveStoreImpl<Archive, Octant_Aggregator_Data> {
      static inline void store(const Archive& ar, const Octant_Aggregator_Data& d) {
        ar << d.get_octant_key() << d.get_parent_key() << d.get_next_level();
        ar << wrap(&d, sizeof(Octant_Aggregator_Data));
      }
    };

    template <class Archive>
    struct ArchiveLoadImpl<Archive, Octant_Aggregator_Data> {
      static inline void load(const Archive& ar, Octant_Aggregator_Data& d) {
        Key octant_key;
        Key parent_key;
        int next_level;
        ar >> octant_key >> parent_key >> next_level;
        d = Octant_Aggregator_Data(octant_key, parent_key, next_level);
        ar >> wrap(&d, sizeof(Octant_Aggregator_Data));
      }
    };
  }
}

auto make_test(ttg::Edge<Key, std::vector<Octant_Aggregator_Data>>& testEdge)
{
  auto f = [=](const Key& key, std::vector<Octant_Aggregator_Data>& testData, std::tuple<>& out)
            {
              printf("==========TEST========  \n");
            };

  return ttg::wrap<Key>(f, ttg::edges(testEdge), ttg::edges(), "test", {"testEdge"}, {});
}



int main(int argc, char** argv) 
{
  
  ttg::ttg_initialize(argc, argv, -1);
  auto world = ttg::ttg_default_execution_context();

  ttg::Edge<Key, std::vector<Octant_Aggregator_Data>> edge;
  auto op = make_test(edge);

  auto connected = make_graph_executable(op.get());
  assert(connected);
  TTGUNUSED(connected);
  std::cout << "Graph is connected: " << connected << std::endl;

  if (world.rank() == 0) 
  {
    op->invoke(Key{100, 10, 10, 10, 10}, std::vector<Octant_Aggregator_Data>{});
  }

  ttg::ttg_execute(world);
  ttg::ttg_fence(world);
  ttg::ttg_finalize();

  return 0;
}