#include "src/database.hpp"
#include <bits/stdc++.h>
using namespace std;
int main() {
    Database<int> db;
    int n;
    scanf("%d", &n);
    while (n--) {
        char cmd[10];
        scanf("%s", cmd);
        if (strcmp(cmd, "insert") == 0) {
            char index[64];
            int value;
            scanf("%s %d", index, &value);
            db.insert(index, value);
        } else if (strcmp(cmd, "delete") == 0) {
            char index[64];
            int value;
            scanf("%s %d", index, &value);
            db.remove(index, value);
        } else if (strcmp(cmd, "find") == 0) {
            char index[64];
            scanf("%s", index);
            db.find(index);
        }
    }
    return 0;
}