#include "miniamr_ht.h"

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
    parsec_hash_table_init(data_table, offsetof(data_item_t, ht_item), 8, ht_fcts, NULL);
    printf(" HT %p \n", data_table);
}

void delete_ht()
{
    parsec_hash_table_fini(data_table);
}


//data_id = 0 -> main block of octant data
//data_id 1..6 -> six shared faces (ghost regions)

void insert_ht_data(int data_id, Key octant_key, void* data)
{
    parsec_data_copy_t *copy = PARSEC_OBJ_NEW(parsec_data_copy_t);
    copy->device_private = data;
    
    data_item_t* ht_data_item = NULL;
    parsec_key_t ht_key = reinterpret_cast<parsec_key_t>(octant_key.hash());
    parsec_hash_table_lock_bucket(data_table, ht_key);
    ht_data_item = (data_item_t *) parsec_hash_table_nolock_find(data_table, ht_key);

    if (NULL == ht_data_item) 
    {
        ht_data_item = PARSEC_OBJ_NEW(data_item_t);
        ht_data_item->data[data_id] = copy;
        ht_data_item->ht_item.key = octant_key.hash();
        parsec_hash_table_nolock_insert(data_table, &ht_data_item->ht_item);
        parsec_hash_table_unlock_bucket(data_table, ht_key);
        
    }
    else
    {
        ht_data_item->data[data_id] = copy;
        parsec_hash_table_unlock_bucket(data_table, ht_key);
    }
}


void* get_ht_data(int data_id, Key octant_key)
{
    parsec_data_copy_t *copy;
    data_item_t* ht_data_item = NULL;
    parsec_hash_table_lock_bucket(data_table, octant_key.hash());
    parsec_key_t ht_key = (parsec_key_t) octant_key.hash() + 2;
    ht_data_item = (data_item_t *) parsec_hash_table_find(data_table, ht_key);

    if (NULL == ht_data_item) 
    {
        printf("Something is wrong in HT \n");
        parsec_hash_table_unlock_bucket(data_table, ht_key);
        
    }
    else
    {
        copy = ht_data_item->data[data_id];
        parsec_hash_table_unlock_bucket(data_table, ht_key);
        return copy->device_private;
    }
}



