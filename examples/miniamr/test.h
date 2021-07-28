#ifndef KEY_H
#define KEY_H
#include "key.h"
#endif /* KEY_H */

#ifndef AGGREGATORDATA_H
#define AGGREGATORDATA_H
#include "aggregatordata.h"
#endif /* AGGREGATORDATA_H */

#ifndef MINIAMR_H
#define MINIAMR_H
#include "miniamr.h"
#endif /* MINIAMR_H */

#ifndef POINT_H
#define POINT_H
#include "point.h"
#endif /* POINT_H */



void test_same_neighbours()
{
  std::vector<Octant_Aggregator_Data> testData;
  std::vector<Key> neighbours[7];

  testData.push_back(Octant_Aggregator_Data{ Key{0, 0, 0, 1, 1}, Key{0, 0, 0, 0, 1} } );
  testData.push_back(Octant_Aggregator_Data{ Key{0, 32, 0, 1, 1}, Key{0, 0, 0, 0, 1} } );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 0, 0, 1, 1}, Key{0, 0, 0, 0, 1} } );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 32, 0, 1, 1}, Key{0, 0, 0, 0, 1} } );
  testData.push_back(Octant_Aggregator_Data{ Key{0, 0, 32, 1, 1}, Key{0, 0, 0, 0, 1} } );
  testData.push_back(Octant_Aggregator_Data{ Key{0, 32, 32, 1, 1}, Key{0, 0, 0, 0, 1} } );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 0, 32, 1, 1}, Key{0, 0, 0, 0, 1} } );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 32, 32, 1, 1},Key{0, 0, 0, 0, 1} } );

  Key par_key{0, 0, 0, 0, 1};
  Key child_key{0, 0, 32, 1, 1};
  find_neigbours(testData, par_key, child_key, neighbours);

  for(int direction = 1; direction < 7; direction++)
  {
    printf("-----------NEIGHBOURS of (%d, %d, %d, %d, %d)---Direction %d \n", 
        child_key.x, child_key.y, child_key.z, child_key.l, child_key.ts, direction);

    for(auto it: neighbours[direction])
    {
       printf(" NEI (%d, %d, %d, %d, %d) \n", it.x, it.y, it.z, it.l, it.ts);
    }
  }
}


void test_lower_neighbours()
{
  std::vector<Octant_Aggregator_Data> testData;
  std::vector<Key> neighbours[7];

  testData.push_back(Octant_Aggregator_Data{ Key{0, 0, 0, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{0, 32, 0, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 0, 0, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 32, 0, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{0, 0, 32, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{0, 32, 32, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 0, 32, 1, 1}, Key{0, 0, 0, 0, 1}} );

  testData.push_back(Octant_Aggregator_Data{ Key{32, 32, 32, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 48, 32, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{48, 32, 32, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{48, 48, 32, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 32, 48, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 48, 48, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{48, 32, 48, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{48, 48, 48, 2, 1}, Key{32, 32, 32, 1, 1}} );
  
  
  

  Key par_key{32, 32, 32, 1, 1};
  Key child_key{32, 32, 32, 2, 1};
  find_neigbours(testData, par_key, child_key, neighbours);

  for(int direction = 1; direction < 7; direction++)
  {
    printf("-----------NEIGHBOURS of (%d, %d, %d, %d, %d)---Direction %d \n", 
        child_key.x, child_key.y, child_key.z, child_key.l, child_key.ts, direction);

    for(auto it: neighbours[direction])
    {
       printf(" NEI (%d, %d, %d, %d, %d) \n", it.x, it.y, it.z, it.l, it.ts);
    }
  }
}




void test_higher_neighbours()
{
  std::vector<Octant_Aggregator_Data> testData;
  std::vector<Key> neighbours[7];

  testData.push_back(Octant_Aggregator_Data{ Key{0, 0, 0, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{0, 32, 0, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 0, 0, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 32, 0, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{0, 0, 32, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{0, 32, 32, 1, 1}, Key{0, 0, 0, 0, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 0, 32, 1, 1}, Key{0, 0, 0, 0, 1}} );

  testData.push_back(Octant_Aggregator_Data{ Key{32, 32, 32, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 48, 32, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{48, 32, 32, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{48, 48, 32, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 32, 48, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{32, 48, 48, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{48, 32, 48, 2, 1}, Key{32, 32, 32, 1, 1}} );
  testData.push_back(Octant_Aggregator_Data{ Key{48, 48, 48, 2, 1}, Key{32, 32, 32, 1, 1}} );
  
  
  

  Key par_key{0, 0, 0, 0, 1};
  Key child_key{32, 0, 32, 1, 1};
  find_neigbours(testData, par_key, child_key, neighbours);

  for(int direction = 1; direction < 7; direction++)
  {
    printf("-----------NEIGHBOURS of (%d, %d, %d, %d, %d)---Direction %d \n", 
        child_key.x, child_key.y, child_key.z, child_key.l, child_key.ts, direction);

    for(auto it: neighbours[direction])
    {
       printf(" NEI (%d, %d, %d, %d, %d) \n", it.x, it.y, it.z, it.l, it.ts);
    }
  }
}




void test_point()
{
    Point<10, double> testpoint;
    testpoint = 2;
}

void test_intersection()
{
  Key key{0, 0 , 0, 1, 1};

  double c[3] = {0, 0, 0};
  double oc[3] = {0, 0, 0};
  double m[3] = {1, 1, 1};
  double om[3] = {1, 1, 1};
  double s = 2;
  double os = 2;
  double inc = 1;

  Object obj1(1, c, oc, m, om, s, os, inc );
  printf("test_intersection 1 =  %d \n", obj1.check_object_intersection(key));


  c[0] = c[1] = c[2] = 1;
  Object obj2(1, c, oc, m, om, s, os, inc );
  printf("test_intersection 2 =  %d \n", obj2.check_object_intersection(key));

  c[0] = c[1] = c[2] = 4;
  Object obj3(1, c, oc, m, om, s, os, inc );
  printf("test_intersection 3 =  %d \n", obj3.check_object_intersection(key));


}


void test_find_children()
{
  std::vector<Key> children_list;

  find_children_next_ts(Key{32, 0, 0, 1, 1}, children_list);
  for(auto it: children_list)
  {
     printf(" CHILDREN (%d, %d, %d, %d, %d) \n", it.x, it.y, it.z, it.l, it.ts);
  }


}

void test_parse()
{
  const std::lock_guard<std::mutex> lock( child_parent_mutex);
  printf(" $$$$$ START PARSE $$$$$$$$\n");

  for (auto it = child_parent_map.begin(); it != child_parent_map.end(); it++)
    printf(" PARSE CHILD(%d, %d, %d, %d, %d) : PARENT(%d, %d, %d, %d, %d)  HASH = %d\n", 
      it->first.x, it->first.y, it->first.z, it->first.l, it->first.ts,
      it->second.x, it->second.y, it->second.z, it->second.l, it->second.ts,
      it->first.hash());

  printf(" $$$$$ END PARSE $$$$$$$$\n");
}





