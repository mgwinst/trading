#include <unordered_map>
#include <vector>

std::unordered_map<InstrumentId, Instrument> mInstruments;
// Upside: stable pointers, cheap lookup
// Downsides: all elements of hashmap will be scattered all over memory 

std::vector<Instrument> mInstruments;
// Upsides: contiguous memory
// Downsides: expensive resize

stable_vector<Instrument> mInstruments;
// stable_vector NOT boost::stable_vector (= node container)
// Downsides:: overhead on iterators operations
