#include <ttg/serialization.h>
#include <iostream>
#include <unordered_map>
#include <parsec.h>
#include <parsec/data_internal.h>
#include <parsec/data_dist/matrix/matrix.h>
#include <parsec/data_dist/matrix/sym_two_dim_rectangle_cyclic.h>
#include <parsec/data_dist/matrix/two_dim_rectangle_cyclic.h>

template <typename T>
class BlockMatrix {
 private:
  int _rows = 0;
  int _cols = 0;
  std::shared_ptr<T> m_block;  // should become std::shared_ptr<T[]> but could not make older Apple clang to accept it

 public:
  BlockMatrix() = default;

  BlockMatrix(int rows, int cols) : _rows(rows), _cols(cols), m_block(new T[_rows * _cols], [](T* p) { delete[] p; }) {}

  BlockMatrix(int rows, int cols, T* block) : _rows(rows), _cols(cols), m_block(block) {}

  ~BlockMatrix() {}

  int size() const { return _rows * _cols; }
  int rows() const { return _rows; }
  int cols() const { return _cols; }
  const T* get() const { return m_block.get(); }
  T* get() { return m_block.get(); }

  void fill() {
    // Initialize all elements of the matrix to 1
    for (int i = 0; i < _rows; ++i) {
      for (int j = 0; j < _cols; ++j) {
        m_block.get()[i * _cols + j] = 1;
      }
    }
  }

  void fill(T val) {
    // Initialize all elements of the matrix to val
    for (int i = 0; i < _rows; ++i) {
      for (int j = 0; j < _cols; ++j) {
        m_block.get()[i * _cols + j] = val;
      }
    }
  }

  int is_empty()
  {
    //sample 5 points to check for sparcity
    if((*this)(0, 0) == -99999 && (*this)(0, _cols-1) == -99999 && (*this)(_rows-1, 0) == -99999 && 
      (*this)(_rows-1, _cols-1) == -99999 && (*this)(_rows/2, _cols/2))
      return 1;
    else
      return 0;
  }

  void set_empty()
  {
    this->fill(-99999);
  }

  // return a diagonal bias (minimally) sufficient to make a matrix with
  // random elements in (-1,+1) positive definite
  static double diag_bias(int N) 
  {
    return (1.0 + N / 4.0); // the scaling factor 4.0 is empirically determined
  }

  void init_lower_pos_def_tile(int N, int i0, int j0, int seed) 
  {
    int i, j;
    srand(i0 + j0 + 29*seed);     

    for (i=0; i < _rows; i++) 
      for (j=0; j < _cols; j++) 
      {
        if (i < j) //upper triangular element 
	        (*this)(i, j) = 0.0;
       else 
       {
	        (*this)(i, j) = (2.0 * rand() / RAND_MAX) - 1.0;
	        assert (-1.0 <= m_block.get()[i * _cols + j] <= 1.0);
	        if (i == j)
	          (*this)(i, j) = fabs((*this)(i, j)) + diag_bias(N);
        }
      } 
  } 

  bool operator==(const BlockMatrix& m) const {
    bool equal = true;
    for (int i = 0; i < _rows; i++) {
      for (int j = 0; j < _cols; j++) {
        if (m_block.get()[i * _cols + j] != m.m_block.get()[i * _cols + j]) {
          equal = false;
          break;
        }
      }
    }
    return equal;
  }

  bool operator!=(const BlockMatrix& m) const {
    bool notequal = false;
    for (int i = 0; i < _rows; i++) {
      for (int j = 0; j < _cols; j++) {
        if (m_block.get()[i * _cols + j] != m.m_block.get()[i * _cols + j]) {
          notequal = true;
          break;
        }
      }
    }

    return notequal;
  }

  // Return by value
  inline T& operator()(int row, int col) { return m_block.get()[row * _cols + col]; }
  inline const T& operator()(int row, int col) const { return m_block.get()[row * _cols + col]; }

  void operator()(int row, int col, T val) { m_block.get()[row * _cols + col] = val; }

#ifdef TTG_SERIALIZATION_SUPPORTS_BOOST
  template <typename Archive>
  void save(Archive& ar, const unsigned int version) const {
    ar << rows() << cols();
    ar << boost::serialization::make_array(get(), rows() * cols());
  }

  template <typename Archive>
  void load(Archive& ar, const unsigned int version) {
    int rows, cols;
    ar >> rows >> cols;
    *this = BlockMatrix<T>(rows, cols);
    ar >> boost::serialization::make_array(get(), this->rows() * this->cols());  // BlockMatrix<T>(bm.rows(),
                                                                                 // bm.cols());
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER();
#endif  // TTG_SERIALIZATION_SUPPORTS_BOOST
};

#ifdef TTG_SERIALIZATION_SUPPORTS_MADNESS
namespace madness {
  namespace archive {
    template <class Archive, typename T>
    struct ArchiveStoreImpl<Archive, BlockMatrix<T>> {
      static inline void store(const Archive& ar, const BlockMatrix<T>& bm) {
        ar << bm.rows() << bm.cols();
        ar << wrap(bm.get(), bm.rows() * bm.cols());  // BlockMatrix<T>(bm.rows(), bm.cols());
      }
    };

    template <class Archive, typename T>
    struct ArchiveLoadImpl<Archive, BlockMatrix<T>> {
      static inline void load(const Archive& ar, BlockMatrix<T>& bm) {
        int rows, cols;
        ar >> rows >> cols;
        bm = BlockMatrix<T>(rows, cols);
        ar >> wrap(bm.get(), bm.rows() * bm.cols());  // BlockMatrix<T>(bm.rows(), bm.cols());
      }
    };
  }  // namespace archive
}  // namespace madness

static_assert(madness::is_serializable_v<madness::archive::BufferOutputArchive, BlockMatrix<double>>);

#endif  // TTG_SERIALIZATION_SUPPORTS_MADNESS

template <typename T>
std::ostream& operator<<(std::ostream& s, const BlockMatrix<T>& m) {
  for (int i = 0; i < m.rows(); i++) {
    for (int j = 0; j < m.cols(); j++) s << m(i, j) << " ";
    s << std::endl;
  }
  return s;
}

// https://stackoverflow.com/questions/32685540/why-cant-i-compile-an-unordered-map-with-a-pair-as-key
// We need this since pair cannot be hashed by unordered_map.
struct pair_hash {
  template <class T1, class T2>
  std::size_t operator()(const std::pair<T1, T2>& p) const {
    auto h1 = std::hash<T1>{}(p.first);
    auto h2 = std::hash<T2>{}(p.second);

    // Mainly for demonstration purposes, i.e. works but is overly simple
    // In the real world, use sth. like boost.hash_combine
    return h1 ^ h2;
  }
};


template <typename T>
class DistMatrix 
{
  private:
    BlockMatrix<T> **dcA;
    int block_rows, block_cols; // number of rows and columns in the global block matrix
    int tile_rows, tile_cols; //number of rows and columns in each tile (block)
    int rank; //rank of the proces
    int processes; //total number
   
  public:

    DistMatrix(BlockMatrix<T>* A, int n, int m, int t_n, int t_m) : dcA(A), block_rows(m), block_cols(n), tile_rows(t_m), tile_cols(t_n){}

    DistMatrix(int p, int r, int m, int n, int t_m, int t_n) 
    {
      block_rows = m;
      block_cols = n;
      tile_rows = t_m;
      tile_cols = t_n;
      rank = r;
      processes = p;

      int l = (block_rows * block_cols) / processes;
      dcA = new BlockMatrix<T>*[l];
      for(int i = 0; i < l; i++)
        dcA[i] = nullptr;
    }

    DistMatrix(int m, int n, int t_m, int t_n) 
    {
      block_rows = m;
      block_cols = n;
      tile_rows = t_m;
      tile_cols = t_n;
      rank = ttg::ttg_default_execution_context().rank() ;
      processes = ttg::ttg_default_execution_context().size() ;

      int l = (block_rows * block_cols) / processes;
      dcA = new BlockMatrix<T>*[l];
      for(int i = 0; i < l; i++)
        dcA[i] = nullptr;
    }

    void init_matrix(int perc_sparcity)
    {
      for(int m = 0; m < block_rows; m++ )
        for(int n = 0; n < block_cols; n++ )
        {
          if(is_local(m, n))
          {
            if(m == n) // diogonal tiles are always dense
            {
              int tile_num = m * block_rows + n;
              int local_mem_index = tile_num / processes;
              dcA[local_mem_index] = new BlockMatrix<T>(tile_rows, tile_cols);
              dcA[local_mem_index]->init_lower_pos_def_tile(block_rows * block_rows * tile_rows * tile_cols, m, n, 0);
            }
            else if( m < n) // upper triangular tiles. Right now we dont care about this
            {
              // do nothing
            }
            else // lower triangular tiles. perc_sparcity specifies the perecentage of sparcity
            {    // in these tile

              int tile_num = m * block_rows + n;

              if(tile_num % 2 == 0)
              {
                int local_mem_index = tile_num / processes;
                dcA[local_mem_index] = new BlockMatrix<T>(tile_rows, tile_cols);
                dcA[local_mem_index]->init_lower_pos_def_tile(block_rows * block_rows * tile_rows * tile_cols, m, n, 0);
              }
            }
            
          }
        }
    }

    void fill(int m, int n, T val)
    {
      assert(is_empty(m, n));
      assert(is_local(m, n));

      int tile_num = m * block_rows + n;
      int local_mem_index = tile_num / processes;
      dcA[local_mem_index] = new BlockMatrix<T>(tile_rows, tile_cols);
      dcA[local_mem_index]->fill(val);
    }

    BlockMatrix<T> operator()(int m, int n) const 
    {
      int tile_num = m * block_rows + n;
      assert(is_local(m, n));  
      BlockMatrix<T> *ptr = dcA[ tile_num / processes ];
      return *ptr; 
  }

  /* The rank storing the tile at {m, n} */
  int rank_of(int m, int n) const 
  {
    int tile_num = m * block_rows + n;
    return tile_num % processes;
  }

  bool is_local(int m, int n) const 
  {
    return rank == rank_of(m, n);
  }

  
  void set_empty(int m, int n)
  {
    int tile_num = m * block_rows + n;
    assert(is_local(m, n));  
    dcA[ tile_num / processes ] = nullptr;
  }

  bool is_empty(int m, int n) const 
  {
    int tile_num = m * block_rows + n;
    assert(is_local(m, n));  
    T* ptr = dcA(tile_num / processes );
                      
    if(ptr == NULL || ptr == nullptr)
      return 1;
    else  
      return 0;
  }

  int rows(void) const {
    return block_rows;
  }

  int cols(void) const {
    return block_cols;
  }
};
