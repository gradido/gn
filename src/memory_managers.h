#ifndef MEMORY_MANAGERS_H
#define MEMORY_MANAGERS_H

#include <cstddef>
#include <forward_list>

// TODO: remove
#include <vector>
#include <cstdint>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

// TODO: remove
#include <iostream>

namespace gradido {

    // those containers attempt to zero objects they return; if T is
    // polymorphic, reset_to_zero() method is called; otherwise it is
    // memset()

    // assumes allocated blocks won't need to be released during lifetime
    // of this object; block contents is initialized to 0, without
    // calling T constructor
    //
    // T cannot be array
    template<typename T>
    class PermanentBlockAllocator {
    private:
        std::forward_list<T*> blocks;
    public:

        const size_t block_size;

        PermanentBlockAllocator(size_t block_size) : 
        block_size(block_size) {}

        ~PermanentBlockAllocator() {
            for (auto i : blocks) {
                delete [] i;
            }
            blocks.clear();
        }

        template<typename T2=T>
        typename std::enable_if<std::is_polymorphic<T2>::value, void>::type
        allocate(T2** out) {
            *out = new T2[block_size];
            for (int i = 0; i < block_size; i++)
                (*out)[i].reset_to_zero();
            blocks.push_front(*out);
        }

        template<typename T2=T>
        typename std::enable_if<!std::is_polymorphic<T2>::value, void>::type
        allocate(T2** out) {
            *out = new T2[block_size];
            memset(*out, 0, sizeof(T2) * block_size);
            blocks.push_front(*out);
        }
    };

    // assumes T[TCount] is a large object, allocating them only when it 
    // is necessary, allowing instances of T[Count] to be released, 
    // reusing memory; it is possible to reduce overall used memory by 
    // releasing unused T instances, with calling release_unused(); need 
    // to provide optimal ratio to that method
    //
    // T should be valid if all full of 0
    //
    // - if TCount == 1, object is operated as a scalar, otherwise T* is
    //   array
    // - object data is zeroed when object is released
    // - user object has to take care about calling release() for each
    //   allocated object; destructor of this class won't destroy them
    template<typename T, int TCount>
    class BigObjectMemoryPool {
    private:

        struct Item {
            Item() : next(0), payload(0) {}
            Item* next;
            T* payload;
        };

        class ItemList {
        public:
            ItemList() : first(0), last(0), count(0) {}
            ~ItemList() {
                Item* p = first;
                while (p) {
                    Item* p2 = p->next;
                    if (p->payload)
                        if constexpr (TCount == 1)
                            delete p->payload;
                        else delete [] p->payload;
                    p = p2;
                }
            }
            Item* first;
            Item* last;
            size_t count;
            void dispose_first() {
                if (first) {
                    Item* p = first->next;
                    if (first->payload)
                        if constexpr (TCount == 1)
                            delete first->payload;
                        else delete [] first->payload;
                    first = p;
                    count--;
                }
            }

            void dispose_first(ItemList& into) {
                if (first) {
                    Item* p = first->next;
                    if (first->payload)
                        if constexpr (TCount == 1)
                            delete first->payload;
                        else delete [] first->payload;
                    first->payload = 0;
                    into.push(first);
                    first = p;
                    count--;
                }
            }

            void push(Item* item) {
                if (first)
                    item->next = first;
                else {
                    item->next = 0;
                    last = item;
                }
                first = item;
                count++;
            }

            Item* pop() {
                Item* res = first;
                first = first->next;
                res->next = 0;
                count--;
                return res;
            }
        };

        PermanentBlockAllocator<Item> item_allocator;
        ItemList no_payload;
        ItemList ready;
        ItemList used;

        template<typename T2=T, int TCount2=TCount>
        typename std::enable_if<std::is_polymorphic<T2>::value, void>::type
        reset_to_zero(T2* rec) {
            for (int i = 0; i < TCount2; i++)
                rec[i].reset_to_zero();
        }

        template<typename T2=T, int TCount2=TCount>
        typename std::enable_if<!std::is_polymorphic<T2>::value, void>::type
        reset_to_zero(T2* rec) {
            memset(rec, 0, sizeof(T2) * TCount2);
        }


    public:
        BigObjectMemoryPool(size_t item_block_size) : 
        item_allocator(item_block_size) {
            if constexpr (TCount < 1)
                PRECISE_THROW("BigObjectMemoryPool: TCount < 1");
        }

        ~BigObjectMemoryPool() {
            release_unused(0);
        }

        // not_exceed_ratio is ready.count / used.count, which should not
        // be exceeded; 1.0 would mean to not have more ready items than
        // used item; 0.1 would mean to not have more ready items than
        // 1/10 of used items; ratio of 0 clears all ready items
        void release_unused(float not_exceed_ratio) {
            if (not_exceed_ratio < 0)
                not_exceed_ratio = 0;
            int rc = (int)(not_exceed_ratio * used.count);
            int crc = ready.count;
            for (int i = rc; i < crc; i++)
                ready.dispose_first(no_payload);
        }

        T* get() {
            T* res = 0;

            if (ready.count > 0) {
                Item* item = ready.pop();
                res = item->payload;
                reset_to_zero(res);
                item->payload = 0;
                used.push(item);
            } else if (no_payload.count > 0) {
                Item* item = no_payload.pop();
                item->payload = 0;
                used.push(item);
                if constexpr (TCount == 1) {
                    res = new T();
                 } else {
                    res = new T[TCount];
                }
                reset_to_zero(res);
            } else {
                Item* new_items;
                item_allocator.allocate(&new_items);

                for (int i = 0; i < item_allocator.block_size; i++)
                    no_payload.push(new_items + i);
                return get();
            }
            return res;
        }

        void release(T* obj) {
            if (used.count == 0)
                PRECISE_THROW("BigObjectMemoryPool: cannot release");
            Item* item = used.pop();
            item->payload = obj;
            ready.push(item);
        }
    };
}

#endif

