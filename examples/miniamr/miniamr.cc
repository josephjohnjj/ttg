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
  // (x, y, z, l, ts) where (x, y, z) is the cartesian coordinate of the anchor
  //                    l is the level of the block in the octree
  //                    ts is the timestep
    

  int x = 0, y = 0, z = 0, l = 0, ts = 0;
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

  template <typename Archive>
  void serialize(Archive& ar) {
    ar& madness::archive::wrap((unsigned char*)this, sizeof(*this));
  }

};

std::map<Key, std::atomic<int>> key_edge_map; // keymap used to identify the input edge number
                                     // of an aggregator task    
std::mutex key_edge_mutex; // mutex to manage operation on key_edge_map  

int edge_number(Key& key)
{
  auto [x, y, z, l, ts, h] = key;

  const std::lock_guard<std::mutex> lock( key_edge_mutex);

  key_edge_map.emplace(key, 0); //insert key-vale if it doesnt exist
  auto it = key_edge_map.find(key);
  int terminal_id = it->second++;
  return terminal_id;
}



std::map<Key, const Key> child_parent_map; // maps a child key to a parent key    
std::mutex child_parent_mutex; // mutex to manage operation on parent_map 

int insert_key(Key& child_key, const Key& parent_key )
{
  const std::lock_guard<std::mutex> lock( child_parent_mutex);
  child_parent_map.emplace(child_key, parent_key); //insert child_key: parent_key
  return 1;
}

const Key find_key(Key child_key)
{
  auto it = child_parent_map.find(child_key);
  return it->second;
}


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


void stencil(Key oct_key, Key parent_key)
{
  printf("MINIMAR: OCTANT(x=%d, y=%d, z=%d, l=%d, ts=%d) --HASH %lu-- ---calls---> AGGREGATOR(x=%d, y=%d, z=%d, l=%d, ts=%d) \n", 
    oct_key.x, oct_key.y, oct_key.z, oct_key.l, oct_key.ts, oct_key.hash_val,
    parent_key.x, parent_key.y, parent_key.z, parent_key.l, parent_key.ts+1);
}


auto make_aggregator(ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data1,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data2,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data3,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data4,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data5,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data6,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data7,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data8) 
{

  auto f = [=](const Key& key,  Octant_Aggregator_Data& data1,
                                Octant_Aggregator_Data& data2,
                                Octant_Aggregator_Data& data3,
                                Octant_Aggregator_Data& data4,
                                Octant_Aggregator_Data& data5,
                                Octant_Aggregator_Data& data6,
                                Octant_Aggregator_Data& data7,
                                Octant_Aggregator_Data& data8, std::tuple<>& out)
            {
              auto [x, y, z, l, ts, h] = key;
              key_edge_map.erase(key);
              
              printf("MINIMAR: AGGREGATOR(x=%d, y=%d, z=%d, l=%d, ts=%d --HASH %lu-- ) \n", x, y, z, l, ts, h);
            };

  return ttg::wrap<Key>(f, ttg::edges(aggregator_data1,                                                                       
                                      aggregator_data2, 
                                      aggregator_data3, 
                                      aggregator_data4, 
                                      aggregator_data5, 
                                      aggregator_data6,
                                      aggregator_data7,
                                      aggregator_data8), ttg::edges(),"AGREGATOR", {"octant_aggregator_data_1",
                                                                                    "octant_aggregator_data_2",
                                                                                    "octant_aggregator_data_3",
                                                                                    "octant_aggregator_data_4",
                                                                                    "octant_aggregator_data_5",
                                                                                    "octant_aggregator_data_6",
                                                                                    "octant_aggregator_data_7",
                                                                                    "octant_aggregator_data_8"}, {});
}


auto make_octant(ttg::Edge<Key, Key>& treeParent_treeChild, ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data1,
                                                            ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data2,
                                                            ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data3,
                                                            ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data4,
                                                            ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data5,
                                                            ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data6,
                                                            ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data7,
                                                            ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data8) 
{

  auto f = [=](const Key& key,  Key& parent_key, std::tuple<ttg::Out<Key, Octant_Aggregator_Data>,
                                                            ttg::Out<Key, Octant_Aggregator_Data>,
                                                            ttg::Out<Key, Octant_Aggregator_Data>,
                                                            ttg::Out<Key, Octant_Aggregator_Data>,
                                                            ttg::Out<Key, Octant_Aggregator_Data>,
                                                            ttg::Out<Key, Octant_Aggregator_Data>,
                                                            ttg::Out<Key, Octant_Aggregator_Data>,
                                                            ttg::Out<Key, Octant_Aggregator_Data>>& out)
            {
              stencil(key, parent_key);
  
              Key aggKey = find_key(key);
              Octant_Aggregator_Data aggData{key, parent_key, parent_key.l};
              int edge = edge_number(aggKey);
             
              switch (edge)
              {
                case 0: ttg::send<0>(aggKey, aggData, out); break;
                case 1: ttg::send<1>(aggKey, aggData, out); break;
                case 2: ttg::send<2>(aggKey, aggData, out); break;
                case 3: ttg::send<3>(aggKey, aggData, out); break;
                case 4: ttg::send<4>(aggKey, aggData, out); break;
                case 5: ttg::send<5>(aggKey, aggData, out); break;
                case 6: ttg::send<6>(aggKey, aggData, out); break;
                case 7: ttg::send<7>(aggKey, aggData, out); break;
                default: printf("Something is wrong \n"); break;
              }            

            };

  return ttg::wrap<Key>(f, ttg::edges(treeParent_treeChild), ttg::edges(aggregator_data1,
                                                                        aggregator_data2,
                                                                        aggregator_data3,
                                                                        aggregator_data4,
                                                                        aggregator_data5,
                                                                        aggregator_data6,
                                                                        aggregator_data7,
                                                                        aggregator_data8 ), "OCTANT", {"parent"}, {"octant_aggregator1",
                                                                                                                   "octant_aggregator2",
                                                                                                                   "octant_aggregator3",
                                                                                                                   "octant_aggregator4",
                                                                                                                   "octant_aggregator5",
                                                                                                                   "octant_aggregator6",
                                                                                                                   "octant_aggregator7",
                                                                                                                   "octant_aggregator8"});
}     

auto make_initiator(ttg::Edge<Key, Key>& initiator) 
{

  auto f = [=](const Key& key, 
               std::tuple<ttg::Out<Key, Key>>& out)
            {
              printf("INITIATOR --HASH %lu--  \n", key.hash_val);

              int D = N / pow(2, key.l+1);

              Key child_key;

              child_key = {key.x, key.y, key.z, key.l+1, key.ts+1};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x+D, key.y, key.z, key.l+1, key.ts+1};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x, key.y+D, key.z, key.l+1, key.ts+1};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x+D, key.y+D, key.z, key.l+1, key.ts+1};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x, key.y, key.z+D, key.l+1, key.ts+1};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);
              
              child_key = {key.x+D, key.y, key.z+D, key.l+1, key.ts+1};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x, key.y+D, key.z+D, key.l+1, key.ts+1};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x+D, key.y+D, key.z+D, key.l+1, key.ts+1};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);
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
  ttg::Edge<Key, Octant_Aggregator_Data> aggregator_data1("aggregator_data1"); 
  ttg::Edge<Key, Octant_Aggregator_Data> aggregator_data2("aggregator_data2"); 
  ttg::Edge<Key, Octant_Aggregator_Data> aggregator_data3("aggregator_data3"); 
  ttg::Edge<Key, Octant_Aggregator_Data> aggregator_data4("aggregator_data4"); 
  ttg::Edge<Key, Octant_Aggregator_Data> aggregator_data5("aggregator_data5"); 
  ttg::Edge<Key, Octant_Aggregator_Data> aggregator_data6("aggregator_data6"); 
  ttg::Edge<Key, Octant_Aggregator_Data> aggregator_data7("aggregator_data7"); 
  ttg::Edge<Key, Octant_Aggregator_Data> aggregator_data8("aggregator_data8"); 
 
  auto op_initiator = make_initiator(treeParent_treeChild);
  auto op_octant = make_octant(treeParent_treeChild, aggregator_data1, aggregator_data2, aggregator_data3, 
                                                     aggregator_data4, aggregator_data5, aggregator_data6, 
                                                     aggregator_data7, aggregator_data8);   
  auto op_aggregator = make_aggregator(aggregator_data1, aggregator_data2, aggregator_data3, 
                                       aggregator_data4, aggregator_data5, aggregator_data6, 
                                       aggregator_data7, aggregator_data8);              
  
  auto keymap = [=](const Key& key) { return key.z %  world.size(); }; 
  op_octant->set_keymap(keymap);

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