#include "ttg.h"
using namespace ttg;
#include <madness/world/world.h>

#ifndef BLOCKMATRIX_H
#define BLOCKMATRIX_H
#include "blockmatrix.h"
#endif /* BLOCKMATRIX_H */
#include "distmatrix.h"

#include <parsec.h>
#include <parsec/data_internal.h>
#include <parsec/data_dist/matrix/matrix.h>
#include <parsec/data_dist/matrix/sym_two_dim_rectangle_cyclic.h>
#include <parsec/data_dist/matrix/two_dim_rectangle_cyclic.h>

#include <mkl.h>


int N = 4;          // rows in the global tiled matrix
int M = N;          // columns in the global tiled matrix
int NB = 4;         // rows rows in each tile
int MB = NB;        // rows columns in each tile
int nthreads = -1;  // threads

struct Key1 {
  //  (K, K)) is the diagonal tile coordiante 
  int K = 0;
  madness::hashT hash_val;

  Key1() { rehash(); }
  Key1(int K) : K(K){ rehash(); }

  madness::hashT hash() const { return hash_val; }
  void rehash() {
    hash_val = K;
  }

  // Equality test
  bool operator==(const Key1& b) const { return K == b.K; }

  // Inequality test
  bool operator!=(const Key1& b) const { return !((*this) == b); }

  template <typename Archive>
  void serialize(Archive& ar) {
    ar& madness::archive::wrap((unsigned char*)this, sizeof(*this));
  }
};

struct Key2 {
  // (I, J) is the tile coordiante 
  int I = 0, J = 0;
  madness::hashT hash_val;

  Key2() { rehash(); }
  Key2(int I, int J) : I(I), J(J){ rehash(); }

  madness::hashT hash() const { return hash_val; }
  void rehash() {
    hash_val = (static_cast<madness::hashT>(I) << 32)
             ^ (static_cast<madness::hashT>(J));
  }

  // Equality test
  bool operator==(const Key2& b) const { return I == b.I && J == b.J; }

  // Inequality test
  bool operator!=(const Key2& b) const { return !((*this) == b); }

  template <typename Archive>
  void serialize(Archive& ar) {
    ar& madness::archive::wrap((unsigned char*)this, sizeof(*this));
  }
};


struct Key3 {
  // ((I, J), K) where (I, J) is the tile coordiante and K is POTRF diagonal tile number
  int I = 0, J = 0, K = 0;
  madness::hashT hash_val;

  Key3() { rehash(); }
  Key3(int I, int J, int K) : I(I), J(J), K(K) { rehash(); }

  madness::hashT hash() const { return hash_val; }
  void rehash() {
    hash_val = (static_cast<madness::hashT>(I) << 48)
             ^ (static_cast<madness::hashT>(J) << 32)
             ^ (K << 16);
  }

  // Equality test
  bool operator==(const Key3& b) const { return I == b.I && J == b.J && K == b.K; }

  // Inequality test
  bool operator!=(const Key3& b) const { return !((*this) == b); }

  template <typename Archive>
  void serialize(Archive& ar) {
    ar& madness::archive::wrap((unsigned char*)this, sizeof(*this));
  }
};

namespace std {
  // specialize std::hash for Key

  template <>
  struct hash<Key1> {
    std::size_t operator()(const Key1& s) const noexcept { return s.hash(); }
  };

  template <>
  struct hash<Key2> {
    std::size_t operator()(const Key2& s) const noexcept { return s.hash(); }
  };

  template <>
  struct hash<Key3> {
    std::size_t operator()(const Key3& s) const noexcept { return s.hash(); }
  };

  std::ostream& operator<<(std::ostream& s, const Key1& key) {
    s << "Key(" << key.K << ")";
    return s;
  }

  std::ostream& operator<<(std::ostream& s, const Key2& key) {
    s << "Key(" << key.I << "," << key.J << ")";
    return s;
  }

  std::ostream& operator<<(std::ostream& s, const Key3& key) {
    s << "Key(" << key.I << "," << key.J << "," << key.K << ")";
    return s;
  }
  
}  // namespace std



template <typename T>
auto make_potrf(DistMatrix<T>& A,
               ttg::Edge<Key1, BlockMatrix<T>>& input_gemm,    // from GEMM
               ttg::Edge<Key2, BlockMatrix<T>>& output_trsm,   // to TRSM
               ttg::Edge<Key2, BlockMatrix<T>>& output_result) // write back
{
  auto f = [=](const Key1& key,
                     BlockMatrix<T>&&  tile_gemm, //from previous gemm with same coordinate
                     std::tuple<ttg::Out<Key2, BlockMatrix<T>>,
                                ttg::Out<Key2, BlockMatrix<T>>>& out){
    const int K = key.K;

    //printf("timestep %d: POTRF(%d) \n", K, K);
 
    int info = 0;
    int wT = tile_gemm.rows();
    //dpotrf_("Upper", &wT, tile_gemm.data(), &wT, &info);

    for(int n = K+1; n < A.rows(); n++) //syrk is replaced with gemm
      ttg::send<0>(Key2(n, K), tile_gemm, out); // to TRSMs
    
    ttg::send<1>(Key2(K, K), tile_gemm, out); // Write back
      
  };

  auto fg = [=](const Key1& key,
                     BlockMatrix<T>&  tile_gemm, //from previous gemm with same coordinate
                     std::tuple<ttg::Out<Key2, BlockMatrix<T>>,
                                ttg::Out<Key2, BlockMatrix<T>>>& out){

    int wT = tile_gemm.rows();
    return wT * wT * wT;
  };

  return ttg::wrapG<Key1>(f, fg, ttg::edges(input_gemm), ttg::edges(output_trsm, output_result),
                   "POTRF", {"input_gemm"}, {"output_trsm", "output_result"});
}


template <typename T>
auto make_trsm(DistMatrix<T>& A,
               ttg::Edge<Key2, BlockMatrix<T>>& input_potrf,    // from POTRF
               ttg::Edge<Key2, BlockMatrix<T>>& input_gemm,    // from previous GEMM
               ttg::Edge<Key3, BlockMatrix<T>>& output_gemm_ik,   // to GEMM
               ttg::Edge<Key3, BlockMatrix<T>>& output_gemm_jk,   // to GEMM
               ttg::Edge<Key2, BlockMatrix<T>>& output_result)   // write back
{
  auto f = [=](const Key2& key,
                     BlockMatrix<T>&  tile_potrf,
                     BlockMatrix<T>&& tile_gemm, //from previous gemm with same coordinate as this trsm
                     std::tuple<ttg::Out<Key3, BlockMatrix<T>>,
                                ttg::Out<Key3, BlockMatrix<T>>,
                                ttg::Out<Key2, BlockMatrix<T>>>& out){
    const int I = key.I;
    const int J = key.J;
    const int K = key.J; 
    int write = 0;

    //printf("timestep %d: TRSM(%d, %d)\n", K, I, J);

    
    if(!tile_gemm.is_empty())
    {
      int info = 0;
      write = 1;
      //cblas_dtrsm(CblasRowMajor, CblasRight, CblasLower, CblasTrans, 
		  //  CblasNonUnit, wT, wT, 1.0, tile_potrf.data(), wT, tile_gemm.data(), wT);
    }
    //else
    //  printf("SparseTRSM \n");
   

    // send the tile to  gemms (ik of that gemm) 
    for (int n = J+1; n <= I; ++n) 
      ttg::send<0>(Key3(I, n, K), tile_gemm, out);

    // send the tile to all gemms (jk of that gemm)
    for (int m = I; m < A.rows(); ++m) 
      ttg::send<1>(Key3(m, I, K), tile_gemm, out);


    if(write == 1)
      ttg::send<2>(Key2(I, J), tile_gemm, out); // Write back

    
  };

  auto fg = [=](const Key2& key,
                     BlockMatrix<T>&  tile_potrf,
                     BlockMatrix<T>&& tile_gemm, //from previous gemm with same ccordinate as this trsm
                     std::tuple<ttg::Out<Key3, BlockMatrix<T>>,
                                ttg::Out<Key3, BlockMatrix<T>>,
                                ttg::Out<Key2, BlockMatrix<T>>>& out){

    if(!tile_gemm.is_empty())
    {
      int wT = tile_gemm.rows();
      return wT * wT * wT;
    }

    return -1;
  };


  return ttg::wrapG(f, fg, ttg::edges(input_potrf, input_gemm), 
    ttg::edges(output_gemm_ik, output_gemm_jk, output_result),
    "TRSM", {"input_potrf", "input_gemm"}, {"output_gemm_ik", "output_gemm_jk", "output_result"});
}


template<typename T>
auto make_gemm(DistMatrix<T>& A,
               ttg::Edge<Key3, BlockMatrix<T>>& input_trsm_ik, //from TRSM
               ttg::Edge<Key3, BlockMatrix<T>>& input_trsm_jk, //from TRSM
               ttg::Edge<Key3, BlockMatrix<T>>& input_gemm_ij, //from GEMM
               ttg::Edge<Key1, BlockMatrix<T>>& output_potrf, // to POTRF
               ttg::Edge<Key2, BlockMatrix<T>>& output_trsm, // to TRSM
               ttg::Edge<Key3, BlockMatrix<T>>& output_gemm, // to GEMM
               ttg::Edge<Key2, BlockMatrix<T>>& output_result) // write back
{
  auto f = [=](const Key3& key,
               BlockMatrix<T>& tile_ik,
               BlockMatrix<T>& tile_jk,
               BlockMatrix<T>&& tile_ij, //from previous gemm with same coordinate
               std::tuple<ttg::Out<Key1, BlockMatrix<T>>,
                          ttg::Out<Key2, BlockMatrix<T>>,
                          ttg::Out<Key3, BlockMatrix<T>>,
                          ttg::Out<Key2, BlockMatrix<T>>>& out){
    
    const int I = key.I;
    const int J = key.J;
    const int K = key.K;
    int fill_flag = 0;

    //printf("timestep %d: GEMM(%d, %d)\n", K, I, J);


    if(!tile_ik.is_empty() && !tile_jk.is_empty())
    {
      if(tile_ij.is_empty())
      {
        tile_ij.fill(0);
        fill_flag = 1;
      }

        int info = 0;
        int wT = tile_ij.rows();
        //cblas_dgemm(CblasRowMajor, CblasNoTrans, CblasTrans, wT, wT, wT, 
			  //  -1.0, tile_ik.data(), wT, tile_jk.data(), wT, 1.0, tile_ij.data(), wT);
    }
    //else
    //  printf("SparseGEMM \n");
    

    if(fill_flag == 1)
        ttg::send<3>(Key2(K, K), tile_ij, out); // Write back the filled block

    if(I == (K+1) && J == (K+1) ) //send tile to the POTRF in the next step
      ttg::send<0>(Key1(K+1), tile_ij, out);
    else if(J == K+1 && I > K+1 ) //send tile to the TRSMs in the next step
      ttg::send<1>(Key2(I, J), tile_ij, out);
    else
      ttg::send<2>(Key3(I, J, K+1), tile_ij, out); //send tile to the GEMMs in the next step
    
  };

  auto fg = [=](const Key3& key,
               BlockMatrix<T>& tile_ik,
               BlockMatrix<T>& tile_jk,
               BlockMatrix<T>& tile_ij, //from previous gemm with same coordinate
               std::tuple<ttg::Out<Key1, BlockMatrix<T>>,
                          ttg::Out<Key2, BlockMatrix<T>>,
                          ttg::Out<Key3, BlockMatrix<T>>,
                          ttg::Out<Key2, BlockMatrix<T>>>& out){

    if(!tile_ik.is_empty() && !tile_jk.is_empty())
    {
      int wT = tile_ij.rows();
      return wT * wT * wT;
    }
       
    return -1;
  };

  return ttg::wrapG<Key3>(f, fg, ttg::edges(input_trsm_ik, input_trsm_jk, input_gemm_ij), 
          ttg::edges(output_potrf, output_trsm, output_gemm, output_result), "GEMM",
          {"input_trsm_ik", "input_trsm_jk", "input_gemm_ij"}, 
          {"output_potrf", "output_trsm", "output_gemm","output_result"});
}

template <typename T>
auto make_result(DistMatrix<T>& A, const ttg::Edge<Key2, BlockMatrix<T>>& result) 
{
  auto f = [=](const Key2& key, BlockMatrix<T>&& tile, std::tuple<>& out) {
    const int I = key.I;
    const int J = key.J;
    BlockMatrix<T>& current_tile = A(I, J);

    if(current_tile != tile) 
      std::copy_n(tile.data(), tile.rows() * tile.cols(), current_tile.data());
  };

  auto fg = [=](const Key2& key, BlockMatrix<T>&& tile, std::tuple<>& out) {
    return -1;
  };

  return ttg::wrapG(f, fg, ttg::edges(result), ttg::edges(), "Final Output", {"result"}, {});

}

template<typename T>
auto make_initiator(DistMatrix<T>& A,
               ttg::Edge<Key1, BlockMatrix<T>>& gemm_potrf, // to POTRF
               ttg::Edge<Key2, BlockMatrix<T>>& gemm_trsm,  // to TRSM
               ttg::Edge<Key3, BlockMatrix<T>>& gemm_gemm)  // to GEMM 
{
  auto f = [=](const Key3& key,
               std::tuple<ttg::Out<Key1, BlockMatrix<T>>,
                          ttg::Out<Key2, BlockMatrix<T>>,
                          ttg::Out<Key3, BlockMatrix<T>>>& out){

    /* kick off first POTRF */
    if (A.is_local(0, 0)) 
      ttg::send<0>(Key1(0), A(0, 0), out);

    for (int i = 1; i < A.rows(); i++) 
    {
      /* send gemm input to TRSM */
      if (A.is_local(i, 0)) 
          ttg::send<1>(Key2(i, 0), A(i, 0), out);
      
      for (int j = 1; j <= i; j++) 
      {
        /* send gemm to GEMM */
        if (A.is_local(i, j)) 
          ttg::send<2>(Key3(i, j, 0), A(i, j), out);
      }
    }
  };

  auto fg = [=](const Key3& key,
               std::tuple<ttg::Out<Key1, BlockMatrix<T>>,
                          ttg::Out<Key2, BlockMatrix<T>>,
                          ttg::Out<Key3, BlockMatrix<T>>>& out){

    return -1;                       
  };


  return ttg::wrapG<Key3>(f, fg, ttg::edges(), ttg::edges(gemm_potrf, gemm_trsm, gemm_gemm), "INITIATOR");
}


void potrf_parse_params(int argc, char *argv[]){
  int i = 1; 
  int err = -1;
  while (i < argc && err == -1) {
    
    switch (argv[i][1]) {
      case 'N':
        N = atoi(argv[i+1]); break;
      case 'M':
        M = atoi(argv[i+1]); break;
      case 'n':
        NB = atoi(argv[i+1]); break;
      case 'm':
        MB = atoi(argv[i+1]); break;
      case 'c':
        nthreads = atoi(argv[i+1]); break;
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

int main(int argc, char **argv)
{
  potrf_parse_params(argc, argv);

  ttg::ttg_initialize(0, nullptr, nthreads );
  auto world = ttg::ttg_default_execution_context();

  DistMatrix<double> DistMat(N, N, NB, NB);
  DistMat.init_matrix(1);

  ttg::Edge<Key3, BlockMatrix<double>> trsm_gemm_ik("trsm_gemm_ik");
  ttg::Edge<Key3, BlockMatrix<double>> trsm_gemm_jk("trsm_gemm_jk");
  ttg::Edge<Key3, BlockMatrix<double>> gemm_gemm("gemm_gemm_ij"); 
  ttg::Edge<Key1, BlockMatrix<double>> gemm_potrf("gemm_potrf"); 
  ttg::Edge<Key2, BlockMatrix<double>> gemm_trsm("gemm_trsm"); 
  ttg::Edge<Key2, BlockMatrix<double>> potrf_trsm("potrf_trsm"); 
  ttg::Edge<Key2 , BlockMatrix<double>> result("result"); 



  auto keymap1 = [&](const Key1& key) {
    return DistMat.rank_of(key.K, key.K);
  };

  auto keymap2 = [&](const Key2& key) {
    return DistMat.rank_of(key.I, key.J);
  };

  auto keymap3 = [&](const Key3& key) {
    return DistMat.rank_of(key.I, key.J);
  };

  auto keymap4 = [&](const Key3& key) {
    return world.rank();
  };

  auto op_gemm  = make_gemm(DistMat, trsm_gemm_ik, trsm_gemm_jk, gemm_gemm,
                                 gemm_potrf, gemm_trsm, gemm_gemm, result);
  op_gemm->set_keymap(keymap3);

  auto op_trsm  = make_trsm(DistMat, potrf_trsm, gemm_trsm,
                            trsm_gemm_ik, trsm_gemm_jk, result);
  op_trsm->set_keymap(keymap2);

  auto op_potrf = make_potrf(DistMat, gemm_potrf, potrf_trsm, result);
  op_potrf->set_keymap(keymap1);

  auto op_init  = make_initiator(DistMat, gemm_potrf, gemm_trsm, gemm_gemm);
  op_init->set_keymap(keymap4);

  auto op_result = make_result(DistMat, result);
  op_result->set_keymap(keymap2); 
  

  auto connected = make_graph_executable(op_init.get());
  assert(connected);
  TTGUNUSED(connected);
  std::cout << "Graph is connected: " << connected << std::endl;

  op_init->invoke(Key3{0, 0, 0});

  ttg::ttg_execute(world);
  ttg::ttg_fence(world);

  //delete A;
  /* cleanup allocated matrix before shutting down PaRSEC */
  //parsec_data_free(dcA.mat); dcA.mat = NULL;
  //parsec_tiled_matrix_dc_destroy( (parsec_tiled_matrix_dc_t*)&dcA);


  ttg::ttg_finalize();
  return 0;
}
