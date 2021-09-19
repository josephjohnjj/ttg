#include <ttg/serialization.h>

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
  T* data() const { return m_block.get();}

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

  void print_block() {
    printf("Block: \n");
    for (int i = 0; i < _rows; ++i) {
      printf("\n");
      for (int j = 0; j < _cols; ++j) {
        printf("%lf ", m_block.get()[i * _cols + j]);
      }
    }
    printf("\n");
  }

  int is_empty()
  {
    //sample 5 points to check for sparcity
    if((*this)(0, 0) == -99 && (*this)(0, _cols-1) == -99 && (*this)(_rows-1, 0) == -99 && 
      (*this)(_rows-1, _cols-1) == -99 && (*this)(_rows/2, _cols/2) == -99)
      return 1;
    else
      return 0;
  }

  void set_empty()
  {
    this->fill(-99);
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

