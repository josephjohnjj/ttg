#include <array>
#include <cmath>
#include <cstdio>
#include "ttg.h"
using namespace ttg;
#include <madness/world/world.h>

//#include "uts.h"

extern "C" {
#ifndef __cplusplus
#define __cplusplus 
#endif
#include "rng/brg_sha1.h"
}


/*  Tree type BIN (BINOMIAL)
 *  The branching factor at the root is specified by b_0.
 *  The branching factor below the root follows an
 *     identical binomial distribution at all nodes.
 *  A node has m children with prob q, or no children with
 *     prob (1-q).  The expected branching factor is q * m.
 *
 *  Default parameter values
 */
double b_0 = 4.0;                  // default branching factor at the root
int nonLeafBF = 4;                 // m
double nonLeafProb = 15.0 / 64.0;  // q
int rootId = 0;                    // default seed for RNG state at root
int computeGranularity = 1;        //granularity of a task
int num_threads = -1;              // threads in a process
int keymap_option = 1;

// Interpret 32 bit positive integer as value on [0,1)
double rng_toProb(int n) {
  if (n < 0) {
    printf("*** toProb: rand n = %d out of range\n", n);
  }
  return ((n < 0) ? 0.0 : ((double)n) / 2147483648.0);
}

// find the number of children for each node binary distribution
int uts_numChildren_bin(unsigned char state_char[]) {
  // distribution is identical everywhere below root
  int v = rng_rand(state_char);
  double d = rng_toProb(v);

  return (d < nonLeafProb) ? nonLeafBF : 0;
}

struct Key {
  // (l, n, s) where l is the level of the node in the tree and
  //                 m is the position of the node in that level
  //                 s is the spwan number, ie, it is the sth spawn of its parent
  //                 p is the node where the parent was executed.

  long long int l = 0, n = 0;
  int s, p; 
  madness::hashT hash_val;

  Key() { rehash(); }
  Key(long long int l, long long int n, int s) : l(l), n(n), s(s), p(0){ rehash(); }
  Key(long long int l, long long int n, int s, int p) : l(l), n(n), s(s), p(p){ rehash(); }

  madness::hashT hash() const { return hash_val; }
  void rehash() {
    hash_val = (static_cast<madness::hashT>(l) << 32) ^ (static_cast<madness::hashT>(n) << 16) ^ (s << 8);
  }

  // Equality test
  bool operator==(const Key& b) const { return l == b.l && n == b.n; }
  // Inequality test
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
  s << "Key(" << key.l << ", " << key.n << ", " << key.s << ", " << key.p << ")";
  return s;
}


auto make_node(ttg::Edge<Key, std::array<char, 20>>& edge) {
  auto f =
      [=](const Key& key, 
          std::array<char, 20> par_state_array, 
          std::tuple<ttg::Out<Key, std::array<char, 20>>>& out) {

        int next_l, next_n;
        std::array<char, 20> my_state_array;
        std::vector<Key> keylist;
        keylist.reserve(nonLeafBF);
        auto [l, n, s, p, h] = key; 

        auto world = ttg::ttg_default_execution_context();
        //printf("node(%lld, %lld) rank %d\n", l, n, world.rank());

        RNG_state* my_state_ptr = reinterpret_cast<RNG_state*>(my_state_array.data());
        RNG_state* par_state_ptr = reinterpret_cast<RNG_state*>(par_state_array.data());

        for (int i = 0; i < computeGranularity; i++) //just for granularuty purpose
          rng_spawn(par_state_ptr, my_state_ptr, s);

        int numChildren = uts_numChildren_bin(my_state_ptr);

        if (numChildren > 0) 
        {
          next_l = l + 1;
          for (int i = 0; i < numChildren; i++) 
          {
            next_n = (nonLeafBF * n) + i;
            keylist.push_back(Key{next_l, next_n, i, world.rank()});
          }
        }

        for(auto it: keylist)
          ttg::send<0>(it, my_state_array, out);
    };

  return ttg::wrap<Key>(f, ttg::edges(edge), ttg::edges(edge), "NODE", {"input_edge"}, {"output_edge"});
}


auto root(ttg::Edge<Key, std::array<char, 20>>& edge) {
  auto f =
      [=](const Key& key, std::tuple<ttg::Out<Key, std::array<char, 20>>>& out) {

        unsigned char my_state_char[20];
        std::array<char, 20> my_state_array;
        std::vector<Key> keylist;
        keylist.reserve(b_0);

        rng_init(reinterpret_cast<RNG_state*>(my_state_array.data()), rootId);  // initialise the SHA1

        auto world = ttg::ttg_default_execution_context();
        //printf("root on rank %d \n", world.rank());

        int numChildren = (int)floor(b_0);
        for (int i = 0; i < numChildren; i++) 
          keylist.push_back(Key{1, i, i, world.rank()});

        for(auto it: keylist)
          ttg::send<0>(it, my_state_array, out);

      };

  return ttg::wrap<Key>(f, ttg::edges(), ttg::edges(edge), "ROOT", {}, {"root_edge"});
}


void uts_parseParams(int argc, char *argv[]){
  int i = 1; 
  int err = -1;
  while (i < argc && err == -1) {
    
    switch (argv[i][1]) {
      case 'q':
        nonLeafProb = atof(argv[i+1]); break;
      case 'm':
        nonLeafBF = atoi(argv[i+1]); break;
      case 'r':
        rootId = atoi(argv[i+1]); break;
      case 'b':
        b_0 = atof(argv[i+1]); break;
      case 'g':
        computeGranularity = std::max(1,atoi(argv[i+1])); break;
      case 'c':
        num_threads = atoi(argv[i+1]); break;
      case 'k':
        keymap_option = atoi(argv[i+1]); break;
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

  uts_parseParams(argc, argv);
  ttg::ttg_initialize(0, nullptr, num_threads );
  auto world = ttg::ttg_default_execution_context();

  ttg::Edge<Key, std::array<char, 20>> edge("edge");   
 
  auto op_root = root(edge);             
  auto op_node = make_node(edge); 

  auto keymap1 = [=](const Key& key) { 
    return key.p; // map the tasks to the same node as the parent task
  };

  auto keymap2 = [=](const Key& key) { 
    if(key.l == 1)
      return key.s % world.size();
    return (key.p + key.s) % world.size(); // map the tasks to the nodes next to in round robin fashion
  };

  if(keymap_option == 1)
    op_node->set_keymap(keymap1);
  if(keymap_option == 2)
    op_node->set_keymap(keymap2);

  auto connected = make_graph_executable(op_root.get());
  assert(connected);
  TTGUNUSED(connected);
  std::cout << "Graph is connected: " << connected << std::endl;

  if (world.rank() == 0) 
    op_root->invoke(Key{0, 0, 0});

  ttg::ttg_execute(world);
  ttg::ttg_fence(world);
  ttg::ttg_finalize();

  return 0;
}