#include <array>
#include <cmath>
#include <cstdio>
#include "ttg.h"
using namespace ttg;
#include <madness/world/world.h>

#ifndef MINIAMR_H
#define MINIAMR_H
#include "miniamr.h"
#endif /* MINIAMR_H */

#include "test.h"

std::map<Key, Key> child_parent_map_copy;

auto make_aggregator(ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data1,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data2,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data3,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data4,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data5,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data6,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data7,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_data8,
                     ttg::Edge<Key, Key>& inject_task, 
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_OutData1,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_OutData2,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_OutData3,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_OutData4,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_OutData5,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_OutData6,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_OutData7,
                     ttg::Edge<Key, Octant_Aggregator_Data>& aggregator_OutData8) 
{

  auto f = [=](const Key& key,  Octant_Aggregator_Data& data1, Octant_Aggregator_Data& data2,
                                Octant_Aggregator_Data& data3, Octant_Aggregator_Data& data4,
                                Octant_Aggregator_Data& data5, Octant_Aggregator_Data& data6,
                                Octant_Aggregator_Data& data7,Octant_Aggregator_Data& data8, 
                                std::tuple<ttg::Out<Key, Key>,
                                           ttg::Out<Key, Octant_Aggregator_Data>, ttg::Out<Key, Octant_Aggregator_Data>,
                                           ttg::Out<Key, Octant_Aggregator_Data>, ttg::Out<Key, Octant_Aggregator_Data>,
                                           ttg::Out<Key, Octant_Aggregator_Data>, ttg::Out<Key, Octant_Aggregator_Data>,
                                           ttg::Out<Key, Octant_Aggregator_Data>,ttg::Out<Key, Octant_Aggregator_Data>>& out)
            {
              
              auto [x, y, z, l, ts, r, h] = key;
              if( x == 0 && y == 0 && z == 0 && l == 0) //root agrregator 
              {
                printf("******* MAPSIZE BEFORE %ld ******* \n", child_parent_map.size());
                printf("ROOT AGGREGATOR(x=%d, y=%d, z=%d, l=%d, ts=%d --HASH %lu-- ) \n", x, y, z, l, ts, h);
                
                key_edge_map.clear();
                
                if( ts < 2 )
                { 
                  printf("########################## TS %d ##################################\n", ts+1);

                  std::vector<Key> new_key_list;
                  exchange_faces(all_aggregated_data); //exchange ghost regions
                  refine(all_aggregated_data, new_key_list);
                  update_key_ts(ts+1); //update the ts of the keys in the map

                  all_aggregated_data.clear(); //new ts starts fresh
            

                  for(auto it: new_key_list)
                    ttg::send<0>(it, find_parent_key(it), out);

                  printf("******* MAPSIZE AFTER %ld ******* \n", child_parent_map.size());
                  
                }

                test_parse();

              }
              else 
              {
                Key rootkey = find_parent_key(key);
                //Key rootkey{0,0,0,0,2};
                Octant_Aggregator_Data aggData{key, rootkey};
                int edge = edge_number(rootkey) + 1;

                printf("*** INTREMEDIATE AGGREGATOR (x=%d, y=%d, z=%d, l=%d, ts=%d) --->  (x=%d, y=%d, z=%d, l=%d, ts=%d)\n", 
                  key.x, key.y, key.z, key.l, key.ts,
                  rootkey.x, rootkey.y, rootkey.z, rootkey.l, rootkey.ts);
             
                switch (edge)
                {
                  case 1: ttg::send<1>(rootkey, aggData, out); break;
                  case 2: ttg::send<2>(rootkey, aggData, out); break;
                  case 3: ttg::send<3>(rootkey, aggData, out); break;
                  case 4: ttg::send<4>(rootkey, aggData, out); break;
                  case 5: ttg::send<5>(rootkey, aggData, out); break;
                  case 6: ttg::send<6>(rootkey, aggData, out); break;
                  case 7: ttg::send<7>(rootkey, aggData, out); break;
                  case 8: ttg::send<8>(rootkey, aggData, out); break;
                  default: printf("Something is wrong 2\n"); break;
                }   
              }
            };

  return ttg::wrap<Key>(f, 
                        ttg::edges(aggregator_data1, aggregator_data2, aggregator_data3, aggregator_data4, 
                                   aggregator_data5,aggregator_data6,aggregator_data7,aggregator_data8), 
                        ttg::edges(inject_task, 
                                   aggregator_OutData1, aggregator_OutData2, aggregator_OutData3, aggregator_OutData4,
                                   aggregator_OutData5, aggregator_OutData6, aggregator_OutData7, aggregator_OutData8),
                        "AGREGATOR", 
                        {"octant_aggregator_data_1", "octant_aggregator_data_2", "octant_aggregator_data_3","octant_aggregator_data_4",
                         "octant_aggregator_data_5", "octant_aggregator_data_6", "octant_aggregator_data_7","octant_aggregator_data_8"}, 
                        {"inject_task", 
                         "aggregator_Out1", "aggregator_Out2", "aggregator_Out3", "aggregator_Out4",
                         "aggregator_Out5", "aggregator_Out6", "aggregator_Out7", "aggregator_Out8"});
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

  auto f = [=](const Key& key,  Key& parent_key, 
              std::tuple<ttg::Out<Key, Octant_Aggregator_Data>, ttg::Out<Key, Octant_Aggregator_Data>,
                         ttg::Out<Key, Octant_Aggregator_Data>, ttg::Out<Key, Octant_Aggregator_Data>,
                         ttg::Out<Key, Octant_Aggregator_Data>, ttg::Out<Key, Octant_Aggregator_Data>,
                         ttg::Out<Key, Octant_Aggregator_Data>, ttg::Out<Key, Octant_Aggregator_Data>>& out)
            {
             
              stencil<Point<Var, double>>(key); //TODO: change stencil
              Key oct_key = key;
              oct_key.refine = check_object_intersection(key, rectangle);
              //oct_key.refine = 0;
           

              //if(oct_key.refine == 1)
              //{
              //  printf(" INTERSECTION for OCT(x=%d, y=%d, z=%d, l=%d, ts=%d) \n",
              //    key.x, key.y, key.z, key.l, key.ts);
              //}


              Key aggKey = find_parent_key(key);
              Octant_Aggregator_Data aggData{oct_key, aggKey};
              insert_aggregated_data(aggData);
              int edge = edge_number(aggKey);

              printf(" OCT(x=%d, y=%d, z=%d, l=%d, ts=%d) from PAR(x=%d, y=%d, z=%d, l=%d, ts=%d) to AGG(x=%d, y=%d, z=%d, l=%d, ts=%d) \n",
              key.x, key.y, key.z, key.l, key.ts,
              parent_key.x, parent_key.y, parent_key.z, parent_key.l, parent_key.ts,
              aggKey.x, aggKey.y, aggKey.z, aggKey.l, aggKey.ts);  
                
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

  return ttg::wrap<Key>(f, 
                        ttg::edges(treeParent_treeChild), 
                        ttg::edges(aggregator_data1, aggregator_data2, aggregator_data3, aggregator_data4,
                                   aggregator_data5, aggregator_data6, aggregator_data7, aggregator_data8 ), 
                        "OCTANT", 
                        {"parent"}, 
                        {"octant_aggregator1", "octant_aggregator2", "octant_aggregator3", "octant_aggregator4",
                         "octant_aggregator5", "octant_aggregator6", "octant_aggregator7", "octant_aggregator8"});
}     

auto make_initiator(ttg::Edge<Key, Key>& initiator) 
{

  auto f = [=](const Key& key, 
               std::tuple<ttg::Out<Key, Key>>& out)
            {
              printf("INITIATOR --HASH %lu--  \n", key.hash_val);


              create_ht(); 
              BlockData<Point<Var, double>> *block = new BlockData<Point<Var, double>> (N, N, N);
              block->fill();
              insert_ht_data(0, key, static_cast<void*>(block));
              rectangle = initialise_object();

              std::vector<Key> new_keys_list = split_blocks(key);

              //Key temp = key;
              //temp.ts++;
              for(auto new_key: new_keys_list)
                insert_key(new_key, key);

              update_key_ts(1);
              //test_parse();

              printf("########################## TS 1 ##################################\n");

              for(auto new_key: new_keys_list)
                ttg::send<0>(new_key, find_parent_key(new_key), out);
      
            };

  return ttg::wrap<Key>(f, ttg::edges(), ttg::edges(initiator), "INITIATOR", {}, {"initiator"});
} 


int main(int argc, char** argv) 
{
  mask = pow(2, d-1) - 1;
  N = pow(2, d); //dimension of a blocks is NxNxN.

  child_parent_map.clear();
  key_edge_map.clear();
  

  printf("MINIMAR: d = %d, mask = %d, N = %d \n", d, mask, N);
  

  ttg::ttg_initialize(argc, argv, -1);
  auto world = ttg::ttg_default_execution_context();

  ttg::Edge<Key, Key> treeParent_treeChild("treeParent_treeChild");  
  ttg::Edge<Key, Octant_Aggregator_Data> aggregator_data1("aggregator_data1"), aggregator_data2("aggregator_data2"),
        aggregator_data3("aggregator_data3"), aggregator_data4("aggregator_data4") ,aggregator_data5("aggregator_data5"),
        aggregator_data6("aggregator_data6"), aggregator_data7("aggregator_data7"), aggregator_data8("aggregator_data8"); 
 
  auto op_initiator = make_initiator(treeParent_treeChild);

  auto op_octant = make_octant(treeParent_treeChild, 
                               aggregator_data1, aggregator_data2, aggregator_data3, aggregator_data4, 
                               aggregator_data5, aggregator_data6, aggregator_data7, aggregator_data8); 

  auto op_aggregator = make_aggregator(aggregator_data1, aggregator_data2, aggregator_data3, aggregator_data4, 
                                       aggregator_data5, aggregator_data6,aggregator_data7, aggregator_data8, 
                                       treeParent_treeChild, 
                                       aggregator_data1, aggregator_data2, aggregator_data3, aggregator_data4, 
                                       aggregator_data5, aggregator_data6, aggregator_data7, aggregator_data8);              
  
  auto keymap = [=](const Key& key) { return 0; }; 
  op_octant->set_keymap(keymap);
  op_initiator->set_keymap(keymap);
  op_aggregator->set_keymap(keymap);

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