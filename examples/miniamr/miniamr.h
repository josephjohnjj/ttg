#ifndef TTG_H
#define TTG_H
#include "ttg.h"
#endif /* TTG_H */
using namespace ttg;

#ifndef KEY_H
#define KEY_H
#include "key.h"
#endif /* KEY_H */

#include "miniamr_ht.h"

/* 
 *  Default parameter values
 */

int d = 6; 
int mask; // pow(2, d-1) - 1;
int N; //pow(2, d); //dimension of a blocks is NxNxN.
int TotTs = 1; // total ts to run the simulation

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

static int edge_number(Key& key)
{
  auto [x, y, z, l, ts, h] = key;

  const std::lock_guard<std::mutex> lock( key_edge_mutex);

  key_edge_map.emplace(key, 0); //insert key-vale if it doesnt exist
  auto it = key_edge_map.find(key);
  int terminal_id = it->second++;
  return terminal_id;
}



std::map<Key, Key> child_parent_map; // maps a child key to a parent key    
std::mutex child_parent_mutex; // mutex to manage operation on parent_map 

int insert_key(Key child_key, Key parent_key )
{
  const std::lock_guard<std::mutex> lock( child_parent_mutex);

  auto it = child_parent_map.find(child_key);
  //if(it != child_parent_map.end())
  //  printf("INSERT ERROR --> (x=%d, y=%d, z=%d, l=%d, ts=%d) : (x=%d, y=%d, z=%d, l=%d, ts=%d) \n",
  //  child_key.x, child_key.y, child_key.z, child_key.l, child_key.ts,
  //  parent_key.x, parent_key.y, parent_key.z, parent_key.l, parent_key.ts);
  child_parent_map.emplace(child_key, parent_key); //insert child_key: parent_key

  //printf("INSERTED --> (x=%d, y=%d, z=%d, l=%d, ts=%d) : (x=%d, y=%d, z=%d, l=%d, ts=%d) \n",
  //  child_key.x, child_key.y, child_key.z, child_key.l, child_key.ts,
  //  parent_key.x, parent_key.y, parent_key.z, parent_key.l, parent_key.ts); 
  return 1;
}

Key find_key(Key child_key)
{
  auto it = child_parent_map.find(child_key);

  //printf("FOUND --> (x=%d, y=%d, z=%d, l=%d, ts=%d) : (x=%d, y=%d, z=%d, l=%d, ts=%d) \n",
  //  child_key.x, child_key.y, child_key.z, child_key.l, child_key.ts,
  //  it->second.x, it->second.y, it->second.z, it->second.l, it->second.ts);

  return it->second;
}

 void update_key_ts() // ts should be updated before the root agrregator activates
                     // the tasks in the next timestep. If this is not done tasks
                     // will not be able to find the aggregator in thenext timestep
{
  printf("^^^ UPDATE ^^^ \n");
  std::map<Key, Key> temp;
  temp.clear();
  for (auto it = child_parent_map.begin(); it != child_parent_map.end(); it++)
  {
    auto child_key = it->first;
    child_key.ts++;
    auto parent_key = it->second;
    parent_key.ts++;

    temp.insert({child_key, parent_key});
  }
  child_parent_map.clear();

  for (auto it = temp.begin(); it != temp.end(); it++)
    child_parent_map.insert({it->first, it->second});

  temp.clear();
  
  printf("^^^ END UPDATE ^^^ \n");
}

std::map<Key, Octant_Aggregator_Data> refinement; // maps an octant key in a ts to its refinement 
                                                       // decision in the next ts  
std::mutex refinement_mutex; // mutex to manage operation on refinement

void refinement_decision(Key octkey, Key parkey)
{
  
  auto [x, y, z, l, ts, h] = octkey;
  Octant_Aggregator_Data aggData;

  const std::lock_guard<std::mutex> lock(refinement_mutex);

  if(x == 32 && y == 32 && z == 32 && l == 0) //This should be replaced by refinement condition
    aggData = Octant_Aggregator_Data{octkey, parkey, octkey.l+1};
  else
    aggData = Octant_Aggregator_Data{octkey, parkey, octkey.l};

  refinement.insert({octkey, aggData}); 

}

void refinement_parse()
{
  for( auto it = refinement.begin(); it != refinement.end(); it++)
  {
    Key octkey = it->second.octant_key;

    //This should be replaced by the actual refinement parsing
    printf("PARSE --> (x=%d, y=%d, z=%d, l=%d, ts=%d) current level = %d, next level = %d \n",
      octkey.x, octkey.y, octkey.z, octkey.l, octkey.ts, octkey.l, it->second.next_level);
  }
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

void stencil(Key oct_key, Key AggKey, int edge)
{
  //This should be replaced by stencil computation
  printf("OCTANT(x=%d, y=%d, z=%d, l=%d, ts=%d) --HASH %lu-- ---calls---> AGGREGATOR(x=%d, y=%d, z=%d, l=%d, ts=%d, EDGE = %d \n", 
    oct_key.x, oct_key.y, oct_key.z, oct_key.l, oct_key.ts, oct_key.hash_val,
    AggKey.x, AggKey.y, AggKey.z, AggKey.l, AggKey.ts,
    edge);
}

void stencil(Key oct_key)
{
  //get_ht_data(1, oct_key);
}


