#include <ttg/serialization.h>
#include <iostream>
#include <unordered_map>

template <typename T>
class BlockData {
 private:
  int _x = 0;
  int _y = 0;
  int _z = 0;
  std::shared_ptr<T> m_block;  // should become std::shared_ptr<T[]> but could not make older Apple clang to accept it

 public:
  BlockData() = default;

  //BlockData(int x, int y, int z) : _x(x), _y(y), _z(z), m_block(new T[_x * _y * _z], [](T* p) { delete[] p; }) {}

  BlockData(int x, int y, int z, T* block) : _x(x), _y(y), _z(z), m_block(block) {}


  ~BlockData() {}

  int size() const { return _x * _y * _z; }
  int x() const { return _x; }
  int y() const { return _y; }
  int z() const { return _z; }

  const T* get() const { return m_block.get(); }
  T* get() { return m_block.get(); }

  void fill() {
    // Initialize all elements of the matrix to 1
    for (int i = 0; i < _x; ++i) 
      for (int j = 0; j < _y; ++j) 
        for (int k = 0; k < _z; ++k)
            m_block.get()[ (i * _y * _z) + (j *_z) + k ] = 1;
  }
      
    
  bool operator==(const BlockData& m) const {
    bool equal = true;
    for (int i = 0; i < _x; ++i) 
      for (int j = 0; j < _y; ++j) 
        for (int k = 0; k < _z; ++k)
        {
            if (m_block.get()[(i * _y * _z) + (j *_z) + k] != m.m_block.get()[(i * _y * _z) + (j *_z) + k]) 
            {
              equal = false;
              break;
            }
        }
        
    return equal;
  }

  bool operator!=(const BlockData& m) const {
    bool notequal = false;
    for (int i = 0; i < _x; ++i) 
      for (int j = 0; j < _y; ++j) 
        for (int k = 0; k < _z; ++k)
        {
            if (m_block.get()[(i * _y * _z) + (j *_z) + k] != m.m_block.get()[(i * _y * _z) + (j *_z) + k]) 
            {
              notequal = true;
              break;
            }
       }
  
    return notequal;
  }

  // Return by value
  inline T& operator()(int x, int y, int z) { return m_block.get()[x * _y * _z + (y * _z) + z]; }
  inline const T& operator()(int x, int y, int z) const { return m_block.get()[x * _y * _z + (y * _z) + z];  }
  void operator()(int x, int y, int z, T val) { m_block.get()[x * _y * _z + (y * _z) + z] = val; }

#ifdef TTG_SERIALIZATION_SUPPORTS_BOOST
  template <typename Archive>
  void save(Archive& ar, const unsigned int version) const {
    ar << x() << y() << z();
    ar << boost::serialization::make_array(get(), x() * y() * z() );
  }

  template <typename Archive>
  void load(Archive& ar, const unsigned int version) {
    int x, y, z;
    ar >> x >> y >> z;
    *this = BlockData<T>(x, y, z);
    ar >> boost::serialization::make_array(get(), this->x() * this->y() * this->z() );  // BlockData<T>(bm.rows(),
                                                                                 // bm.cols());
  }
  BOOST_SERIALIZATION_SPLIT_MEMBER();
#endif  // TTG_SERIALIZATION_SUPPORTS_BOOST
};
