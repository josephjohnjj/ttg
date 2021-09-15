#include <array>
#include <cmath>
#include <cstdio>
#include "ttg.h"
using namespace ttg; 
#include <madness/world/world.h>
#include "taskbench.h"

int points = 40;                   // number of sweep points in one layer
int levels  = 1;                   // levels in sweep  
int computeGranularity = 1;        // granularity of a task
const int data_size = 1;           // size of the data
int num_threads = -1;              // threads in a process


struct Key {
  // (l, n) where l is the level of the node in the tree and
  //              m is the position of the node in that level


  long long int l = 0, n = 0;
  madness::hashT hash_val;

  Key() { rehash(); }
  Key(long long int l, long long int n) : l(l), n(n){ rehash(); }
 
  madness::hashT hash() const { return hash_val; }
  void rehash() {
    hash_val = (static_cast<madness::hashT>(l) << 32) ^ (static_cast<madness::hashT>(n) << 16);
  }

  bool operator==(const Key& b) const { return l == b.l && n == b.n; }
  bool operator!=(const Key& b) const { return !((*this) == b); }

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
  s << "Key(" << key.l << ", " << key.n << ")";
  return s;
}


auto make_sweep(ttg::Edge<Key, std::array<char, data_size>>& input_left, 
                  ttg::Edge<Key, std::array<char, data_size>>& input_up,
                  ttg::Edge<Key, std::array<char, data_size>>& output_left,
                  ttg::Edge<Key, std::array<char, data_size>>& output_up){
  auto f =
      [=](const Key& key, std::array<char, data_size>& data_left, 
                          std::array<char, data_size>& data_up,  
                                        std::tuple<ttg::Out<Key, std::array<char, data_size>>,
                                                   ttg::Out<Key, std::array<char, data_size>>>& out) {
        
        compute_kernel(computeGranularity);

        auto world = ttg::ttg_default_execution_context();
        printf(" I am Task(%d, %d) on node %d \n", key.l, key.n, world.rank() );

        if(key.n < points)
          ttg::send<1>(Key{key.l, key.n + 1}, data_up, out); 
        if(key.l <= levels)
          ttg::send<1>(Key{key.l + 1, key.n}, data_up, out);
    };

  return ttg::wrap<Key>(f, ttg::edges(input_left, input_up), 
                           ttg::edges(output_left, output_up), 
                           "SWEEP", 
                           {"input_left", "input_up"}, 
                           {"output_left", "output_ip"});
}


auto make_initiator(ttg::Edge<Key, std::array<char, data_size>>& output_left, 
                    ttg::Edge<Key, std::array<char, data_size>>& output_up){
  auto f =
      [=](const Key& key, std::tuple<ttg::Out<Key, std::array<char, data_size>>,
                                     ttg::Out<Key, std::array<char, data_size>>>& out) {

        std::array<char, data_size> data;

        for(int i = 1; i <= levels; i++)
          ttg::send<0>(Key{i, 0}, data, out); 

        for(int j = 0; j < points; j++)
          ttg::send<1>(Key{0, j}, data, out);

      };

  return ttg::wrap<Key>(f, ttg::edges(), ttg::edges(output_left, output_up), 
                            "INITIATOR", {}, {"output_left", "output_up"});
}


void sweepl_parse_params(int argc, char *argv[]){
  int i = 1; 
  int err = -1;
  while (i < argc && err == -1) {
    
    switch (argv[i][1]) 
    {
      case 'p':
        points = atoi(argv[i+1]); break;
      case 'l':
        levels = atoi(argv[i+1]); break;
      case 'g':
        computeGranularity = std::max(1,atoi(argv[i+1])); break;
      case 'c':
        num_threads = atoi(argv[i+1]); break;
      default:
        err = i;
    }

    if (err != -1) break;

    i += 2;
  }

  if (err != -1) {
    printf("Unrecognized parameter or incorrect/missing value: '%s %s'\n", argv[i], (i+1 < argc) ? argv[i+1] : "[none]");
    exit(0);
  }
}

int main(int argc, char** argv) {

  sweepl_parse_params(argc, argv);
  ttg::ttg_initialize(0, nullptr, num_threads );      
  auto world = ttg::ttg_default_execution_context();

  ttg::Edge<Key, std::array<char, data_size>> left("left"); 
  ttg::Edge<Key, std::array<char, data_size>> up("up");   

  auto op_initiator = make_initiator(left, up);     
  auto op_sweep = make_sweep(left, up, left, up); 
  

  auto keymap = [=](const Key& key) { 
    int chunk = points / world.size();
    return (key.n / chunk) % world.size();
  };

  op_sweep->set_keymap(keymap);
  

  auto connected = make_graph_executable(op_initiator.get());
  assert(connected);
  TTGUNUSED(connected);
  std::cout << "Graph is connected: " << connected << std::endl;

  if (world.rank() == 0) 
    op_initiator->invoke(Key{0, 0});

  ttg::ttg_execute(world);
  ttg::ttg_fence(world);
  ttg::ttg_finalize();

  return 0;
}