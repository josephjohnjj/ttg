#ifndef KEY_H
#define KEY_H
#include "key.h"
#endif /* KEY_H */

struct Octant_Aggregator_Data
{
  Key octant_key;
  Key parent_key;

  Octant_Aggregator_Data() = default;
  Octant_Aggregator_Data(Key key1, Key key2) : 
    octant_key(key1), parent_key(key2){}

  bool operator==(const Octant_Aggregator_Data& b) const 
      { return octant_key == b.octant_key && parent_key == b.parent_key; }
      
  bool operator!=(const Octant_Aggregator_Data& b) const { return !((*this) == b); }

  template <typename Archive>
  void serialize(Archive& ar) {
    ar& madness::archive::wrap((unsigned char*)this, sizeof(*this));
  }

};