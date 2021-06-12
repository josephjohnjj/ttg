This document describes how a TTG program works. This may be helpful for someone who has predominantly worked with PTG and C. Experienced C++ programmers may find most explanations rudimentary. 

(---> denote functiona call chains)


Example code (https://github.com/devreal/ttg/tree/ttg_cholesky/examples):

```
#define TTG_USE_PARSEC 1

#include <ttg.h>
//#include <madness.h>
#include "../blockmatrix.h"

#include <lapack.hh>

#include "core_dplgsy.h"

// needed for madness::hashT and xterm_debug
#include <madness/world/world.h>

struct Key {
  // ((I, J), K) where (I, J) is the tile coordiante and K is the iteration number
  int I = 0, J = 0, K = 0;
  madness::hashT hash_val;

  Key() { rehash(); }
  Key(int I, int J, int K) : I(I), J(J), K(K) { rehash(); }

  madness::hashT hash() const { return hash_val; }
  void rehash() {
    hash_val = (static_cast<madness::hashT>(I) << 48)
             ^ (static_cast<madness::hashT>(J) << 32)
             ^ (K << 16);
  }

  // Equality test
  bool operator==(const Key& b) const { return I == b.I && J == b.J && K == b.K; }

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
  s << "Key(" << key.I << "," << key.J << "," << key.K << ")";
  return s;
}

void plgsy(Matrix<double>* A)
{
  auto bump = A->rows();
  auto seed = 42;
  for (int i = 0; i < A->rows(); ++i) {
    for (int j = 0; j < A->cols(); ++j) {
      auto tile = (*A)(i, j);
      CORE_dplgsy(bump, tile.rows(), tile.cols(), tile.get(), tile.rows(),
                  A->rows(), i*tile.rows(), j*tile.cols(), seed);
    }
  }
}

template <typename T>
auto make_potrf(Matrix<T>* A,
                ttg::Edge<Key, BlockMatrix<T>>& input,
                ttg::Edge<Key, BlockMatrix<T>>& output_trsm,
                ttg::Edge<Key, BlockMatrix<T>>& output_result)
{
  auto f = [=](const Key& key,
               BlockMatrix<T>&& tile_kk,
               std::tuple<ttg::Out<Key, BlockMatrix<T>>,
                          ttg::Out<Key, BlockMatrix<T>>>& out){
    const int I = key.I;
    const int J = key.J;
    const int K = key.K;
    assert(I == J);
    assert(I == K);

    lapack::potrf(lapack::Uplo::Lower, tile_kk.rows(), tile_kk.get(), tile_kk.rows());

    //std::cout << "POTRF(" << key << ")" << std::endl;

    /* tile is done */
    ttg::send<0>(key, tile_kk, out);

    /* send the tile to outputs */
    for (int m = I+1; m < A->rows(); ++m) {
      /* send tile to trsm */
      ttg::send<1>(Key(m, J, K), tile_kk, out);
    }
  };
  return ttg::wrap(f, ttg::edges(input), ttg::edges(output_result, output_trsm), "POTRF", {"tile_kk"}, {"output_result", "output_trsm"});
}

template <typename T>
auto make_trsm(Matrix<T>* A,
               ttg::Edge<Key, BlockMatrix<T>>& input_kk,
               ttg::Edge<Key, BlockMatrix<T>>& input_mk,
               ttg::Edge<Key, BlockMatrix<T>>& output_diag,
               ttg::Edge<Key, BlockMatrix<T>>& output_row,
               ttg::Edge<Key, BlockMatrix<T>>& output_col,
               ttg::Edge<Key, BlockMatrix<T>>& output_result)
{
  auto f = [=](const Key& key,
               const BlockMatrix<T>&  tile_kk,
                     BlockMatrix<T>&& tile_mk,
                     std::tuple<ttg::Out<Key, BlockMatrix<T>>,
                                ttg::Out<Key, BlockMatrix<T>>,
                                ttg::Out<Key, BlockMatrix<T>>,
                                ttg::Out<Key, BlockMatrix<T>>>& out){
    const int I = key.I;
    const int J = key.J;
    const int K = key.K;
    assert(I > K); // we're below (k, k) in row i, column j [k+1 .. NB, k]

    /* No support for different tile sizes yet */
    assert(tile_mk.rows() == tile_kk.rows());
    assert(tile_mk.cols() == tile_kk.cols());

    auto m = tile_mk.rows();

    blas::trsm(blas::Layout::RowMajor,
               blas::Side::Right,
               lapack::Uplo::Lower,
               blas::Op::Trans,
               blas::Diag::NonUnit,
               tile_kk.rows(), m, 1.0,
               tile_kk.get(), m,
               tile_mk.get(), m);

    //std::cout << "TRSM(" << key << ")" << std::endl;

    /* tile is done */
    ttg::send<0>(key, tile_kk, out);

    //if (I+1 < A->rows()) {
      /* send tile to syrk on diagonal */
      //std::cout << "TRSM(" << key << "): sending output to diag " << Key{I, I, K} << std::endl;
      ttg::send<1>(Key(I, I, K), tile_mk, out);

      /* send the tile to all gemms across in row i */
      for (int n = J+1; n < I; ++n) {
        //std::cout << "TRSM(" << key << "): sending output to row " << Key{I, n, K} << std::endl;
        ttg::send<2>(Key(I, n, K), tile_mk, out);
      }

      /* send the tile to all gemms down in column i */
      for (int m = I+1; m < A->rows(); ++m) {
        //std::cout << "TRSM(" << key << "): sending output to col " << Key{m, I, K} << std::endl;
        ttg::send<3>(Key(m, I, K), tile_mk, out);
      }
    //}
  };
  return ttg::wrap(f, ttg::edges(input_kk, input_mk), ttg::edges(output_result, output_diag, output_row, output_col),
                   "TRSM", {"tile_kk", "tile_mk"}, {"output_result", "output_diag", "output_row", "output_col"});
}


template <typename T>
auto make_syrk(Matrix<T>* A,
               ttg::Edge<Key, BlockMatrix<T>>& input_mk,
               ttg::Edge<Key, BlockMatrix<T>>& input_mm,
               ttg::Edge<Key, BlockMatrix<T>>& output_potrf,
               ttg::Edge<Key, BlockMatrix<T>>& output_syrk)
{
  auto f = [=](const Key& key,
               const BlockMatrix<T>&  tile_mk,
                     BlockMatrix<T>&& tile_mm,
                     std::tuple<ttg::Out<Key, BlockMatrix<T>>,
                                ttg::Out<Key, BlockMatrix<T>>>& out){
    const int I = key.I;
    const int J = key.J;
    const int K = key.K;
    assert(I == J);
    assert(I > K);

    /* No support for different tile sizes yet */
    assert(tile_mk.rows() == tile_mm.rows());
    assert(tile_mk.cols() == tile_mm.cols());

    auto m = tile_mk.rows();

    blas::syrk(blas::Layout::RowMajor,
               lapack::Uplo::Lower,
               blas::Op::NoTrans,
               tile_mk.rows(), m, -1.0,
               tile_mk.get(), m, 1.0,
               tile_mm.get(), m);

    //std::cout << "SYRK(" << key << ")" << std::endl;

    if (I == K+1) {
      /* send the tile to potrf */
      //std::cout << "SYRK(" << key << "): sending output to POTRF " << Key{I, I, K+1} << std::endl;
      ttg::send<0>(Key(I, I, K+1), tile_mm, out);
    } else {
      /* send output to next syrk */
      //std::cout << "SYRK(" << key << "): sending output to SYRK " << Key{I, I, K+1} << std::endl;
      ttg::send<1>(Key(I, I, K+1), tile_mm, out);
    }

  };
  return ttg::wrap(f,
                   ttg::edges(input_mk, input_mm),
                   ttg::edges(output_potrf, output_syrk), "SYRK",
                   {"tile_mk", "tile_mm"}, {"output_potrf", "output_syrk"});
}


template <typename T>
auto make_gemm(Matrix<T>* A,
               ttg::Edge<Key, BlockMatrix<T>>& input_nk,
               ttg::Edge<Key, BlockMatrix<T>>& input_mk,
               ttg::Edge<Key, BlockMatrix<T>>& input_nm,
               ttg::Edge<Key, BlockMatrix<T>>& output_trsm,
               ttg::Edge<Key, BlockMatrix<T>>& output_gemm)
{
  auto f = [](const Key& key,
              const BlockMatrix<T>& tile_nk,
              const BlockMatrix<T>& tile_mk,
                    BlockMatrix<T>&& tile_nm,
                    std::tuple<ttg::Out<Key, BlockMatrix<T>>,
                               ttg::Out<Key, BlockMatrix<T>>>& out){
    const int I = key.I;
    const int J = key.J;
    const int K = key.K;
    assert(I != J && I > K && J > K);

    /* No support for different tile sizes yet */
    assert(tile_nk.rows() == tile_mk.rows() && tile_nk.rows() == tile_nm.rows());
    assert(tile_nk.cols() == tile_mk.cols() && tile_nk.cols() == tile_nm.cols());

    auto m = tile_nk.rows();

    blas::gemm(blas::Layout::RowMajor,
               blas::Op::NoTrans,
               blas::Op::Trans,
               m, m, m, -1.0,
               tile_nk.get(), m,
               tile_mk.get(), m, 1.0,
               tile_nm.get(), m);

    //std::cout << "GEMM(" << key << ")" << std::endl;

    /* send the tile to output */
    if (J == K+1) {
      /* send the tile to trsm */
      //std::cout << "GEMM(" << key << "): sending output to TRSM " << Key{I, J, K+1} << std::endl;
      ttg::send<0>(Key(I, J, K+1), tile_nm, out);
    } else {
      /* send the tile to the next gemm */
      //std::cout << "GEMM(" << key << "): sending output to GEMM " << Key{I, J, K+1} << std::endl;
      ttg::send<1>(Key(I, J, K+1), tile_nm, out);
    }
  };
  return ttg::wrap(f,
                   ttg::edges(input_nk, input_mk, input_nm),
                   ttg::edges(output_trsm, output_gemm), "GEMM",
                   {"input_nk", "input_mk", "input_nm"},
                   {"output_trsm", "outout_gemm"});
}

template<typename T>
auto initiator(Matrix<T>* A,
               ttg::Edge<Key, BlockMatrix<T>>& syrk_potrf,
               ttg::Edge<Key, BlockMatrix<T>>& gemm_trsm,
               ttg::Edge<Key, BlockMatrix<T>>& syrk_syrk,
               ttg::Edge<Key, BlockMatrix<T>>& gemm_gemm)
{
  auto f = [=](const Key&,
               std::tuple<ttg::Out<Key, BlockMatrix<T>>,
                          ttg::Out<Key, BlockMatrix<T>>,
                          ttg::Out<Key, BlockMatrix<T>>,
                          ttg::Out<Key, BlockMatrix<T>>>& out){
    /* kick off first POTRF */
    ttg::send<0>(Key{0, 0, 0}, (*A)(0, 0), out);
    for (int i = 1; i < A->rows(); i++) {
      /* send gemm input to TRSM */
      ttg::send<1>(Key{i, 0, 0}, (*A)(i, 0), out);
      /* send syrk to SYRK */
      ttg::send<2>(Key{i, i, 0}, (*A)(i, i), out);
      for (int j = 1; j < i; j++) {
        /* send gemm to GEMM */
        ttg::send<3>(Key{i, j, 0}, (*A)(i, j), out);
      }
    }
  };

  return ttg::wrap<Key>(f, ttg::edges(), ttg::edges(syrk_potrf, gemm_trsm, syrk_syrk, gemm_gemm), "INITIATOR");
}

template <typename T>
auto make_result(Matrix<T> *A, const ttg::Edge<Key, BlockMatrix<T>>& result) {
  auto f = [](const Key& key, BlockMatrix<T>&& tile, std::tuple<>& out) {
    /* TODO: is this node actually needed? */
    //std::cout << "FINAL " << key << std::endl;
  };

  return ttg::wrap(f, ttg::edges(result), ttg::edges(), "Final Output", {"result"}, {});
}


int main(int argc, char **argv)
{

  std::chrono::time_point<std::chrono::high_resolution_clock> beg, end;
  int N = 1024;
  int M = N;
  int NB = 128;
  ttg::ttg_initialize(argc, argv, 2);

  if (argc > 1) {
    N = M = atoi(argv[1]);
  }

  if (argc > 2) {
    NB = atoi(argv[2]);
  }

  int n_rows = (N / NB) + (N % NB > 0);
  int n_cols = (M / NB) + (M % NB > 0);

  Matrix<double>* A = new Matrix<double>(n_rows, n_cols, NB, NB);

  ttg::Edge<Key, BlockMatrix<double>> potrf_trsm("potrf_trsm"),
                                      trsm_syrk("trsm_syrk"),
                                      syrk_potrf("syrk_potrf"),
                                      syrk_syrk("syrk_syrk"),
                                      gemm_gemm("gemm_gemm"),
                                      gemm_trsm("gemm_trsm"),
                                      trsm_gemm_row("trsm_gemm_row"),
                                      trsm_gemm_col("trsm_gemm_col"),
                                      result("result");

  /* initialize the matrix */
  plgsy(A);

  auto op_init  = initiator(A, syrk_potrf, gemm_trsm, syrk_syrk, gemm_gemm);
  auto op_potrf = make_potrf(A, syrk_potrf, potrf_trsm, result);
  auto op_trsm  = make_trsm(A,
                            potrf_trsm, gemm_trsm,
                            trsm_syrk, trsm_gemm_row, trsm_gemm_col, result);
  auto op_syrk  = make_syrk(A, trsm_syrk, syrk_syrk, syrk_potrf, syrk_syrk);
  auto op_gemm  = make_gemm(A,
                            trsm_gemm_row, trsm_gemm_col, gemm_gemm,
                            gemm_trsm, gemm_gemm);
  auto op_result = make_result(A, result);

  auto connected = make_graph_executable(op_init.get());
  assert(connected);
  TTGUNUSED(connected);
  std::cout << "Graph is connected: " << connected << std::endl;

  auto world = ttg::ttg_default_execution_context();

  if (world.rank() == 0) {
    std::cout << "==== begin dot ====\n";
    std::cout << ttg::Dot()(op_init.get()) << std::endl;
    std::cout << "==== end dot ====\n";

    beg = std::chrono::high_resolution_clock::now();
    op_init->invoke(Key{0, 0, 0});
  }

  ttg::ttg_execute(world);
  ttg::ttg_fence(world);
  if (world.rank() == 0) {
    end = std::chrono::high_resolution_clock::now();
    std::cout << "TTG Execution Time (milliseconds) : "
              << (std::chrono::duration_cast<std::chrono::microseconds>(end - beg).count()) / 1000 << std::endl;
  }

  delete A;
  ttg::ttg_finalize();
  return 0;
}


```


### DAG Definition
The DAG consists of a set of nodes and the edges between them. In TTG we define, the types of edges and the types of tasks (task class) used. 

#### Edge definition

Edge in TTG an edge carries both key of the task it is intended for and the input data for the same task. So when defining an edge we pass the type of the key and the type of data as template arguments [1] and the name of the edge as the constructor argument [2]. 

```
ttg::Edge<Key, BlockMatrix<double>> potrf_trsm("potrf_trsm")
```

#### Task definition

To define a task we create a function (eg: make_gemm()) that takes all the types of edges this task type will use, in addition to all the elements we need to define a task body (eg: data it operates on). The function returns an object of class Op (ttg/ttg/ttg/parsec/ttg.h). The instance of s class Op represents a task class in TTG.

The function mainly does 3 things
 1. it wraps ( wrap() ttg/ttg/ttg/wrap.h) everything together to return the class Op instance 
 2. it defines the computation inside a task body (hook) as a lambda function 
 3. inside the lambda function it defines the out edge from a task to its successor task 

For example take the the function make_gemm(). 

```
return ttg::wrap(f,
                   ttg::edges(input_nk, input_mk, input_nm),
                   ttg::edges(output_trsm, output_gemm), "GEMM",
                   {"input_nk", "input_mk", "input_nm"},
                   {"output_trsm", "outout_gemm"});
```

This creates a task class that executes function lamda fucntion 'f, and takes an input edge of type input_nk (trsm_gemm_row),input_mk (trsm_gemm_col) and input_nm (gemm_gemm). It has an output edge of type output_trsm (gemm_trsm) and output_gemm (gemm_gemm). "GEMM" is the name of the task class. {"input_nk", "input_mk", "input_nm"} are the name given to the input edges and {"output_trsm", "outout_gemm"} are the names given to the output edges. 

The lambda function 'f' carries out the required computation (in the case of "GEMM" a blas::gemm() operation) and sends the output to its successors. The order of declaring the edges in ttg::wrap() is very important as it plays an important role in sending the data to the successors. For instance in the function f in make_gemm(), we have

```
ttg::send<0>(Key{0, 0, 0}, (*A)(0, 0), out);
```

This means that the task will send the key Key{0, 0, 0} and data (*A)(0, 0) through the 0th output edge output_trsm (gemm_trsm). If you check the function make_trsm() you can see that gemm_trsm is an input edge to the task "TRSM". 

Similarly, the same order should be maintained when declaring the input to the lambda function. 
```
auto f = [](const Key& key,
              const BlockMatrix<T>& tile_nk,
              const BlockMatrix<T>& tile_mk,
                    BlockMatrix<T>&& tile_nm,
                    std::tuple<ttg::Out<Key, BlockMatrix<T>>,
                               ttg::Out<Key, BlockMatrix<T>>>& out){.... }
```

The order is 
1. key
2. followed by the input data types (same order as the input edges)
3. followed by the tuple of output data types (same order as the output edges).  

### What happens when a task sends key + data

#### Sender Side


```
ttg::send<0>(Key{0, 0, 0}, (*A)(0, 0), out);
```

invokes the functions:

```
template <size_t i, typename keyT, typename valueT, typename... output_terminalsT>
  std::enable_if_t<meta::is_none_void_v<keyT,std::decay_t<valueT>>,void>
      send(const keyT &key, valueT &&value, std::tuple<output_terminalsT...> &t) {

    std::get<i>(t).send(key, std::forward<valueT>(value));
  }
```

The above function gets the ith terminal from the tuple of output terminals. It then invokes the member function

```
template<typename Key = keyT, typename Value = valueT>
std::enable_if_t<meta::is_none_void_v<Key,Value>,void> send(const Key &key, const Value &value) {
  for (auto && successor : successors()) {
    assert(successor->get_type() != TerminalBase::Type::Write);
    if (successor->get_type() == TerminalBase::Type::Read) {
      static_cast<In<keyT, std::add_const_t<valueT>> *>(successor)->send(key, value);
    } else if (successor->get_type() == TerminalBase::Type::Consume) {
      static_cast<In<keyT, valueT> *>(successor)->send(key, value);
    }
  }
}
```

of the class Out (ttg/ttg/ttg/terminal.h). This in turn calls the member function 

```
template <typename Key = keyT, typename Value = valueT>
std::enable_if_t<meta::is_none_void_v<Key,Value>,void>
send(const Key &key, const Value &value) {
  std::cout << "DEBUG: send b4 send_callback 66" << std::endl;
  if (!send_callback) throw std::runtime_error("send callback not initialized");
  send_callback(key, value);
}
```
of class In (ttg/ttg/ttg/terminal.h). Where send_callback is a lamda function (ttg/ttg/parsec/ttg.h)

```
auto send_callback = [this](const keyT &key, const valueT &value) {
          set_arg<i, keyT, const valueT &>(key, value);
        };
```

which in turn calls set_args()

```
template <std::size_t i, typename Key, typename Value>
 std::enable_if_t<!ttg::meta::is_void_v<Key> && !std::is_void_v<std::decay_t<Value>>, void> set_arg(const Key &key,
                                                                                                    Value &&value) {
   set_arg_impl<i>(key, std::forward<Value>(value));
 }
```
set_arg_impl() packs both the key and the data  using pack() (ttg/ttg/ttg/parsec/ttg.h) ---> pack_payload() (ttg/ttg/ttg/serialization/data_descriptor.h) 

```
static uint64_t pack_payload(const void *object, uint64_t size, uint64_t begin, void *buf) {

    unsigned char *char_buf = reinterpret_cast<unsigned char *>(buf);
    std::memcpy(&char_buf[begin], object, size);
    return begin + size;
}
```

into an object of type msg_t (ttg/ttg/ttg/parsec/ttg.h). The unique instance id for the task class (explained in the receiver part), taskpool id and the position of the output terminal is also packed into this. 

```
struct msg_header_t {
  uint32_t taskpool_id;
  uint64_t op_id;
  std::size_t param_id;
};

struct msg_t {
  msg_header_t op_id;
  unsigned char bytes[WorldImpl::PARSEC_TTG_MAX_AM_SIZE - sizeof(msg_header_t)];
  msg_t() = default;
  msg_t(uint64_t op_id, uint32_t taskpool_id, std::size_t param_id) : op_id{taskpool_id, op_id,param_id} {}
};
```

The packed message is then send across network using the parsec sending function send_am(). 

```
parsec_ce.send_am(&parsec_ce, world_impl.parsec_ttg_tag(), owner, static_cast<void *>(msg),
                        sizeof(msg_header_t) + pos);
```

world_impl.parsec_ttg_tag() returns the tag _PARSEC_TTG_TAG.

#### Receiver Side

When a task class is created using ttg:wrap() it creates a unique instance id for the task class. This unique instance id is used as a key to create an entry in the std::map [4] object static_id_to_op_map, data corresponding to the key is an object of std::pair. The first element in the pair is a function static_set_arg() (ttg/ttg/ttg/parsec/ttg.h) and the second element is the instance returned by ttg:wrap() (The unique instance id belongs to class OpBase. static_set_arg() also operates on object of type class OpBase. But as class Op returned by ttg:wrap() inherits from OpBase, this is not a problem).


The _PARSEC_TTG_TAG is matched by a receiver function that invokes the function (static_unpack_msg)

```
parsec_ce.tag_register(_PARSEC_TTG_TAG, static_unpack_msg, this, PARSEC_TTG_MAX_AM_SIZE);

```

The static_unpack_msg() extracts the unique instance id (op_id) from the message (of type msg_t).
The op_id of the task class is then used to find the correct pair in the static_id_to_op_map object. 

```
auto op_pair = static_id_to_op_map.at(op_id);
static_map_mutex.unlock();
tp->tdm.module->incoming_message_start(tp, src_rank, NULL, NULL, 0, NULL);
static_set_arg_fct = op_pair.first;
static_set_arg_fct(data, size, op_pair.second);
tp->tdm.module->incoming_message_end(tp, NULL);
```

static_set_arg_fct in the above code refers to the function static_set_arg(). The function is called with arguments 
1. message - which is message received over the network, 
2. size - which is the size of the message and 
3. op_pair.second is the task class itself. 

static_set_arg() ---> set_arg_from_msg() (ttg/ttg/parsec/ttg.h). set_arg_from_msg() unpacks both key and the data from the message. set_arg_from_msg() ---> set_args() ---> set_arg_impl() ---> set_arg_local() ---> set_arg_local_impl().

set_arg_local_impl() checks if the task has already been created by checking the hash table. If the task is not available in the hash table
1. task is created
2. the data is associated with the task
3. the task is pushed to the hash table

if the task is already available hash tables
1. the data is associated with the task
2. if all its data dependency is met, remove it from hash table
3. schedule the task



### REFERENCES

[1]: Templates https://www.geeksforgeeks.org/templates-cpp/
[2]: Constructors https://www.learncpp.com/cpp-tutorial/constructors-and-initialization-of-derived-classes/
[3]: Reference https://isocpp.org/wiki/faq/references
[4]: std::map https://en.cppreference.com/w/cpp/container/map
[5]: std::pair https://en.cppreference.com/w/cpp/utility/pair


