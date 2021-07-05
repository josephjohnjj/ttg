#include <array>
#include <cmath>
#include <cstdio>
#include "ttg.h"
using namespace ttg;
#include <madness/world/world.h>
#include "miniamr.h"




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
              
              auto [x, y, z, l, ts, h] = key;
              if( x == 0 && y == 0 && z == 0) //root agrregator 
              {
                printf("ROOT AGGREGATOR(x=%d, y=%d, z=%d, l=%d, ts=%d --HASH %lu-- ) \n", x, y, z, l, ts, h);
                
                if( ts < TotTs)
                {

                  refinement_parse(); //coordinate refinement decisions
                  update_key_ts(); //update the ts of the keys in the map

                  key_edge_map.clear();
                  std::map<Key, Octant_Aggregator_Data> refinement_copy = refinement;
                  refinement.clear();
                  

                  for( auto it = refinement_copy.begin(); it != refinement_copy.end(); it++)
                  {
                    Key new_octkey, new_parkey;
                    Key octkey = it->second.octant_key;
                    Key parkey = it->second.parent_key;

                    if(octkey.l == it->second.next_level)
                    {
                      new_octkey = Key{octkey.x, octkey.y, octkey.z, octkey.l, octkey.ts+1};
                      new_parkey = find_key(new_octkey);
                      insert_key(new_octkey, new_parkey);
                      ttg::send<0>(new_octkey, parkey, out);

                    }
                    else
                    {

                      new_parkey = Key{octkey.x, octkey.y, octkey.z, octkey.l, octkey.ts+1};
                      int D = N / pow(2, new_parkey.l+2);

                      new_octkey = {octkey.x, octkey.y, octkey.z, octkey.l+1, octkey.ts+1};
                      insert_key(new_octkey, new_parkey);
                      ttg::send<0>(new_octkey, new_parkey, out);

                      new_octkey = {octkey.x + D, octkey.y, octkey.z, octkey.l+1, octkey.ts+1};
                      insert_key(new_octkey, new_parkey);
                      ttg::send<0>(new_octkey, new_parkey, out);

                      new_octkey = {octkey.x, octkey.y + D, octkey.z, octkey.l+1, octkey.ts+1};
                      insert_key(new_octkey, new_parkey);
                      ttg::send<0>(new_octkey, new_parkey, out);

                      new_octkey = {octkey.x + D, octkey.y + D, octkey.z, octkey.l+1, octkey.ts+1};
                      insert_key(new_octkey, new_parkey);
                      ttg::send<0>(new_octkey, new_parkey, out);

                      new_octkey = {octkey.x, octkey.y, octkey.z + D, octkey.l+1, octkey.ts+1};
                      insert_key(new_octkey, new_parkey);
                      ttg::send<0>(new_octkey, new_parkey, out);

                      new_octkey = {octkey.x + D, octkey.y, octkey.z + D, octkey.l+1, octkey.ts+1};
                      insert_key(new_octkey, new_parkey);
                      ttg::send<0>(new_octkey, new_parkey, out);

                      new_octkey = {octkey.x, octkey.y + D, octkey.z + D, octkey.l+1, octkey.ts+1};
                      insert_key(new_octkey, new_parkey);
                      ttg::send<0>(new_octkey, new_parkey, out);

                      new_octkey = {octkey.x + D, octkey.y + D, octkey.z + D, octkey.l+1, octkey.ts+1};
                      insert_key(new_octkey, new_parkey);
                      ttg::send<0>(new_octkey, new_parkey, out);


                    }
                  }
                }

              }
              else 
              {
                Key rootkey = find_key(key);
                Octant_Aggregator_Data aggData{key, rootkey, rootkey.l};
                int edge = edge_number(rootkey) + 1;

                printf("*** INTREMEDIATE AGGREGATOR (x=%d, y=%d, z=%d, l=%d, ts=%d) --->  (x=%d, y=%d, z=%d, l=%d, ts=%d) EDGE-%d \n", 
                  key.x, key.y, key.z, key.l, key.ts,
                  rootkey.x, rootkey.y, rootkey.z, rootkey.l, rootkey.ts,
                  edge);
             
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
              
              Key aggKey = find_key(key);
              Octant_Aggregator_Data aggData{key, parent_key, parent_key.l};
              int edge = edge_number(aggKey);

              stencil(key, aggKey, edge);
              refinement_decision(key, parent_key);

              

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

              int D = N / pow(2, key.l+1);

              Key child_key;

              child_key = {key.x, key.y, key.z, key.l, key.ts};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x+D, key.y, key.z, key.l, key.ts};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x, key.y+D, key.z, key.l, key.ts};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x+D, key.y+D, key.z, key.l, key.ts};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x, key.y, key.z+D, key.l, key.ts};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);
              
              child_key = {key.x+D, key.y, key.z+D, key.l, key.ts};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x, key.y+D, key.z+D, key.l, key.ts};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);

              child_key = {key.x+D, key.y+D, key.z+D, key.l, key.ts};
              insert_key(child_key, key);
              ttg::send<0>(child_key, key, out);
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