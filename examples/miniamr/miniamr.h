#ifndef TTG_H
#define TTG_H
#include "ttg.h"
#endif /* TTG_H */
using namespace ttg;

#ifndef KEY_H
#define KEY_H
#include "key.h"
#endif /* KEY_H */

#ifndef AGGREGATORDATA_H
#define AGGREGATORDATA_H
#include "aggregatordata.h"
#endif /* AGGREGATORDATA_H */

#include "miniamr_ht.h"
#include "block.h"
#include "../blockmatrix.h"

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

template <typename T>
void stencil(Key oct_key)
{
  BlockData<double> blockData = *(reinterpret_cast<BlockData<double>*> (get_ht_data(0, oct_key)));
  BlockMatrix<double> face1 = *(reinterpret_cast<BlockMatrix<double>*> (get_ht_data(1, oct_key)));
  BlockMatrix<double> face2 = *(reinterpret_cast<BlockMatrix<double>*> (get_ht_data(2, oct_key)));
  BlockMatrix<double> face3 = *(reinterpret_cast<BlockMatrix<double>*> (get_ht_data(3, oct_key)));
  BlockMatrix<double> face4 = *(reinterpret_cast<BlockMatrix<double>*> (get_ht_data(4, oct_key)));
  BlockMatrix<double> face5 = *(reinterpret_cast<BlockMatrix<double>*> (get_ht_data(5, oct_key)));
  BlockMatrix<double> face6 = *(reinterpret_cast<BlockMatrix<double>*> (get_ht_data(6, oct_key)));
  
  int _x = blockData.x();
  int _y = blockData.y();
  int _z = blockData.z();


  for (int i = 0; i < _x; ++i) 
      for (int j = 0; j < _y; ++j) 
        for (int k = 0; k < _z; ++k)
        {
            double val = /*same*/    ( blockData(i, j, k) + 
                         /*top*/      (k != _z-1) ? blockData(i, j, k+1) : ( (face1 != NULL) ? face1(i, j) : 0 ) +
                         /*bottom*/   (k != 0 ) ? blockData(i, j, k-1) : ( (face2 != NULL) ? face2(i, j) : 0 )+
                         /*forward*/  (j != _y-1) ? blockData(i, j+1, k) : ( (face3 != NULL) ? face3(i, k) : 0 )  + 
                         /*backward*/ (j != 0) ? blockData(i, j-1, k) : ( (face4 != NULL) ? face4(i, k) : 0 )  +
                         /*rigt*/     (i != _x-1) ? blockData(i+1, j, k) : ( (face5 != NULL) ? face5(j, k) : 0 )+ 
                         /*left*/     (i != 0) ? blockData(i-1, j, k) : ( (face6 != NULL) ? face6(j, k) : 0 )
                                     ) / 7;

            blockData(i, j, k, val); //store
                                              
        }
            

}

// 1 - top face  (z = N)
// 2 - bottom face (z = 0)
// 3 - forward  (x = N)
// 4 - backward (x = 0)
// 5 - rigt (y = N)
// 6 - left (z = 0)   

template <typename T>
BlockMatrix<double> generate_face(int face_num, Key oct_key)
{
  BlockData<double> blockData = *(reinterpret_cast<BlockData<double>*> (get_ht_data(0, oct_key)));
  int _x = blockData.x();
  int _y = blockData.y();
  int _z = blockData.z();

  int M, N;
  BlockMatrix<double> face(M, N); 

  
  if(face_num == 1)
  {
    M = _x;
    N = _y;
    for (int i = 0; i < M; ++i) 
      for (int j = 0; j < N; ++j) 
          face(i, j) = blockData(i, j, N-1);

    return face;
  }
  else if(face_num == 2)
  {
    M = _x;
    N = _y;
    for (int i = 0; i < M; ++i) 
      for (int j = 0; j < N; ++j) 
        face(i, j) = blockData(i, j, 0);

    return face;
  }
  else if(face_num == 3)
  {
    M = _y;
    N = _z;
    for (int j = 0; j < M; ++j) 
      for (int k = 0; k < N; ++k) 
        face(j, k) = blockData(N, j, k);
  
    return face;
  }
  else if(face_num == 4)
  {
    M = _y;
    N = _z;
    for (int j = 0; j < M; ++j) 
      for (int k = 0; k < N; ++k) 
        face(j, k) = blockData(0, j, k);

    return face;
  }
  else if(face_num == 5)
  {
    M = _x;
    N = _z;
    for (int i = 0; i < M; ++i) 
      for (int k = 0; k < N; ++k) 
          face(i, k) = blockData(i, N-1, k);

    return face;
  }
  else if(face_num == 6)
  {
    M = _x;
    N = _z;
    for (int i = 0; i < M; ++i) 
      for (int k = 0; k < N; ++k) 
        face(i, k) = blockData(i, 0, k);

    return face;
  }
  
}

int find_child_num(Key par_key, Key child_key)
{
  bool b0 = ( (par_key.z ^ child_key.z) == 0 ) ? 0 : 1;
  bool b1 = ( (par_key.y ^ child_key.y) == 0 ) ? 0 : 1;
  bool b2 = ( (par_key.x ^ child_key.x) == 0 ) ? 0 : 1;

  int child_num = 0 | b2 << 2 | b1 << 1 | b0;

  if(child_num == 0 && (par_key.l == child_key.l) )
    return 0;
  else
    return child_num + 1;

}

int neighbour_search_same_refinement(std::vector<Octant_Aggregator_Data>& aggData,
  Key par_key, Key child_key, std::vector<Key>& neighbours, int direction)
{
  Key search_key;
  int D = N / pow(2, child_key.l); //TODO:verify
  int found = 0;

  if(direction == 1)
  {
    search_key = child_key; 
    search_key.z += D;
  }
  else if(direction == 2)
  {
    search_key = child_key; 
    search_key.z -= D;
  }
  else if(direction == 3)
  {
    search_key = child_key; 
    search_key.x += D;
  }
  else if(direction == 4)
  {
    search_key = child_key; 
    search_key.x -= D;
  }
  else if(direction == 5)
  {
    search_key = child_key; 
    search_key.y += D;
  }
  else if(direction == 6)
  {
    search_key = child_key; 
    search_key.y -= D;
  }

  for (auto& it : aggData)
  {
    if(it.octant_key == search_key)
    {
      found = 1;
      neighbours.push_back(Key{search_key.x, search_key.y, search_key.z, search_key.l, search_key.ts});
      break;
    }
  }
    
  if(found == 1)
    return 1;
  else
    return 0;
}

int neighbour_search_lower_refinement(std::vector<Octant_Aggregator_Data>& aggData,
  Key par_key, Key child_key, std::vector<Key>& neighbours, int direction)
{
  Key search_key;
  int D = N / pow(2, par_key.l); //TODO:verify
  int found = 0;

  if(direction == 1)
  {
    search_key = par_key; 
    search_key.z += D;
  }
  else if(direction == 2)
  {
    search_key = par_key; 
    search_key.z -= D;
  }
  else if(direction == 3)
  {
    search_key = par_key; 
    search_key.x += D;
  }
  else if(direction == 4)
  {
    search_key = par_key; 
    search_key.x -= D;
  }
  else if(direction == 5)
  {
    search_key = par_key; 
    search_key.y += D;
  }
  else if(direction == 6)
  {
    search_key = par_key; 
    search_key.y -= D;
  }

  for (auto& it : aggData)
  {
    if(it.octant_key == search_key)
    {
      found = 1;
      neighbours.push_back(Key{search_key.x, search_key.y, search_key.z, search_key.l, search_key.ts});
      break;
    }
  }
    
  if(found == 1)
    return 1;
  else
    return 0;
}

int neighbour_search_higher_refinement(std::vector<Octant_Aggregator_Data> aggData,
  Key par_key, Key child_key, std::vector<Key>& neighbours, int direction)
{
  Key search_key;
  int D = N / pow(2, child_key.l); //TODO:verify
  int found = 0;

  //search only one key. If that is found the rest
  // can be inferred

  if(direction == 1)
  {
    search_key = child_key; 
    search_key.z += D;
    search_key.l += 1;
  }
  else if(direction == 2)
  {
    search_key = child_key; 
    search_key.z -= D;
    search_key.l += 1;
  }
  else if(direction == 3)
  {
    search_key = child_key; 
    search_key.x += D;
    search_key.l += 1;
  }
  else if(direction == 4)
  {
    search_key = child_key; 
    search_key.x -= D;
    search_key.l += 1;
  }
  else if(direction == 5)
  {
    search_key = child_key; 
    search_key.y += D;
    search_key.l += 1;
  }
  else if(direction == 6)
  {
    search_key = child_key; 
    search_key.y -= D;
    search_key.l += 1;
  }

  for (auto& it : aggData)
  {
    if(it.octant_key == search_key)
    {
      found = 1;
      break;
    }
  }
    
  if(found == 1)
  {
    auto [x, y, z, l, ts, h] = search_key;
    int DD = N / pow(2, search_key.l); //TODO:verify
    if(direction == 1)
    {
      neighbours.push_back(Key{x, y, z, l, ts});
      neighbours.push_back(Key{x+DD, y, z, l, ts});
      neighbours.push_back(Key{x, y+DD, z, l, ts});
      neighbours.push_back(Key{x+DD, y+DD, z, l, ts});
    }
    else if(direction == 2)
    {
      neighbours.push_back(Key{x, y, z, l, ts});
      neighbours.push_back(Key{x+DD, y, z, l, ts});
      neighbours.push_back(Key{x, y+DD, z, l, ts});
      neighbours.push_back(Key{x+DD, y+DD, z, l, ts});
    }
    else if(direction == 3)
    {
      neighbours.push_back(Key{x, y, z, l, ts});
      neighbours.push_back(Key{x, y+DD, z, l, ts});
      neighbours.push_back(Key{x, y, z+DD, l, ts});
      neighbours.push_back(Key{x, y+DD, z+DD, l, ts});
    }
    else if(direction == 4)
    {
      neighbours.push_back(Key{x, y, z, l, ts});
      neighbours.push_back(Key{x, y+DD, z, l, ts});
      neighbours.push_back(Key{x, y, z+DD, l, ts});
      neighbours.push_back(Key{x, y+DD, z+DD, l, ts});
    }
    else if(direction == 5)
    {
      neighbours.push_back(Key{x, y, z, l, ts});
      neighbours.push_back(Key{x+DD, y, z, l, ts});
      neighbours.push_back(Key{x, y, z+DD, l, ts});
      neighbours.push_back(Key{x+DD, y, z+DD, l, ts});
    }
    else if(direction == 6)
    {
      neighbours.push_back(Key{x, y, z, l, ts});
      neighbours.push_back(Key{x+DD, y, z, l, ts});
      neighbours.push_back(Key{x, y, z+DD, l, ts});
      neighbours.push_back(Key{x+DD, y, z+DD, l, ts});
    }

    return 1;
  }
  else
    return 0;

}

int find_neigbours(std::vector<Octant_Aggregator_Data>& aggData, Key par_key, Key child_key, 
  std::vector<Key>& neighbours)
{

  for(int direction = 1; direction < 7; direction++)
    if(0 == neighbour_search_same_refinement(aggData, par_key, child_key, neighbours, direction))
      if(0 == neighbour_search_lower_refinement(aggData, par_key, child_key, neighbours, direction))   
       if(0 == neighbour_search_higher_refinement(aggData, par_key, child_key, neighbours, direction))
       {
         printf("Border block \n");
       }

  return 0;
}







