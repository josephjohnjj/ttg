#include <parsec/class/parsec_hash_table.h>

#ifndef MINIAMR_H
#define MINIAMR_H
#include "miniamr.h"
#endif /* MINIAMR_H */

#ifndef TTG_H
#define TTG_H
#include "ttg.h"
#endif /* TTG_H */
#include "ttg/fwd.h"


parsec_hash_table_t data_table;

typedef struct data_item_s
{
    parsec_list_item_t next;
    Key octant_key;
    parsec_hash_table_item_t ht_item;
    parsec_data_copy_t *data[7];
} data_item_t;
PARSEC_DECLSPEC PARSEC_OBJ_CLASS_DECLARATION(data_item_t);
PARSEC_OBJ_CLASS_INSTANCE(data_item_t, parsec_list_item_t, NULL, NULL);

static int ht_key_equal(parsec_key_t a, parsec_key_t b, void *user_data) 
{
    Key ka = *(reinterpret_cast<Key *>(a));
    Key kb = *(reinterpret_cast<Key *>(b));
    return ka == kb;
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
        Key kk = *(reinterpret_cast<Key *>(k));
        using ttg::hash;
        uint64_t hv = hash<decltype(kk)>{}(kk);
        return hv;
    }

parsec_key_fn_t ht_fcts = {ht_key_equal, ht_key_print, ht_key_hash};

void create_ht()
{
    parsec_hash_table_init(&data_table, offsetof(data_item_t, ht_item), 8, ht_fcts, NULL);
}

void create_ht()
{
    parsec_hash_table_fini(&data_table);
}


//data_id = 0 -> main block of octant data
//data_id 1..6 -> six shared faces (ghost regions)

void insert_octant_data(int data_id, Key octant_key, void* data)
{
    parsec_data_copy_t *copy = PARSEC_OBJ_NEW(parsec_data_copy_t);
    copy->device_private = data;
  
    data_item_t* ht_data_item = NULL;
    parsec_hash_table_lock_bucket(&data_table, octant_key.hash());
    ht_data_item = (data_item_t *) parsec_hash_table_find(&data_table, octant_key.hash());

    if (NULL == ht_data_item) 
    {
        data_item_t* ht_data_item = PARSEC_OBJ_NEW(data_item_t);
        ht_data_item->data[data_id] = copy;
        ht_data_item->ht_item.key = octant_key.hash();
        parsec_hash_table_nolock_insert(&data_table, &ht_data_item->ht_item);
        parsec_hash_table_unlock_bucket(&data_table, octant_key.hash());
    }
    else
    {
        ht_data_item->data[data_id] = copy;
        parsec_hash_table_unlock_bucket(&data_table, octant_key.hash());
    }
}



