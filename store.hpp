#ifndef BPT_STORE_HPP
#define BPT_STORE_HPP

#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>

const int HASH_SIZE = 100003;
const int BLOCK_CAPACITY = 200;

struct HashBucket {
    long long head;
    long long tail;
};

struct Record {
    char index[64];
    int value;
};

struct BlockHeader {
    int count;
    long long next;
};

struct Block {
    BlockHeader header;
    Record records[BLOCK_CAPACITY];
};

FILE* file = nullptr;

unsigned long long hash_str(const char* str) {
    unsigned long long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash *33 + c
    }
    return hash;
}

void init_file() {
    file = fopen("data.dat", "rb+");
    if (!file) {
        file = fopen("data.dat", "wb+");
        HashBucket init_bucket = {0, 0};
        for (int i = 0; i < HASH_SIZE; ++i) {
            fseek(file, i * sizeof(HashBucket), SEEK_SET);
            fwrite(&init_bucket, sizeof(HashBucket), 1, file);
        }
        fflush(file);
    }
}

long long allocate_block() {
    fseek(file, 0, SEEK_END);
    long long offset = ftell(file);
    Block empty_block;
    memset(&empty_block, 0, sizeof(Block));
    fwrite(&empty_block, sizeof(Block), 1, file);
    fflush(file);
    return offset;
}

void process_insert(const char* index, int value) {
    unsigned long long h = hash_str(index) % HASH_SIZE;
    HashBucket bucket;
    fseek(file, h * sizeof(HashBucket), SEEK_SET);
    fread(&bucket, sizeof(HashBucket), 1, file);

    long long current_offset = bucket.head;
    bool exists = false;

    Block block;
    while (current_offset != 0) {
        fseek(file, current_offset, SEEK_SET);
        fread(&block, sizeof(Block), 1, file);
        for (int i = 0; i < block.header.count; ++i) {
            if (strcmp(block.records[i].index, index) == 0 && block.records[i].value == value) {
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
        Block new_block;
        new_block.header.count = 1;
        new_block.header.next = 0;
        strncpy(new_block.records[0].index, index, 64);
        new_block.records[0].value = value;

        fseek(file, new_offset, SEEK_SET);
        fwrite(&new_block, sizeof(Block), 1, file);

        bucket.head = bucket.tail = new_offset;
        fseek(file, h * sizeof(HashBucket), SEEK_SET);
        fwrite(&bucket, sizeof(HashBucket), 1, file);
    } else {
        fseek(file, bucket.tail, SEEK_SET);
        Block tail_block;
        fread(&tail_block, sizeof(Block), 1, file);
        if (tail_block.header.count < BLOCK_CAPACITY) {
            strncpy(tail_block.records[tail_block.header.count].index, index, 64);
            tail_block.records[tail_block.header.count].value = value;
            tail_block.header.count++;

            fseek(file, bucket.tail, SEEK_SET);
            fwrite(&tail_block, sizeof(Block), 1, file);
        } else {
            long new_offset = allocate_block();
            Block new_block;
            new_block.header.count = 1;
            new_block.header.next = 0;
            strncpy(new_block.records[0].index, index, 64);
            new_block.records[0].value = value;

            fseek(file, new_offset, SEEK_SET);
            fwrite(&new_block, sizeof(Block), 1, file);

            tail_block.header.next = new_offset;
            fseek(file, bucket.tail, SEEK_SET);
            fwrite(&tail_block, sizeof(Block), 1, file);

            bucket.tail = new_offset;
            fseek(file, h * sizeof(HashBucket), SEEK_SET);
            fwrite(&bucket, sizeof(HashBucket), 1, file);
        }
    }
    fflush(file);
}

void process_delete(const char* index, int value) {
    unsigned long h = hash_str(index) % HASH_SIZE;
    HashBucket bucket;
    fseek(file, h * sizeof(HashBucket), SEEK_SET);
    fread(&bucket, sizeof(HashBucket), 1, file);

    long current_offset = bucket.head;
    long prev_offset = 0;
    bool found = false;

    while (current_offset != 0 && !found) {
        Block block;
        fseek(file, current_offset, SEEK_SET);
        fread(&block, sizeof(Block), 1, file);

        for (int i = 0; i < block.header.count; ++i) {
            if (strcmp(block.records[i].index, index) == 0 && block.records[i].value == value) {
                for (int j = i; j < block.header.count - 1; ++j) {
                    block.records[j] = block.records[j + 1];
                }
                block.header.count--;
                found = true;

                fseek(file, current_offset, SEEK_SET);
                fwrite(&block, sizeof(Block), 1, file);

                if (block.header.count == 0) {
                    if (prev_offset == 0) {
                        bucket.head = block.header.next;
                        if (bucket.head == 0) bucket.tail = 0;
                        fseek(file, h * sizeof(HashBucket), SEEK_SET);
                        fwrite(&bucket, sizeof(HashBucket), 1, file);
                    } else {
                        Block prev_block;
                        fseek(file, prev_offset, SEEK_SET);
                        fread(&prev_block, sizeof(Block), 1, file);
                        prev_block.header.next = block.header.next;
                        fseek(file, prev_offset, SEEK_SET);
                        fwrite(&prev_block.header, sizeof(BlockHeader), 1, file);

                        if (block.header.next == 0) {
                            bucket.tail = prev_offset;
                            fseek(file, h * sizeof(HashBucket), SEEK_SET);
                            fwrite(&bucket, sizeof(HashBucket), 1, file);
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
    fflush(file);
}

void process_find(const char* index) {
    vector<int> values;
    unsigned long h = hash_str(index) % HASH_SIZE;
    HashBucket bucket;
    fseek(file, h * sizeof(HashBucket), SEEK_SET);
    fread(&bucket, sizeof(HashBucket), 1, file);

    long current_offset = bucket.head;
    while (current_offset != 0) {
        Block block;
        fseek(file, current_offset, SEEK_SET);
        fread(&block, sizeof(Block), 1, file);
        for (int i = 0; i < block.header.count; ++i) {
            if (strcmp(block.records[i].index, index) == 0) {
                values.push_back(block.records[i].value);
            }
        }
        current_offset = block.header.next;
    }

    if (values.empty()) {
        printf("null\n");
    } else {
        sort(values.begin(), values.end());
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) printf(" ");
            printf("%d", values[i]);
        }
        printf("\n");
    }
}

int main() {
    init_file();
    int n;
    scanf("%d", &n);
    while (n--) {
        char cmd[10];
        scanf("%s", cmd);
        if (strcmp(cmd, "insert") == 0) {
            char index[64];
            int value;
            scanf("%s %d", index, &value);
            process_insert(index, value);
        } else if (strcmp(cmd, "delete") == 0) {
            char index[64];
            int value;
            scanf("%s %d", index, &value);
            process_delete(index, value);
        } else if (strcmp(cmd, "find") == 0) {
            char index[64];
            scanf("%s", index);
            process_find(index);
        }
    }
    if (file) fclose(file);
    return 0;
}