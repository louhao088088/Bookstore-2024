#pragma once
#include <fstream>
using namespace std;
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
        return pos;
    }

    void read(T& data, long pos) {
        file.seekg(pos);
        file.read(reinterpret_cast<char*>(&data), sizeof(T));
    }

    void update(const T& data, long pos) {
        file.seekp(pos);
        file.write(reinterpret_cast<const char*>(&data), sizeof(T));
    }
};

