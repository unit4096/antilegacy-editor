#include <memory.h>
#include <tracer.h>
#include <vector>


/*
    This is the master of all vectors, The Pool Handler!
    Creates a non-growing (for now) pool.
*/

#ifndef ALE_POOL
#define ALE_POOL

namespace trc = ale::Tracer;

namespace ale {


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

    ~Pool() {
        /* trc::raw << "Pool size: " << _vector.size() << " type: " << typeid(T).name()<< "\n"; */
        // Delete all free nodes
        /* trc::raw << "Start pool delettion\n"; */

        while (_freeListHead !=nullptr) {
            FreeListNode* tmp = _freeListHead;
            _freeListHead = _freeListHead->next;
            delete tmp;
        }
        /* trc::raw << "Pool deleted\n"; */
    };


    void init(size_t size) {
        if (inited) {
            trc::log("ALREADY INITED", trc::ERROR);
            return;
        }

        _vector.resize(size);

        // Create nodes for free chunks (all are free!)
        for (int i = 0; i < size; i++) {
            FreeListNode* node = new FreeListNode;
            node->offset = &_vector[i];
            node->next = _freeListHead;
            _freeListHead = node;
        }
        inited = true;
    }


    T* request() {
        // No free chunks left
        if (_freeListHead == nullptr) {
            trc::raw << "Over capacity!\n";
            trc::raw << "Pool size: " << _vector.size() << " type: " << typeid(T).name()<< "\n";
            return nullptr;
        }


        // Delete free chunk node, get free chunk id

        auto offset = _freeListHead->offset;

        FreeListNode* tmp = _freeListHead;
        _freeListHead = _freeListHead->next;
        delete tmp;

        return offset;
    };

    void release(T* ptr) {
        FreeListNode* node = new FreeListNode;
        node->offset = ptr;
        node->next = _freeListHead;
        _freeListHead = node;
    };

private:
    bool inited = false;
    // Free list is a separate linked list that tracks which chunks are free
    struct FreeListNode {
        /* alignas(8) */
            T* offset;
        /* alignas(8) */
            FreeListNode* next;
    };

    std::vector<T> _vector;
    FreeListNode* _freeListHead;
};

} //namespace ale

#endif //ALE_POOL
