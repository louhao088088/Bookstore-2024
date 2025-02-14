#include <cstdio>
#include <cstring>
#include <vector>
#include <algorithm>
#include <string>
#include <fstream>
using namespace std;

const int HASH_SIZE = 100003;
const int BLOCK_CAPACITY = 200;

// MemoryRiver.hpp 保持不变
template<typename T>
class MemoryRiver {
private:
    fstream file;
    string filename;
    
public:
    explicit MemoryRiver(const string& name) : filename(name) {
        file.open(filename, ios::binary | ios::in | ios::out);
        if (!file) {
            file.open(filename, ios::binary | ios::out);
            file.close();
            file.open(filename, ios::binary | ios::in | ios::out);
        }
    }

    ~MemoryRiver() {
        if (file.is_open()) file.close();
    }

    long write(const T& data) {
        file.seekp(0, ios::end);
        long pos = file.tellp();
        file.write(reinterpret_cast<const char*>(&data), sizeof(T));
        file.flush();
        return pos;
    }

    void read(T& data, long pos) {
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(&data), sizeof(T));
    }

    void update(const T& data, long pos) {
        file.seekp(pos);
        file.write(reinterpret_cast<const char*>(&data), sizeof(T));
        file.flush();
    }

    bool is_new_file() {
        file.seekg(0, ios::end);
        return file.tellg() == 0;
    }
};

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
    // 修改构造函数，接受文件名参数
    explicit Database(const string& filename) 
        : hashRiver(filename), blockRiver(filename) {
        if (hashRiver.is_new_file()) {
            HashBucket init_bucket = {0, 0};
            for (int i = 0; i < HASH_SIZE; ++i) {
                hashRiver.write(init_bucket);
            }
        }
    }

    // 原有插入、删除、查找方法保持不变
    void insert(const char* index, T value) {
        /* 同原代码 */
    }

    void remove(const char* index, T value) {
        /* 同原代码 */
    }

    void find(const char* index) {
        /* 同原代码 */
    }
};

int main() {
    // 创建不同数据库实例时使用不同文件名
    Database<int> db1("db1.dat");
    Database<int> db2("db2.dat");
    
    // 后续操作与之前相同
    int n;
    scanf("%d", &n);
    while (n--) {
        char cmd[10];
        scanf("%s", cmd);
        if (strcmp(cmd, "insert") == 0) {
            char index[64];
            int value;
            scanf("%s %d", index, &value);
            // 这里示例仅使用db1，实际可根据需要选择数据库实例
            db1.insert(index, value);
        } else if (strcmp(cmd, "delete") == 0) {
            char index[64];
            int value;
            scanf("%s %d", index, &value);
            db1.remove(index, value);
        } else if (strcmp(cmd, "find") == 0) {
            char index[64];
            scanf("%s", index);
            db1.find(index);
        }
    }
    return 0;
}