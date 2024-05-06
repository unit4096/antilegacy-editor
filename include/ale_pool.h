#include <memory.h>
#include <tracer.h>
#include <vector>
#include <forward_list>


/*
    This is the master of all vectors, The Pool Handler!
    Creates a non-growing (for now) pool.
*/

#ifndef ALE_POOL
#define ALE_POOL

namespace trc = ale::Tracer;

namespace ale {

// TODO: Store _freeList as a bitmap. Store it inside empty cells of _vector


// T is a size of a chunk
template <class T>
class Pool {
public:
    Pool(size_t poolSize = 0) {
        // Resize vector to a required pool size
        if (poolSize > 0) {
            init(poolSize);
        }
    };

    ~Pool() {};


    void init(size_t size) {
        if (inited) {
            std::string currentType = (typeid(T).name());
            std::string currentSize = std::to_string(_vector.size());
            trc::log("POOL OF TYPE: " + currentType
                   + " ALREADY INITED TO SIZE: " + currentSize, trc::ERROR);
            return;
        }

        _vector.resize(size);
        // Create nodes for free chunks (all are free!)
        for(size_t i = 0; i < _vector.size(); i++) {
            _freeList.push_front(i);
        }

        inited = true;
    }


    T* request(size_t& id) {
        // No free chunks left
        if (_freeList.empty()) {
            trc::raw << "Over capacity!\n";
            trc::raw << "Pool size: " << _vector.size() << " type: " << typeid(T).name()<< "\n";
            return nullptr;
        }

        // Delete free chunk node, get free chunk id
        size_t _id = _freeList.front();

        id = _id;
        _freeList.pop_front();
        return &_vector[_id];
    };

    void release(size_t id) {
        _freeList.push_front(id);
    };

private:
    bool inited = false;
    // internal vector with data
    std::vector<T> _vector;
    // Free list is a separate linked list that tracks which chunks are free
    std::forward_list<size_t> _freeList;
};

} //namespace ale

#endif //ALE_POOL
