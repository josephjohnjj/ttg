#include "ttg.h"
using namespace ttg;
#include <madness/world/world.h>
#include "blockmatrix.h"

#include <parsec.h>
#include <parsec/data_internal.h>
#include <parsec/data_dist/matrix/matrix.h>
#include <parsec/data_dist/matrix/sym_two_dim_rectangle_cyclic.h>
#include <parsec/data_dist/matrix/two_dim_rectangle_cyclic.h>

//#include <mkl_cblas.h>

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

struct Key4 {
  // ((I, J), K) where (I, J) is the tile coordiante and K is POTRF diagonal tile number
  int I = 0, J = 0, K = 0;
  madness::hashT hash_val;

  Key4() { rehash(); }
  Key4(int I, int J, int K) : I(I), J(J), K(K) { rehash(); }

  madness::hashT hash() const { return hash_val; }
  void rehash() {
    hash_val = (static_cast<madness::hashT>(I) << 48)
             ^ (static_cast<madness::hashT>(J) << 32)
             ^ (K << 16);
  }

  // Equality test
  bool operator==(const Key4& b) const { return I == b.I && J == b.J && K == b.K; }

  // Inequality test
  bool operator!=(const Key4& b) const { return !((*this) == b); }

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

  template <>
  struct hash<Key4> {
    std::size_t operator()(const Key4& s) const noexcept { return s.hash(); }
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

  std::ostream& operator<<(std::ostream& s, const Key4& key) {
    s << "Key(" << key.I << "," << key.J << "," << key.K << ")";
    return s;
  }
}  // namespace std



template <typename T>
auto make_potrf(DistMatrix<T>& A,
               ttg::Edge<Key1, BlockMatrix<T>>& input_gemm,    // from GEMM
               ttg::Edge<Key2, BlockMatrix<T>>& output_trsm,   // to TRSM
               ttg::Edge<Key3, BlockMatrix<T>>& output_result) // write back
{
  auto f = [=](const Key1& key,
                     BlockMatrix<T>&  tile_gemm,
                     std::tuple<ttg::Out<Key2, BlockMatrix<T>>,
                                ttg::Out<Key3, BlockMatrix<T>>>& out){
    const int K = key.K;
 
    //do the operation

    for(int n = K+1; n < A.rows(); n++) //syrk is replaced with gemm
      ttg::send<0>(Key2(n, K), tile_gemm, out); // to TRSMs
      
  };
  return ttg::wrap<Key1>(f, ttg::edges(input_gemm), ttg::edges(output_trsm, output_result),
                   "POTRF", {"input_gemm"}, {"output_trsm", "output_result"});
}

template <typename T>
auto make_forwarder_ik(DistMatrix<T>& A,
               ttg::Edge<Key4 , Key3>& input_trsm,         // from TRSM
               ttg::Edge<Key3, BlockMatrix<T>>& output_gemm)  // to GEMM
{
  auto f = [=](const Key4& key,
                     Key3& gemm_key,
                     std::tuple<ttg::Out<Key3, BlockMatrix<T>>>& out){
    const int I = key.I;
    const int J = key.J;
    const int K = key.K; 


    ttg::send<0>(gemm_key, A(I, J), out); 

    
  };
  return ttg::wrap(f, ttg::edges(input_trsm), ttg::edges(output_gemm), "FORWARDER_IK", 
    {"input_trsm"}, {"output_gemm"});
}

template <typename T>
auto make_forwarder_kj(DistMatrix<T>& A,
               ttg::Edge<Key4 , Key3>& input_trsm,         // from TRSM
               ttg::Edge<Key3, BlockMatrix<T>>& output_gemm)  // to GEMM
{
  auto f = [=](const Key4& key,
                     Key3& gemm_key,
                     std::tuple<ttg::Out<Key3, BlockMatrix<T>>>& out){
    const int I = key.I;
    const int J = key.J;
    const int K = key.K; 


    ttg::send<0>(gemm_key, A(I, J), out); 

    
  };
  return ttg::wrap(f, ttg::edges(input_trsm), ttg::edges(output_gemm), "FORWARDER_KJ", 
    {"input_trsm"}, {"output_gemm"});
}

template <typename T>
auto make_forwarder_ij(DistMatrix<T>& A,
               ttg::Edge<Key4 , Key3>& input_trsm,     // from TRSM
               ttg::Edge<Key3, BlockMatrix<T>>& output_gemm)  // to GEMM
{
  auto f = [=](const Key4& key,
                     Key3& gemm_key,
                     std::tuple<ttg::Out<Key3, BlockMatrix<T>>>& out){
    const int I = key.I;
    const int J = key.J;
    const int K = key.K; 


    ttg::send<0>(gemm_key, A(I, J), out); 

    
  };
  return ttg::wrap(f, ttg::edges(input_trsm), ttg::edges(output_gemm), "FORWARDER_IJ", 
    {"input_trsm"}, {"output_gemm"});
}


template <typename T>
auto make_trsm(DistMatrix<T>& A,
               ttg::Edge<Key2, BlockMatrix<T>>& input_potrf,    // from POTRF
               ttg::Edge<Key2, BlockMatrix<T>>& input_gemm,    // from previous GEMM
               ttg::Edge<Key4, Key3>& output_forwarder_ik,   // to forwarder_ik
               ttg::Edge<Key4, Key3>& output_forwarder_kj,   // to forwarder_kj
               ttg::Edge<Key4, Key3>& output_forwarder_ij,   // to forwarder_ij
               ttg::Edge<Key3, BlockMatrix<T>>& output_result)
{
  auto f = [=](const Key2& key,
                     BlockMatrix<T>&  tile_potrf,
                     BlockMatrix<T>&& tile_gemm,
                     std::tuple<ttg::Out<Key4, Key3>,
                                ttg::Out<Key4, Key3>,
                                ttg::Out<Key4, Key3>,
                                ttg::Out<Key3, BlockMatrix<T>>>& out){
    const int I = key.I;
    const int J = key.J;
    const int K = key.J; 

    //do the operation

    for(int n = J; n <= I; n++) //syrk is replaced with gemm
    {
      ttg::send<0>(Key4(J, K, K), Key3(J, n, K), out); //forwarder_ik
      ttg::send<1>(Key4(K, n, K), Key3(J, n, K), out); //forwarder_kj
      ttg::send<2>(Key4(J, n, K), Key3(J, n, K), out); //forwarder_ij
    }

    
  };
  return ttg::wrap(f, ttg::edges(input_potrf, input_gemm), 
    ttg::edges(output_forwarder_ik, output_forwarder_kj, output_forwarder_ij, output_result),
    "TRSM", {"input_potrf", "input_gemm"}, 
    {"output_forwarder_kj", "output_forwarder_kj", "output_forwarder_ij", "output_result"});
}


template<typename T>
auto make_gemm(DistMatrix<T>& A,
               ttg::Edge<Key3, BlockMatrix<T>>& input_ik, //from forwarder_ik
               ttg::Edge<Key3, BlockMatrix<T>>& input_kj, //from forwarder_kj
               ttg::Edge<Key3, BlockMatrix<T>>& input_ij, //from forwarder_ij
               ttg::Edge<Key1, BlockMatrix<T>>& output_potrf, // to POTRF
               ttg::Edge<Key2, BlockMatrix<T>>& output_trsm, // to TRSM
               ttg::Edge<Key3, BlockMatrix<T>>& output_result) // write back
{
  auto f = [=](const Key3& key,
               BlockMatrix<T>& tile_ik,
               BlockMatrix<T>& tile_kj,
               BlockMatrix<T>& tile_ij,
               std::tuple<ttg::Out<Key1, BlockMatrix<T>>,
                          ttg::Out<Key2, BlockMatrix<T>>,
                          ttg::Out<Key3, BlockMatrix<T>>>& out){
    
    const int I = key.I;
    const int J = key.J;
    const int K = key.K;

    //do the operation

    if(I == K+1 && J == K+1) //send tile to the next POTRF
      ttg::send<0>(Key1(K+1), std::move(tile_ij), out);
    else if(J == K+1 && I > K+1) //send tile to the next TRSMs
      ttg::send<1>(Key2(I, J), std::move(tile_ij), out);
    else
      ttg::send<2>(Key3(I, J, K), std::move(tile_kj), out); //write back
    
  };

  return ttg::wrap<Key3>(f, ttg::edges(input_ik, input_kj, input_ij), 
          ttg::edges(output_potrf, output_trsm, output_result), "GEMM",
          {"input_ik", "input_kj", "input_ij"}, {"output_potrf", "output_trsm", "output_result"});
}

template <typename T>
auto make_result(DistMatrix<T>& A, const ttg::Edge<Key3, BlockMatrix<T>>& result) 
{
  auto f = [=](const Key3& key, BlockMatrix<T>&& tile, std::tuple<>& out) {
    const int I = key.I;
    const int J = key.J;
    //if (A(I, J).data() != tile.data()) {
    //  std::copy_n(tile.data(), tile.rows()*tile.cols(), A(I, J).data());
    //}
  };

  return ttg::wrap(f, ttg::edges(result), ttg::edges(), "Final Output", {"result"}, {});

}

template<typename T>
auto initiator(DistMatrix<T>& A,
               ttg::Edge<Key1, BlockMatrix<T>>& gemm_potrf, // to POTRF
               ttg::Edge<Key2, BlockMatrix<T>>& gemm_trsm)  // to TRSM
{
  auto f = [=](const Key3& key,
               std::tuple<ttg::Out<Key1, BlockMatrix<T>>,
                          ttg::Out<Key2, BlockMatrix<T>>>& out){
    /* kick off first POTRF */
    if (A.is_local(0, 0)) {
      ttg::send<0>(Key1(0), std::move(A(0, 0)), out);
    }
    for (int i = 1; i < A.rows(); i++) 
    {
      /* send gemm input to TRSM */
      if (A.is_local(i, 0)) 
        ttg::send<1>(Key2(i, 0), std::move(A(i, 0)), out);
    }
  };
  return ttg::wrap<Key3>(f, ttg::edges(), ttg::edges(gemm_potrf, gemm_trsm), "INITIATOR");
}



int main(int argc, char **argv)
{
  int N = 1024;
  int M = N;
  int NB = 128;
  int check = 0;
  int nthreads = -1;
  const char* prof_filename = nullptr;


  ttg::ttg_initialize(argc, argv, nthreads);
  auto world = ttg::ttg_default_execution_context();

  int P = std::sqrt(world.size());
  int Q = (world.size() + P - 1)/P;
  sym_two_dim_block_cyclic_t dcA;
  sym_two_dim_block_cyclic_init(&dcA, matrix_type::matrix_RealDouble,
                                world.size(), world.rank(), NB, NB, N, M,
                                0, 0, N, M, P, matrix_Lower);
  dcA.mat = parsec_data_allocate((size_t)dcA.super.nb_local_tiles *
                                 (size_t)dcA.super.bsiz *
                                 (size_t)parsec_datadist_getsizeoftype(dcA.super.mtype));
  parsec_data_collection_set_key((parsec_data_collection_t*)&dcA, "Matrix A");
  DistMatrix<double> DistMat(&dcA);

  
  ttg::Edge<Key3, BlockMatrix<double>> forwarder_ik_gemm("forwarder_ik_gemm");
  ttg::Edge<Key3, BlockMatrix<double>> forwarder_kj_gemm("forwarder_kj_gemm");
  ttg::Edge<Key3, BlockMatrix<double>> forwarder_ij_gemm("forwarder_ij_gemm");
  ttg::Edge<Key1, BlockMatrix<double>> gemm_potrf("gemm_potrf"); 
  ttg::Edge<Key2, BlockMatrix<double>> gemm_trsm("gemm_trsm"); 
  ttg::Edge<Key2, BlockMatrix<double>> potrf_trsm("potrf_trsm"); 
  ttg::Edge<Key4 , Key3> trsm_forwarder_ik("forwarder_ik"); 
  ttg::Edge<Key4 , Key3> trsm_forwarder_kj("forwarder_kj"); 
  ttg::Edge<Key4 , Key3> trsm_forwarder_ij("forwarder_ij"); 
  ttg::Edge<Key3 , BlockMatrix<double>> result("result"); 



  auto keymap1 = [&](const Key1& key) {
    return DistMat.rank_of(key.K, key.K);
  };

  auto keymap2 = [&](const Key2& key) {
    return DistMat.rank_of(key.I, key.J);
  };

  auto keymap3 = [&](const Key3& key) {
    return DistMat.rank_of(key.I, key.J);
  };

  auto keymap4 = [&](const Key4& key) {
    return DistMat.rank_of(key.I, key.J);
  };

  auto op_gemm  = make_gemm(DistMat, forwarder_ik_gemm, forwarder_kj_gemm, forwarder_ij_gemm,
                                 gemm_potrf, gemm_trsm, result);
  op_gemm->set_keymap(keymap3);

  auto op_trsm  = make_trsm(DistMat, potrf_trsm, gemm_trsm,
                            trsm_forwarder_ik, trsm_forwarder_kj, trsm_forwarder_ij, result);
  op_trsm->set_keymap(keymap2);

  auto op_potrf = make_potrf(DistMat, gemm_potrf, potrf_trsm, result);
  op_potrf->set_keymap(keymap1);

  auto op_forwarder_ik = make_forwarder_ik(DistMat, trsm_forwarder_ik, forwarder_ik_gemm);
  op_forwarder_ik->set_keymap(keymap4);

  auto op_forwarder_kj = make_forwarder_kj(DistMat, trsm_forwarder_kj, forwarder_kj_gemm);
  op_forwarder_kj->set_keymap(keymap4);

  auto op_forwarder_ij = make_forwarder_ij(DistMat, trsm_forwarder_ij, forwarder_ij_gemm);
  op_forwarder_ij->set_keymap(keymap4);

  auto op_init  = initiator(DistMat, gemm_potrf, gemm_trsm);
  op_init->set_keymap([&](const Key3&){ return world.rank(); });

  auto op_result = make_result(DistMat, result);
  op_result->set_keymap(keymap3);
  

  auto connected = make_graph_executable(op_init.get());
  assert(connected);
  TTGUNUSED(connected);
  std::cout << "Graph is connected: " << connected << std::endl;

  op_init->invoke(Key3{0, 0, 0});

  ttg::ttg_execute(world);
  ttg::ttg_fence(world);

  //delete A;
  /* cleanup allocated matrix before shutting down PaRSEC */
  parsec_data_free(dcA.mat); dcA.mat = NULL;
  parsec_tiled_matrix_dc_destroy( (parsec_tiled_matrix_dc_t*)&dcA);


  ttg::ttg_finalize();
  return 0;
}
