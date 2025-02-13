#pragma once
#include "MemoryRiver.hpp"
using namespace std;

const int BLOCK_CAPACITY = 200;

// 原始头文件内容 MemoryRiver.hpp 放在这里


struct HashBucket {
    long head;
    long tail;
};

template<typename T>
struct Record {
    char index[64];
    T value;
};

struct BlockHeader {
    int count;
    long next;
};

template<typename T>
struct Block {
    BlockHeader header;
    Record<T> records[BLOCK_CAPACITY];
};

template<typename T>
class Database {
private:
    MemoryRiver<HashBucket> hashRiver;
    MemoryRiver<Block<T>> blockRiver;

    unsigned long hash_str(const char* str) {
        unsigned long hash = 5381;
        int c;
        while ((c = *str++)) {
            hash = ((hash << 5) + hash) + c;
        }
        return hash;
    }

    long allocate_block() {
        Block<T> empty_block;
        memset(&empty_block, 0, sizeof(Block<T>));
        return blockRiver.write(empty_block);
    }

public:
    Database() : hashRiver("data.dat"), blockRiver("data.dat") {
        if (hashRiver.is_new_file()) {
            HashBucket init_bucket = {0, 0};
            for (int i = 0; i < HASH_SIZE; ++i) {
                hashRiver.write(init_bucket);
            }
        }
    }

    void insert(const char* index, T value) {
        unsigned long h = hash_str(index) % HASH_SIZE;
        HashBucket bucket;
        hashRiver.read(bucket, h * sizeof(HashBucket));

        long current_offset = bucket.head;
        bool exists = false;
        Block<T> block;

        while (current_offset != 0) {
            blockRiver.read(block, current_offset);
            for (int i = 0; i < block.header.count; ++i) {
                if (strcmp(block.records[i].index, index) == 0) {
                    exists = true;
                    break;
                }
            }
            if (exists) break;
            current_offset = block.header.next;
        }

        if (exists) return;

        if (bucket.tail == 0) {
            long new_offset = allocate_block();
            Block<T> new_block;
            new_block.header.count = 1;
            new_block.header.next = 0;
            strncpy(new_block.records[0].index, index, 64);
            new_block.records[0].value = value;

            blockRiver.update(new_block, new_offset);

            bucket.head = bucket.tail = new_offset;
            hashRiver.update(bucket, h * sizeof(HashBucket));
        } else {
            Block<T> tail_block;
            blockRiver.read(tail_block, bucket.tail);
            
            if (tail_block.header.count < BLOCK_CAPACITY) {
                strncpy(tail_block.records[tail_block.header.count].index, index, 64);
                tail_block.records[tail_block.header.count].value = value;
                tail_block.header.count++;
                blockRiver.update(tail_block, bucket.tail);
            } else {
                long new_offset = allocate_block();
                Block<T> new_block;
                new_block.header.count = 1;
                new_block.header.next = 0;
                strncpy(new_block.records[0].index, index, 64);
                new_block.records[0].value = value;

                blockRiver.update(new_block, new_offset);

                tail_block.header.next = new_offset;
                blockRiver.update(tail_block, bucket.tail);

                bucket.tail = new_offset;
                hashRiver.update(bucket, h * sizeof(HashBucket));
            }
        }
    }

    void erase(const char* index) {
        unsigned long h = hash_str(index) % HASH_SIZE;
        HashBucket bucket;
        hashRiver.read(bucket, h * sizeof(HashBucket));

        long current_offset = bucket.head;
        long prev_offset = 0;
        bool found = false;
        Block<T> block;
        
        while (current_offset != 0 && !found) {
            blockRiver.read(block, current_offset);
            for (int i = 0; i < block.header.count; ++i) {
                if (strcmp(block.records[i].index, index) == 0) {
                    
                    for (int j = i; j < block.header.count - 1; ++j) {
                        block.records[j] = block.records[j + 1];
                    }
                    block.header.count--;
                    found = true;
                    blockRiver.update(block, current_offset);

                    if (block.header.count == 0) {
                        if (prev_offset == 0) {
                            bucket.head = block.header.next;
                            if (bucket.head == 0) bucket.tail = 0;
                            hashRiver.update(bucket, h * sizeof(HashBucket));
                        } else {
                            Block<T> prev_block;
                            blockRiver.read(prev_block, prev_offset);
                            prev_block.header.next = block.header.next;
                            blockRiver.update(prev_block, prev_offset);

                            if (block.header.next == 0) {
                                bucket.tail = prev_offset;
                                hashRiver.update(bucket, h * sizeof(HashBucket));
                            }
                        }
                    }
                    break;
                }
            }
            if (!found) {
                prev_offset = current_offset;
                current_offset = block.header.next;
            }
        }
    }
    bool check(const char* index) {
        vector<T> values;
        unsigned long h = hash_str(index) % HASH_SIZE;
        HashBucket bucket;
        hashRiver.read(bucket, h * sizeof(HashBucket));

        long current_offset = bucket.head;
        Block<T> block;

        while (current_offset != 0) {
            blockRiver.read(block, current_offset);
            for (int i = 0; i < block.header.count; ++i) {
                if (strcmp(block.records[i].index, index) == 0) {
                    values.push_back(block.records[i].value);
                }
            }
            current_offset = block.header.next;
        }

        if (values.empty()) {
            return 0;
        }
        return 1;
    }

    T find(const char* index){
        vector<T> values;
        unsigned long h = hash_str(index) % HASH_SIZE;
        HashBucket bucket;
        hashRiver.read(bucket, h * sizeof(HashBucket));

        long current_offset = bucket.head;
        Block<T> block;

        while (current_offset != 0) {
            blockRiver.read(block, current_offset);
            for (int i = 0; i < block.header.count; ++i) {
                if (strcmp(block.records[i].index, index) == 0) {
                    values.push_back(block.records[i].value);
                }
            }
            current_offset = block.header.next;
        }
        return values[0];
    }
};