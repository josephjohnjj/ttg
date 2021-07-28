#include <parsec/class/parsec_hash_table.h>

#ifndef KEY_H
#define KEY_H
#include "key.h"
#endif /* KEY_H */



static parsec_hash_table_t* data_table;

struct HT_entry
{
    parsec_list_item_t next;
    Key octant_key;
    parsec_hash_table_item_t ht_item;
    void *block;
    void *face1;
    void *face2;
    void *face3;
    void *face4;
    void *face5;
    void *face6;
} ;


static int ht_key_equal(parsec_key_t a, parsec_key_t b, void *user_data) 
{
    //Key ka = *(reinterpret_cast<Key *>(a));
    //Key kb = *(reinterpret_cast<Key *>(b));
    //return ka == kb;
    return a == b;
}

static char *ht_key_print(char *buffer, size_t buffer_size, parsec_key_t k, void *user_data) 
{   
    Key kk = *(reinterpret_cast<Key *>(k));
    std::stringstream iss;
    iss << kk;
    memset(buffer, 0, buffer_size);
    iss.get(buffer, buffer_size);
    return buffer; 
}

    static uint64_t ht_key_hash(parsec_key_t k, void *user_data) 
    {
        return k;
    }

parsec_key_fn_t ht_fcts = {ht_key_equal, ht_key_print, ht_key_hash};

void create_ht()
{
    data_table = (parsec_hash_table_t*)calloc(1, sizeof(parsec_hash_table_t));
    parsec_hash_table_init(data_table, offsetof(HT_entry, ht_item), 8, ht_fcts, NULL);
}

void delete_ht()
{
    parsec_hash_table_fini(data_table);
}


//data_id = 0 -> main block of octant data
//data_id 1..6 -> six shared faces (ghost regions)

void insert_ht_data(int data_id, Key octant_key, void* data)
{

    HT_entry* ht_data_item = static_cast<HT_entry *>(NULL);
    parsec_key_t ht_key = octant_key.ht_key();
    parsec_hash_table_lock_bucket(data_table, ht_key);
    ht_data_item = (HT_entry *) parsec_hash_table_nolock_find(data_table, ht_key);

    if (NULL == ht_data_item) 
    {
        printf("HT-DEBUG : Data inserted Key(x=%d, y=%d, z=%d, l=%d, ts=%d)\n",
           octant_key.x, octant_key.y, octant_key.z, octant_key.l, octant_key.ts);
            
        ht_data_item = new HT_entry();
        for(int i = 0; i < 7 ;i++)  
        {
            ht_data_item->block = static_cast<void *>(NULL);
            ht_data_item->face1 = static_cast<void *>(NULL);
            ht_data_item->face2 = static_cast<void *>(NULL);
            ht_data_item->face3 = static_cast<void *>(NULL);
            ht_data_item->face4 = static_cast<void *>(NULL);
            ht_data_item->face5 = static_cast<void *>(NULL);
            ht_data_item->face6 = static_cast<void *>(NULL);
        }  
       
        if( data_id == 0) {ht_data_item->block = data;}
        if( data_id == 1) {ht_data_item->face1 = data;}
        if( data_id == 2) {ht_data_item->face2 = data;}
        if( data_id == 3) {ht_data_item->face3 = data;}
        if( data_id == 4) {ht_data_item->face4 = data;}
        if( data_id == 5) {ht_data_item->face5 = data;}
        if( data_id == 6) {ht_data_item->face6 = data;}

        ht_data_item->ht_item.key = ht_key;
        parsec_hash_table_nolock_insert(data_table, &ht_data_item->ht_item);
        parsec_hash_table_unlock_bucket(data_table, ht_key);
 
    }
    else
    {
        if( data_id == 0) {ht_data_item->block = data;}
        if( data_id == 1) {ht_data_item->face1 = data;}
        if( data_id == 2) {ht_data_item->face2 = data;}
        if( data_id == 3) {ht_data_item->face3 = data;}
        if( data_id == 4) {ht_data_item->face4 = data;}
        if( data_id == 5) {ht_data_item->face5 = data;}
        if( data_id == 6) {ht_data_item->face6 = data;}

        parsec_hash_table_unlock_bucket(data_table, ht_key);
    }
}


void* get_ht_data(int data_id, Key octant_key)
{
    HT_entry* ht_data_item = static_cast<HT_entry *>(NULL);
    parsec_key_t ht_key = (parsec_key_t) octant_key.ht_key();

    parsec_hash_table_lock_bucket(data_table, ht_key);
    ht_data_item = (HT_entry *) parsec_hash_table_nolock_find(data_table, ht_key);

    if (NULL == ht_data_item) 
    {
        printf("HT-DEBUG : No HT item for Key(x=%d, y=%d, z=%d, l=%d, ts=%d)\n",
            octant_key.x, octant_key.y, octant_key.z, octant_key.l, octant_key.ts);

        parsec_hash_table_unlock_bucket(data_table, ht_key);
        return nullptr;
        
    }
    else
    {
        //if(copy == nullptr)
        //    printf("HT-DEBUG : No HT item for Key(x=%d, y=%d, z=%d, l=%d, ts=%d)\n",
        //        octant_key.x, octant_key.y, octant_key.z, octant_key.l, octant_key.ts);
                
        parsec_hash_table_unlock_bucket(data_table, ht_key);

        if( data_id == 0) {return ht_data_item->block;}
        if( data_id == 1) {return ht_data_item->face1;}
        if( data_id == 2) {return ht_data_item->face2;}
        if( data_id == 3) {return ht_data_item->face3;}
        if( data_id == 4) {return ht_data_item->face4;}
        if( data_id == 5) {return ht_data_item->face5;}
        if( data_id == 6) {return ht_data_item->face6;}
    }
}



