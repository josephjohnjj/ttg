#include <assert.h>

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

#ifndef OBJECT_H
#define OBJECT_H
#include "object.h"
#endif /* OBJECT_H */

#ifndef POINT_H
#define POINT_H
#include "point.h"
#endif /* POINT_H */


#include "miniamr_ht.h"
#include "block.h"
#include "../blockmatrix.h"

/* 
 *  Default parameter values
 */

int d = 6; 
int mask; // pow(2, d-1) - 1;
int N; //pow(2, d); //dimension of a blocks is NxNxN.
const int Var = 40; //number of variables in each point in the mesh.
int TotTs = 2; // total ts to run the simulation.
Object rectangle;
int cur_max_level = 0;
std::mutex cur_max_level_mutex; // mutex to manage operation on key_edge_map  

int cur_max_level_inc()
{
  const std::lock_guard<std::mutex> lock( cur_max_level_mutex);
  return ++cur_max_level;
}

int cur_max_level_dec()
{
  const std::lock_guard<std::mutex> lock( cur_max_level_mutex);
  return ++cur_max_level;
}



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
  auto [x, y, z, l, ts, r, h] = key;

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
  assert(it == child_parent_map.end() );
  child_parent_map.insert({child_key, parent_key}); //insert child_key: parent_key

  return 1;
}



Key find_parent_key(Key child_key)
{
  const std::lock_guard<std::mutex> lock( child_parent_mutex);
  auto it = child_parent_map.find(child_key);
  assert(it != child_parent_map.end());
  return it->second;
}

// ts should be updated before the root agrregator activates
// the tasks in the next timestep. If this is not done tasks
// will not be able to find the aggregator in the next timestep

void update_key_ts(int ts) 
{
  const std::lock_guard<std::mutex> lock( child_parent_mutex);

  std::map<Key, Key> temp = child_parent_map;
  child_parent_map.clear();

  for (auto it = temp.begin(); it != temp.end(); it++)
  {
    Key child_key = it->first;
    Key par_key = it->second;

    child_key.ts = ts; 
    par_key.ts = ts;

    child_parent_map.insert({child_key, par_key});
  }
}

std::vector<Octant_Aggregator_Data> all_aggregated_data; //contains all aggregated data in a time step
std::mutex aggregated_data_mutex; // mutex to manage operation on all_aggregated_data


void insert_aggregated_data(Octant_Aggregator_Data agg_data)
{
  const std::lock_guard<std::mutex> lock(aggregated_data_mutex);
  all_aggregated_data.push_back(agg_data);
}

void update_refine_aggregated_data(Key key, int val)
{
  const std::lock_guard<std::mutex> lock(aggregated_data_mutex);
  for(auto it: all_aggregated_data)
  {
    if( it.octant_key == key )
     it.octant_key.refine = val; 

  }
}



std::map<Key, Octant_Aggregator_Data> refinement; // maps an octant key in a ts to its refinement 
                                                       // decision in the next ts  
std::mutex refinement_mutex; // mutex to manage operation on refinement



template <typename T>
void stencil(Key oct_key)
{
  BlockData<T> blockData;
  void* blockData_ptr;
  void *face1_ptr, *face2_ptr, *face3_ptr, *face4_ptr, *face5_ptr, *face6_ptr;
  BlockMatrix<T> face1, face2, face3, face4, face5, face6;

  if((blockData_ptr = get_ht_data(0, oct_key)) != nullptr)
    blockData = *(static_cast<BlockData<T>*> (blockData_ptr));

  if((face1_ptr = get_ht_data(2, oct_key)) != nullptr)
    face1 = *(reinterpret_cast<BlockMatrix<T>*> (face1_ptr));
  if((face2_ptr = get_ht_data(2, oct_key)) != nullptr)
    face2 = *(reinterpret_cast<BlockMatrix<T>*> (face2_ptr));
  if((face3_ptr = get_ht_data(3, oct_key)) != nullptr)
    face3 = *(reinterpret_cast<BlockMatrix<T>*> (face3_ptr));
  if((face4_ptr = get_ht_data(4, oct_key)) != nullptr)
    face4 = *(reinterpret_cast<BlockMatrix<T>*> (face4_ptr));
  if((face5_ptr = get_ht_data(5, oct_key)) != nullptr)
    face5 = *(reinterpret_cast<BlockMatrix<T>*> (face5_ptr));
  if((face6_ptr = get_ht_data(6, oct_key)) != nullptr)
    face6 = *(reinterpret_cast<BlockMatrix<T>*> (face6_ptr));
  
  //BlockMatrix<T> face2 = *(reinterpret_cast<BlockMatrix<T>*> (get_ht_data(2, oct_key)));
  //BlockMatrix<T> face3 = *(reinterpret_cast<BlockMatrix<T>*> (get_ht_data(3, oct_key)));
  //BlockMatrix<T> face4 = *(reinterpret_cast<BlockMatrix<T>*> (get_ht_data(4, oct_key)));
  //BlockMatrix<T> face5 = *(reinterpret_cast<BlockMatrix<T>*> (get_ht_data(5, oct_key)));
  //BlockMatrix<T> face6 = *(reinterpret_cast<BlockMatrix<T>*> (get_ht_data(6, oct_key)));
  
  int _x = N; //blockData.x();
  int _y = N; //blockData.y();
  int _z = N; //blockData.z();
  

  for (int i = 0; i < _x; ++i) 
      for (int j = 0; j < _y; ++j) 
        for (int k = 0; k < _z; ++k)
        {
            T top = (k != _z-1) ? blockData(i, j, k+1) : ( (face1_ptr != nullptr) ? face1(i, j) : T() );
            T bottom = (k != 0 ) ? blockData(i, j, k-1) : ( (face2_ptr != nullptr) ? face2(i, j) : T());
            T forward = (j != _y-1) ? blockData(i, j+1, k) : ( (face3_ptr != nullptr) ? face3(i, k) : T() );
            T backward = (j != 0) ? blockData(i, j-1, k) : ( (face4_ptr != nullptr) ? face4(i, k) : T() );
            T right = (i != _x-1) ? blockData(i+1, j, k) : ( (face5_ptr != nullptr) ? face5(j, k) : T() );
            T left =  (i != 0) ? blockData(i-1, j, k) : ( (face6_ptr != nullptr) ? face6(j, k) : T() );

            T val = ( blockData(i, j, k) + top + bottom + forward + backward+ right + left) / 7;

            blockData(i, j, k, val); //store                                   
        }
            

}

// 1 - top face  (z = N)
// 2 - bottom face (z = 0)
// 3 - forward  (x = N)
// 4 - backward (x = 0)
// 5 - rigt (y = N)
// 6 - left (y = 0)   

template <typename T>
BlockMatrix<T>* generate_face(int face_num, Key oct_key)
{
  BlockData<T> blockData = *(reinterpret_cast<BlockData<T>*> (get_ht_data(0, oct_key)));
  int _x = blockData.x();
  int _y = blockData.y();
  int _z = blockData.z();

  int row, col = N;

  BlockMatrix<T> *face_ptr = new BlockMatrix<T> (N, N);
  BlockMatrix<T> &face = *face_ptr;

  if(face_num == 1)
  {
    for (int i = 0; i < N; ++i) 
      for (int j = 0; j < N; ++j) 
          face(i, j) = blockData(i, j, N-1);
  }
  else if(face_num == 2)
  {
    for (int i = 0; i < N; ++i) 
      for (int j = 0; j < N; ++j) 
        face(i, j) = blockData(i, j, 0);
  }
  else if(face_num == 3)
  {
    for (int j = 0; j < N; ++j) 
      for (int k = 0; k < N; ++k) 
        face(j, k) = blockData(N, j, k);
  }
  else if(face_num == 4)
  {
    for (int j = 0; j < N; ++j) 
      for (int k = 0; k < N; ++k) 
        face(j, k) = blockData(0, j, k);
  }
  else if(face_num == 5)
  {
    for (int i = 0; i < N; ++i) 
      for (int k = 0; k < N; ++k) 
          face(i, k) = blockData(i, N-1, k);
  }
  else if(face_num == 6)
  {
    for (int i = 0; i < N; ++i) 
      for (int k = 0; k < N; ++k) 
        face(i, k) = blockData(i, 0, k);
  }
  return face_ptr;
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

void find_children_next_ts(Key key, std::vector<Key>& children_list)
{
  int D = N / pow(2, key.l+1);

  children_list.push_back({key.x,   key.y,   key.z,   key.l+1, key.ts+1});
  children_list.push_back({key.x+D, key.y,   key.z,   key.l+1, key.ts+1});
  children_list.push_back({key.x,   key.y+D, key.z,   key.l+1, key.ts+1});
  children_list.push_back({key.x+D, key.y+D, key.z,   key.l+1, key.ts+1});
  children_list.push_back({key.x,   key.y,   key.z+D, key.l+1, key.ts+1});
  children_list.push_back({key.x+D, key.y,   key.z+D, key.l+1, key.ts+1});
  children_list.push_back({key.x,   key.y+D, key.z+D, key.l+1, key.ts+1});
  children_list.push_back({key.x+D, key.y+D, key.z+D, key.l+1, key.ts+1});

}

void find_children_same_ts(Key key, std::vector<Key>& children_list)
{
  int D = N / pow(2, key.l+1);

  children_list.push_back({key.x,   key.y,   key.z,   key.l+1, key.ts});
  children_list.push_back({key.x+D, key.y,   key.z,   key.l+1, key.ts});
  children_list.push_back({key.x,   key.y+D, key.z,   key.l+1, key.ts});
  children_list.push_back({key.x+D, key.y+D, key.z,   key.l+1, key.ts});
  children_list.push_back({key.x,   key.y,   key.z+D, key.l+1, key.ts});
  children_list.push_back({key.x+D, key.y,   key.z+D, key.l+1, key.ts});
  children_list.push_back({key.x,   key.y+D, key.z+D, key.l+1, key.ts});
  children_list.push_back({key.x+D, key.y+D, key.z+D, key.l+1, key.ts});

}

int neighbour_search_same_refinement(std::vector<Octant_Aggregator_Data>& aggData,
  Key par_key, Key child_key, std::vector<Key>& neighbours, int direction)
{
  Key search_key;
  int D = N / pow(2, child_key.l); 

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
    //printf(" Search (%d, %d, %d, %d, %d) : (%d, %d, %d, %d, %d)\n", 
    //  search_key.x, search_key.y, search_key.z, search_key.l, search_key.ts,
    //  it.octant_key.x, it.octant_key.y, it.octant_key.z, it.octant_key.l, it.octant_key.ts) ;
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
    auto [x, y, z, l, ts, r, h] = search_key;
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
  std::vector<Key> neighbours[7])
{

  for(int direction = 1; direction < 7; direction++)
    if(0 == neighbour_search_same_refinement(aggData, par_key, child_key, neighbours[direction], direction))
      if(0 == neighbour_search_lower_refinement(aggData, par_key, child_key, neighbours[direction], direction))   
       if(0 == neighbour_search_higher_refinement(aggData, par_key, child_key, neighbours[direction], direction))
       {
         ;//printf("Border block \n");
       }

  return 0;
}



/*
 bounce - If this is 1 then an object will bounce off of the walls
          when the center hits an edge of the unit cube.  If it is
          zero, then the object can leave the mesh.

 center - Three doubles that determine the center of the object in the
          x, y, and z directions.

 move - Three doubles that determine the rate of movement of the center
        of the object in the x, y, and z directions.  The object moves
        this far at each timestep.

  size - The initial size of the object in the x, y, and z directions.
         If any of these become negative, the object will not be used
         in the calculations to determine refinement.  These sizes are
         from the center to the edge in the specified direction.

  inc - The change in size of the object in the x, y, and z directions.
        This will be multiplied by the size of the timestep. 
 */

Object initialise_object()
{
  int bounce = 1;
  double centre[3] = {3, 3, 3};
  double original_centre[3] = {3, 3, 3};
  double move[3] = {32, 32, 32};
  double original_move[3] = {32, 32, 32};
  double size = 2; 
  double original_size = 2;
  double inc = 0;


  return Object{bounce, centre, original_centre, move, original_move, size, original_size, inc};
  

}

int check_object_intersection(Key key, Object obj)
{
  int D = N / pow(2, key.l);
  auto [x, y, z, l, ts, r, h] = key;

  double block_x_min = x;
  double block_x_max = x + D;
  double block_y_min = y;
  double block_y_max = y + D;
  double block_z_min = z;
  double block_z_max = z + D;

  double object_x_min = obj.cen[0] - obj.size;
  double object_x_max = obj.cen[0] + obj.size;
  double object_y_min = obj.cen[1] - obj.size;
  double object_y_max = obj.cen[1] + obj.size;
  double object_z_min = obj.cen[0] - obj.size;
  double object_z_max = obj.cen[0] + obj.size;

  if( object_x_min >  block_x_min && object_x_max < block_x_max &&
      object_y_min >  block_y_min && object_y_max < block_y_max &&
      object_z_min >  block_z_min && object_z_max < block_z_max )
  
    return 1;

  return -1;  
}

int refine_level(std::vector<Octant_Aggregator_Data>& aggData)
{
  for (int level = cur_max_level; level >= 0; level--) 
  {
    
    std::vector<Key> neighbours[7];
    std::vector<Key> neighbour_list;
    std::vector<Key> children_list;

    for( auto& it: aggData)
    {
      
      Key& par_key = it.parent_key;
      Key& oct_key = it.octant_key;
    
      if(oct_key.l == level)
      {
        for(auto& temp: neighbours)
          temp.clear();
        neighbour_list.clear();

        find_neigbours(aggData, par_key, oct_key, neighbours);
        for(int direction = 1; direction < 7; direction++)
          for(auto nei_key: neighbours[direction])
          {
            neighbour_list.push_back(nei_key);
            
            //printf(" ***** OCT(x=%d, y=%d, z=%d, l=%d, ts=%d) NEI(x=%d, y=%d, z=%d, l=%d, ts=%d) *****\n", 
            //      oct_key.x, oct_key.y, oct_key.z, oct_key.l, oct_key.ts,
            //      nei_key.x, nei_key.y, nei_key.z, nei_key.l, nei_key.ts);
          }

        // all its neigbours in the same level cannot coarsen
        // all its neighbours in a lower refinement level should refine
        if(oct_key.refine == 1) 
        {   
          for(auto& nei_key: neighbour_list)
          {
            for(auto& iter: aggData)
            {
              if(nei_key == iter.octant_key)
                if(iter.octant_key.refine == -1 && iter.octant_key.l == oct_key.l) //neighbour at same level
                {
                  //neighbour cannot refine
                  //as it cannot refine, its sibling at the same level also cannot refine

                  find_children_same_ts(iter.parent_key, children_list);

                  for(auto& siblings: children_list)
                   for(auto& iter2: aggData)
                   {
                     if(siblings == iter2.octant_key && iter2.octant_key.refine == -1)
                         iter2.octant_key.refine = 0; 
                   }
                  
                }
                else if(iter.octant_key.refine == -1 && iter.octant_key.l > oct_key.l ) //neighbour at lower level
                  iter.octant_key.refine = 1; 
                
            }
          }
        }
        else if(oct_key.refine == -1)
        {
          for(auto& nei_key: neighbour_list)
          {
            if( nei_key.l  > level  ) //if any neighbour is at a higer refinement the octant cannot coarse
            {
              //Make sure that none of the siblings, including itself, does not coarsen
              find_children_same_ts(par_key, children_list);

               //printf("****************** REFINE ********************\n");
              for(auto& siblings: children_list)
              {
                //printf(" ***** PARENT(x=%d, y=%d, z=%d, l=%d, ts=%d) CHILDREN(x=%d, y=%d, z=%d, l=%d, ts=%d) \n", 
                //  par_key.x, par_key.y, par_key.z, par_key.l, par_key.ts,
                //  siblings.x, siblings.y, siblings.z, siblings.l, siblings.ts);
               for(auto& iter: aggData)
                {
                  if(siblings == iter.octant_key)
                    if(iter.octant_key.refine == -1)
                      iter.octant_key.refine = 0; 
                }
              }

            }    
             
          }
        }

        //printf(" REFINEMENT(x=%d, y=%d, z=%d, l=%d, ts=%d) = %d \n", 
        //  it.octant_key.x, it.octant_key.y, it.octant_key.z, it.octant_key.l, it.octant_key.ts, it.octant_key.refine);
      }
    }
  }

  return 0;
}


std::vector<Key> split_blocks(Key key)
{
  std::vector<Key> children_list;

  if(key.l+1 > cur_max_level)
    cur_max_level_inc();

  find_children_next_ts(key, children_list);

  void *ptr = get_ht_data(0, key);
  BlockData<Point<Var, double>> *block = reinterpret_cast<BlockData<Point<Var, double>> *> (ptr);
  BlockData<Point<Var, double>> &temp_block = *block;

  printf(" SPLIT(x=%d, y=%d, z=%d, l=%d, ts=%d)  \n", key.x, key.y, key.z, key.l, key.ts);

  for(auto it: children_list)
  {
    int i, j, k, i1, j1, k1, i2, j2, k2;

    if(find_child_num(key, it) == 1) {i1 = 0; j1 = 0; k1 = 0; }
    if(find_child_num(key, it) == 2) {i1 = N/2; j1 = 0; k1 = 0; }
    if(find_child_num(key, it) == 3) {i1 = 0; j1 = N/2; k1 = 0; }
    if(find_child_num(key, it) == 4) {i1 = N/2; j1 = N/2; k1 = 0; }
    if(find_child_num(key, it) == 5) {i1 = 0; j1 = 0; k1 = N/2; }
    if(find_child_num(key, it) == 6) {i1 = N/2; j1 = 0; k1 = N/2; }
    if(find_child_num(key, it) == 7) {i1 = 0; j1 = N/2; k1 = N/2; }
    if(find_child_num(key, it) == 8) {i1 = N/2; j1 = N/2; k1 = N/2; }

    BlockData<Point<Var, double>> *child_block = new BlockData<Point<Var, double>> (N, N, N);
    BlockData<Point<Var, double>> &temp_child_block = *child_block;

    for (i2 = i = 1; i < N/2; i++, i2+=2)
      for (j2 = j = 1; j < N/2; j++, j2+=2)
        for (k2 = k = 1; k < N/2; k++, k2+=2)
        {
          temp_child_block(i2  , j2  , k2  ) =  
          temp_child_block(i2+1, j2  , k2  ) = 
          temp_child_block(i2  , j2+1, k2  ) = 
          temp_child_block(i2+1, j2+1, k2  ) = 
          temp_child_block(i2  , j2  , k2+1) = 
          temp_child_block(i2+1, j2  , k2+1) = 
          temp_child_block(i2  , j2+1, k2+1) = 
          temp_child_block(i2+1, j2+1, k2+1) = (temp_block(i+i1, j+j1, k+k1) / 8.0);
        }

    insert_ht_data(0, it, child_block);
  }

 
 return children_list;

}



Key consolidate_blocks(Key key)
{
  std::vector<Key> children_list;

  find_children_same_ts(key, children_list);

  BlockData<Point<Var, double>> *consolidated_block = new BlockData<Point<Var, double>> (N, N, N);

  for(auto it: children_list)
  {
    int i, j, k, i1, j1, k1, i2, j2, k2;

    if(find_child_num(key, it) == 1) {i1 = 0; j1 = 0; k1 = 0; }
    if(find_child_num(key, it) == 2) {i1 = N/2; j1 = 0; k1 = 0; }
    if(find_child_num(key, it) == 3) {i1 = 0; j1 = N/2; k1 = 0; }
    if(find_child_num(key, it) == 1) {i1 = N/2; j1 = N/2; k1 = 0; }
    if(find_child_num(key, it) == 1) {i1 = 0; j1 = 0; k1 = N/2; }
    if(find_child_num(key, it) == 2) {i1 = N/2; j1 = 0; k1 = N/2; }
    if(find_child_num(key, it) == 3) {i1 = 0; j1 = N/2; k1 = N/2; }
    if(find_child_num(key, it) == 1) {i1 = N/2; j1 = N/2; k1 = N/2; }

    void *ptr = get_ht_data(0, it);
    BlockData<Point<Var, double>> *child_block = reinterpret_cast<BlockData<Point<Var, double>> *> (ptr);
    BlockData<Point<Var, double>> &temp_block = *child_block;

    for (i2 = i = 1; i < N/2; i++, i2+=2)
      for (j2 = j = 1; j < N/2; j++, j2+=2)
        for (k2 = k = 1; k < N/2; k++, k2+=2)
        {
          temp_block(i+i1, j+j1, k+k1) = temp_block(i2  , j2  , k2  ) +  
                                         temp_block(i2+1, j2  , k2  ) + 
                                         temp_block(i2  , j2+1, k2  ) + 
                                         temp_block(i2+1, j2+1, k2  ) + 
                                         temp_block(i2  , j2  , k2+1) + 
                                         temp_block(i2+1, j2  , k2+1) + 
                                         temp_block(i2  , j2+1, k2+1) + 
                                         temp_block(i2+1, j2+1, k2+1); 
        }  
  }

  Key consolidated_key{key.x, key.y, key.z, key.l, key.ts+1};
  insert_ht_data(0, consolidated_key, consolidated_block);

  return consolidated_key;

}

int refine(std::vector<Octant_Aggregator_Data>& aggData, std::vector<Key>& new_key_list)
{
  std::vector<Key> temp_list;
  Key temp;

  refine_level(aggData);
  
  for(auto it: aggData)
  {
    if(it.octant_key.refine == 1) 
    {
      temp_list = split_blocks(it.octant_key);
      for(auto new_key: temp_list)
      {
        new_key_list.push_back(new_key);
        insert_key(new_key, it.octant_key);
      }
    }
    else if(it.octant_key.refine == -1)
    {
      if(find_child_num(it.parent_key, it.octant_key) == 1) //only the first sibling initiates coarsening
      {
        temp = consolidate_blocks(it.parent_key);
        new_key_list.push_back(temp);
      }
    }
    else if(it.octant_key.refine == 0)
    {
      temp = it.octant_key;
      temp.ts++;
      temp.refine = -1;
      new_key_list.push_back(temp);
    }
       
  }

  return 0;
}

void exchange_faces(std::vector<Octant_Aggregator_Data>& aggData)
{
  for( auto& it: aggData)
  {
    std::vector<Key> neighbours[7];
    find_neigbours(aggData, it.parent_key, it.octant_key, neighbours);

    for(int direction = 1; direction < 7; direction++)
    {
      int face_num = -1;

      switch (direction)
      {
        case 1: face_num = 2; break; // bottom face of the top block
        case 2: face_num = 1; break; // top face of the bottom block
        case 3: face_num = 4; break; // left face of the right block
        case 4: face_num = 3; break; // right face of the left block
        case 5: face_num = 6; break; // backward face of the forward block
        case 6: face_num = 5; break; // forward face of the backward block
      }
      
  
      if(neighbours[direction].size() == 0) //no nighbours in this direction
      {
        ; //do nothing
      }
      else if(neighbours[direction].size() == 1) //neighbour at same or lower level. only one neighbour in this direction
      {
        Key nei_key = neighbours[direction].front();
        insert_ht_data(direction, it.octant_key, generate_face<Point<Var, double>>(face_num, nei_key));
      }
      else if(neighbours[direction].size() == 4) //neighbour at higher level. 4 neighbour in this direction
      {
        std::sort(neighbours[direction].begin(), neighbours[direction].end());
        BlockMatrix<Point<Var, double>> *face_ptr = new BlockMatrix<Point<Var, double>> (N, N);
        BlockMatrix<Point<Var, double>> &face = *face_ptr;

        int quadrant = 0, p = 0, q = 0;
        
        for( auto neig: neighbours[direction])
        {
          BlockMatrix<Point<Var, double>> *quad_ptr = generate_face<Point<Var, double>>(face_num, neig);
          BlockMatrix<Point<Var, double>> &quad = *quad_ptr;

          if(quadrant == 0) {p = 0; q = 0;}
          if(quadrant == 1) {p = 0; q = N/2;}
          if(quadrant == 2) {p = N/2; q = 0;}
          if(quadrant == 3) {p = N/2; q = N/2;}

          for (int j = 1; j < N/2; j += 2)
            for (int k = 1; k < N/2; k += 2)
              face(p+j, q+k) = quad(j, k) + quad(j, k+1) + quad(j+1, k) + quad(j+1, k+1);
          quadrant++;
        }
        insert_ht_data(direction, it.octant_key, face_ptr);

      }
    }
    
  }

}