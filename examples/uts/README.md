Implements UTS benchmark with binomial distribution.
We use only Dr. Brian R. Gladman's SHA-1 implementation. 

The rng header files in uts/rng has been resused direct from
https://git.code.sf.net/p/uts-benchmark/code. 


RUNNING UTS
==============
mpirun -np 2 ./examples/uts-parsec -m 5 -b 5 -r 1 -q .234 -g 1

where 
b is the children of the root node
m is the number of children of each tree node
r is the initial seed to the rng function
q is the probablity threshold deciding whether a tree node will have children or not
g is the granilarity of a tree node

mpirun -np 2 ./examples/uts-parsec 

will run with default values

Olivier S. et al. (2007) UTS: An Unbalanced Tree Search Benchmark. In: Almási G., Caşcaval C., Wu P. (eds) Languages and Compilers for Parallel Computing. LCPC 2006. Lecture Notes in Computer Science, vol 4382. Springer, Berlin, Heidelberg. https://doi.org/10.1007/978-3-540-72521-3_18

      