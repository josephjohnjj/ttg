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

  int l = 0, n = 0, s = 0;
  madness::hashT hash_val;

  Key() { rehash(); }
  Key(int l, int n, int s) : l(l), n(n), s(s) { rehash(); }

  madness::hashT hash() const { return hash_val; }
  void rehash() {
    hash_val = (static_cast<madness::hashT>(l) << 48) ^ (static_cast<madness::hashT>(n) << 32) ^ (s << 16);
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
  s << "Key(" << key.l << ", " << key.n << ")";
  return s;
}


auto make_node(ttg::Edge<Key, std::array<char, 20>>& edge) {
  auto f =
      [=](const Key& key, 
          std::array<char, 20> par_state_array, 
          std::tuple<ttg::Out<Key, std::array<char, 20>>>& out) {

        int next_l, next_n;
        unsigned char par_state_char[20];
        unsigned char my_state_char[20];
        std::array<char, 20> my_state_array;

        auto [l, n, s, h] = key; 

        //std::cout<<"node("<< l <<", "<< n << ")" << std::endl;
        auto world = ttg::ttg_default_execution_context();
        printf("node(%d, %d) rank %d\n", l, n, world.rank());

        for (int j = 0; j < par_state_array.size(); j++)
          par_state_char[j] = par_state_array[j];  // task receives at std::array. convert it to char*

        rng_spawn(par_state_char, my_state_char, s);


        for (int j = 0; j < my_state_array.size(); j++)  // convert char* my_state_char to std::array
          my_state_array[j] = my_state_char[j];

        int numChildren = uts_numChildren_bin(my_state_char);

        if (numChildren > 0) {
        {
          next_l = l + 1;
          for (int i = 0; i < numChildren; i++) 
          {
            next_n = (nonLeafBF * n) + i;
            ttg::send<0>(Key{next_l, next_n, i}, my_state_array, out);
          }
        }
        }
      };

  return ttg::wrap<Key>(f, ttg::edges(edge), ttg::edges(edge), "NODE", {"input_edge"}, {"output_edge"});
}


auto root(ttg::Edge<Key, std::array<char, 20>>& edge) {
  auto f =
      [=](const Key& key, std::tuple<ttg::Out<Key, std::array<char, 20>>>& out) {
        unsigned char my_state_char[20];
        std::array<char, 20> my_state_array;

        rng_init(my_state_char, rootId);  // initialise the SHA1

        for (int j = 0; j < my_state_array.size(); j++) 
          my_state_array[j] = my_state_char[j];  // edge sends at std::array

        int numChildren = (int)floor(b_0);

        auto world = ttg::ttg_default_execution_context();
        printf("root on rank %d\n", world.rank());

        for (int i = 0; i < numChildren; i++) 
          ttg::send<0>(Key{1, i, i}, my_state_array, out);
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

  ttg::ttg_initialize(argc, argv, -1);

  auto world = ttg::ttg_default_execution_context();

  ttg::Edge<Key, std::array<char, 20>> edge("edge");   
 
  auto op_root = root(edge);             
  auto op_node = make_node(edge); 

  auto keymap = [=](const Key& key) { return key.l %  world.size(); }; 
  op_node->set_keymap(keymap);

  auto connected = make_graph_executable(op_root.get());
  assert(connected);
  TTGUNUSED(connected);
  std::cout << "Graph is connected: " << connected << std::endl;

  

  if (world.rank() == 0) 
  {
    op_root->invoke(Key{0, 0, 0});
  }

  ttg::ttg_execute(world);
  ttg::ttg_fence(world);
  ttg::ttg_finalize();

  return 0;
}