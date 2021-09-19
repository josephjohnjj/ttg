#ifndef BLOCKMATRIX_H
#define BLOCKMATRIX_H
#include "blockmatrix.h"
#endif /* BLOCKMATRIX_H */

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
              int local_mem_index = tile_num / processes;
              dcA[local_mem_index] = new BlockMatrix<T>(tile_rows, tile_cols);

              if(tile_num % 2 != 0)
                dcA[local_mem_index]->init_lower_pos_def_tile(block_rows * block_rows * tile_rows * tile_cols, m, n, 0);
              else
                dcA[local_mem_index]->set_empty();
            }
            
          }
        }
    }

    void fill(int m, int n, T val) const
    {
      assert(is_empty(m, n));
      assert(is_local(m, n));

      int tile_num = m * block_rows + n;
      int local_mem_index = tile_num / processes;
      dcA[local_mem_index] = new BlockMatrix<T>(tile_rows, tile_cols);
      dcA[local_mem_index]->fill(val);
    }

    void set_tile(int m, int n, BlockMatrix<T> *ptr)
    {
      assert(is_empty(m, n));
      assert(is_local(m, n));

      int tile_num = m * block_rows + n;
      int local_mem_index = tile_num / processes; 
      dcA[local_mem_index] = ptr;
    }

    void set_tile(int m, int n)
    {
      assert(is_empty(m, n));
      assert(is_local(m, n));

      int tile_num = m * block_rows + n;
      int local_mem_index = tile_num / processes; 
      dcA[local_mem_index] = new BlockMatrix<T>(tile_rows, tile_cols);
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

  bool is_empty(int m, int n) const 
  {
    int tile_num = m * block_rows + n;
    assert(is_local(m, n));  
    BlockMatrix<T> *ptr = dcA[tile_num / processes];
                      
    if(ptr == NULL || ptr == nullptr)
      return 1;
    else  
      return 0;
  }

  int rows(void) const 
  {
    return block_rows;
  }

  int cols(void) const 
  {
    return block_cols;
  }

  int t_rows(void) const 
  {
    return tile_rows;
  }

  int t_cols(void) const 
  {
    return tile_cols;
  }

  int get_p(void) const
  {
    return processes;
  }

  BlockMatrix<T>& operator()(int m, int n) const 
  {
    int tile_num = m * block_rows + n;
    assert(is_local(m, n));  
    BlockMatrix<T> *ptr = dcA[ tile_num / processes ];
    return *ptr; 
  }
};
