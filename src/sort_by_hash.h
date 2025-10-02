#include <cassert>
#include <cstdint>
#include <vector>
#include "datatypes.h"
#include "hash_integers.h"

/*
  Sort an array of particles by destination rank, where the destination MPI
  rank is determined by (HashInteger(p.Id) % comm_size). The input Particle_t type
  should have an Id field as well as a cached RankId field storing the destination
  rank computed elsewhere. The cached value is used when counting and scattering
  to avoid recomputing the hash, with a sanity check to guard against stale data.

  Returns an array with the offset to the first particle to go to each rank.
*/
template <typename Particle_t>
std::vector<HBTInt> sort_by_hash(std::vector<Particle_t> &Particles, int comm_size)
{

  // First we need to count the number of elements to send to each rank
  std::vector<HBTInt> count(comm_size, 0);
  for (HBTInt i = 0; i < Particles.size(); i += 1)
  {
    int dest = static_cast<int>(Particles[i].RankId);
    if (dest >= comm_size)
    {
      dest = RankFromIdHash(Particles[i].Id, comm_size);
      Particles[i].RankId = static_cast<uint16_t>(dest);
    }
    assert(dest >= 0 && dest < comm_size);
    count[dest] += 1;
  }

  // Compute offset to the first particle for each rank in the output
  std::vector<HBTInt> offset(comm_size, 0);
  for (int i = 1; i < comm_size; i += 1)
  {
    offset[i] = offset[i - 1] + count[i - 1];
  }

  // Allocate storage for the sorted array
  std::vector<Particle_t> SortedParticles(Particles.size());

  // Reset the counts
  std::fill(count.begin(), count.end(), 0);

  // Populate the output array in sorted order
  // TODO: can we parallelize this somehow?
  for (HBTInt i = 0; i < Particles.size(); i += 1)
  {
    int dest = static_cast<int>(Particles[i].RankId);
    if (dest >= comm_size)
    {
      dest = RankFromIdHash(Particles[i].Id, comm_size);
      Particles[i].RankId = static_cast<uint16_t>(dest);
    }
    assert(dest >= 0 && dest < comm_size);
    HBTInt j = offset[dest] + count[dest];
    SortedParticles[j] = Particles[i];
    count[dest] += 1;
  }

  // Replace the original array
  Particles.swap(SortedParticles);

  return offset;
}
