#include <memory>
#include <vector>

/*
template <typename T, size_t ChunkSize>
struct stable_vector {
    static_assert(ChunkSize % 2 == 0, "Chunksize needs to be a multiplier of 2");

    reference operator[](size_type i) {
        return (*mChunks[i / ChunkSize])[i % ChunkSize];
    }

    // ...

    using Chunk = boost::container::static_vector<T, ChunkSize>;
    std::vector<std::unique_ptr<Chunk>> mChunks;

}
*/
