#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <stack>
#include <ctime>
#include <iomanip>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <algorithm>
#include "src/CommandParser.hpp"
#include <bits/stdc++.h>

using namespace std;



// 文件存储管理器模板



// 异步日志系统




// 核心系统类







int main() {
    BookstoreSystem system;
    string line;
    int UUU=0;
    while (getline(cin, line)) {
        UUU++;
        cout<<UUU<<endl;
        if (line.empty()) continue;
        CommandParser::parse(system, line);
    }
    return 0;
}