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
#include "MemoryRiver.hpp"
#include <bits/stdc++.h>

using namespace std;

// 常量定义
const string USER_FILE = "users.dat";
const string BOOK_FILE = "books.dat";
const string FINANCE_LOG = "finance.log";
const string OPERATION_LOG = "operation.log";
const int HASH_SIZE = 100003;
const int BLOCK_SIZE = 4096;

// 权限等级
enum Privilege { GUEST = 0, CUSTOMER = 1, STAFF = 3, ROOT = 7 };

// 用户账户结构
struct User {
    char userID[31] = {0};
    char password[31] = {0};
    char username[31] = {0};
    int privilege = 0;
    int next = -1;

    User() = default;
    User(const string& id, const string& pwd, const string& name, int priv)
        : privilege(priv) {
        strncpy(userID, id.c_str(), 30);
        strncpy(password, pwd.c_str(), 30);
        strncpy(username, name.c_str(), 30);
    }
};

// 图书信息结构
struct Book {
    char ISBN[21] = {0};
    char name[61] = {0};
    char author[61] = {0};
    char keywords[61] = {0};
    double price = 0.0;
    int quantity = 0;
    int next = -1;

    Book() = default;
    explicit Book(const string& isbn) {
        strncpy(ISBN, isbn.c_str(), 20);
    }
};

// 文件存储管理器模板



// 异步日志系统
class AsyncLogger {
private:
    queue<string> logQueue;
    mutex mtx;
    condition_variable cv;
    thread worker;
    bool running = true;

    void processLogs() {
        ofstream logfile(OPERATION_LOG, ios::app);
        while (running || !logQueue.empty()) {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [this] { return !logQueue.empty() || !running; });
            while (!logQueue.empty()) {
                logfile << logQueue.front() << endl;
                logQueue.pop();
            }
        }
    }

public:
    AsyncLogger() : worker(&AsyncLogger::processLogs, this) {}

    ~AsyncLogger() {
        running = false;
        cv.notify_all();
        worker.join();
    }

    void log(const string& message) {
        unique_lock<mutex> lock(mtx);
        logQueue.push(message);
        cv.notify_one();
    }
};

bool matchFilter(Book current, const string &filterType, const string &filterValue){
    return 0;
}

// 核心系统类



class BookstoreSystem {
private:
    vector<User> loginStack;
    Book selectedBook;
    bool hasSelected = false;

    // 文件存储实例
    MemoryRiver<User> userDB{USER_FILE};
    MemoryRiver<Book> bookDB{BOOK_FILE};

    // 哈希索引
    unordered_map<size_t, int> userIndex;
    unordered_map<size_t, int> bookIndex;

    // 异步日志
    AsyncLogger logger;

    // 辅助函数
    size_t hashString(const string& str) {
        return hash<string>{}(str);
    }

    void logOperation(const string& operation) {
        time_t now = time(nullptr);
        stringstream ss;
        ss << put_time(localtime(&now), "%F %T") << " ["
           << (loginStack.empty() ? "Guest" : loginStack.back().userID)
           << "] " << operation;
        logger.log(ss.str());
    }

    void logFinance(double amount, bool isIncome) {
        ofstream finance(FINANCE_LOG, ios::app);
        finance << (isIncome ? '+' : '-') << fixed << setprecision(2) << amount 
               << ' ' << time(nullptr) << endl;
    }

public:
    BookstoreSystem() {
        initializeSystem();
    }

    void initializeSystem() {
        // 初始化root用户
        ifstream test(USER_FILE);
        if (test.fail() || test.peek() == ifstream::traits_type::eof()) {
            User root("root", "sjtu", "Administrator", ROOT);
            int pos = userDB.write(root);
            userIndex[hashString("root")] = pos;
        }
    }

    // 用户管理
    void su(const string& userID, const string& password = "") {
        size_t hashKey = hashString(userID);
        if (userIndex.find(hashKey) == userIndex.end()) {
            throw runtime_error("User not found");
        }

        User user;
        userDB.read(user, userIndex[hashKey]);

        //cout<<user.password<<" "<<password<<endl;

        if (!password.empty() && strcmp(user.password, password.c_str()) != 0) {

            throw runtime_error("Invalid password");
        }

        if (loginStack.empty() || loginStack.back().privilege <= user.privilege) {
            loginStack.push_back(user);
            logOperation("SU " + userID);
        } else {
            throw runtime_error("Permission denied");
        }
    }

    void logout() {
        if (loginStack.empty()) throw runtime_error("No login user");
        logOperation("LOGOUT");
        loginStack.pop_back();
    }

    void registerUser(const string& userID, const string& password, const string& username) {
        if (loginStack.back().privilege < GUEST) 
            throw runtime_error("Permission denied");

        size_t hashKey = hashString(userID);
        if (userIndex.find(hashKey) != userIndex.end()) 
            throw runtime_error("Duplicate userID");

        User newUser(userID, password, username, CUSTOMER);
        int pos = userDB.write(newUser);
        userIndex[hashKey] = pos;

        logOperation("REGISTER " + userID);
    }

    void changePassword(const string& userID, 
                       const string& newPassword, 
                       const string& currentPassword = "") {
        User& current = loginStack.back();
        if (current.privilege != ROOT && currentPassword.empty())
            throw runtime_error("Need current password");
        //cout<<newPassword<<endl;
        //cout<<"FFF\n";
        size_t hashKey = hashString(userID);
        if (userIndex.find(hashKey) == userIndex.end())
            throw runtime_error("User not found");
        
        User target;
        userDB.read(target, userIndex[hashKey]);
        //cout<<target.password<<" "<<currentPassword<<endl;;
        if (current.privilege != ROOT && 
            strcmp(target.password, currentPassword.c_str()) != 0)
            throw runtime_error("Wrong password");

        strncpy(target.password, newPassword.c_str(), 30);
        target.password[30]='\0';
        userDB.update(target, userIndex[hashKey]);
        //userDB.read(target, userIndex[hashKey]);
        //cout<<newPassword<<endl;
        logOperation("PASSWD " + userID);
    }

    void createUser(const string& userID, 
                   const string& password, 
                   int privilege,
                   const string& username) {
        if (loginStack.back().privilege <= privilege)
            throw runtime_error("Insufficient privilege");

        size_t hashKey = hashString(userID);
        if (userIndex.find(hashKey) != userIndex.end())
            throw runtime_error("Duplicate userID");

        User newUser(userID, password, username, privilege);
        int pos = userDB.write(newUser);
        userIndex[hashKey] = pos;

        logOperation("USERADD " + userID);
    }

    void deleteUser(const string& userID) {
        if (loginStack.back().privilege != ROOT)
            throw runtime_error("Permission denied");

        size_t hashKey = hashString(userID);
        if (userIndex.find(hashKey) == userIndex.end())
            throw runtime_error("User not exist");

        for (const auto& u : loginStack) {
            if (strcmp(u.userID, userID.c_str()) == 0)
                throw runtime_error("User is logged in");
        }

        userIndex.erase(hashKey);
        logOperation("DELETE " + userID);
    }

    // 图书管理
    void selectBook(const string& ISBN) {
        if (loginStack.back().privilege < STAFF) 
            throw runtime_error("Permission denied");

        size_t hashKey = hashString(ISBN);
        if (bookIndex.find(hashKey) != bookIndex.end()) {
            bookDB.read(selectedBook, bookIndex[hashKey]);
        } else {
            selectedBook = Book(ISBN);
            int pos = bookDB.write(selectedBook);
            bookIndex[hashKey] = pos;
        }
        hasSelected = true;
        logOperation("SELECT " + ISBN);
    }

    void modifyBook(const vector<pair<string, string>>& modifications) {
        if (!hasSelected) throw runtime_error("No selected book");
        
        for (const auto& [field, value] : modifications) {
            if (field == "ISBN") {
                if (value == selectedBook.ISBN) 
                    throw runtime_error("Duplicate ISBN");
                size_t oldHash = hashString(selectedBook.ISBN);
                bookIndex.erase(oldHash);
                strncpy(selectedBook.ISBN, value.c_str(), 20);
                bookIndex[hashString(value)] = bookDB.write(selectedBook);
            }
            // 其他字段处理...
        }
        logOperation("MODIFY");
    }

    void purchaseBook(const string& ISBN, int quantity) {
        if (quantity <= 0) throw runtime_error("Invalid quantity");

        size_t hashKey = hashString(ISBN);
        if (bookIndex.find(hashKey) == bookIndex.end())
            throw runtime_error("Book not found");
        
        Book target;
        bookDB.read(target, bookIndex[hashKey]);

        if (target.quantity < quantity)
            throw runtime_error("Insufficient stock");

        target.quantity -= quantity;
        bookDB.update(target, bookIndex[hashKey]);

        double total = target.price * quantity;
        logFinance(total, true);

        cout << fixed << setprecision(2) << total << endl;
        logOperation("BUY " + ISBN);
    }

    void importBooks(int quantity, double totalCost) {
        if (!hasSelected) throw runtime_error("No selected book");
        if (quantity <= 0 || totalCost <= 0)
            throw runtime_error("Invalid parameters");

        selectedBook.quantity += quantity;
        int pos = bookIndex[hashString(selectedBook.ISBN)];
        bookDB.update(selectedBook, pos);

        logFinance(totalCost, false);
        logOperation("IMPORT " + to_string(quantity));
    }


    void searchBooks(const string& filterType, const string& filterValue) {
        vector<Book> results;

        int pos = 0;
        while (pos != -1) {
            Book current;
            bookDB.read(current, pos);
            
            if (matchFilter(current, filterType, filterValue)) {
                results.push_back(current);
            }
            pos = current.next;
        }

        sort(results.begin(), results.end(), [](const Book& a, const Book& b) {
            return strcmp(a.ISBN, b.ISBN) < 0;
        });

        for (const auto& book : results) {
            cout << book.ISBN << "\t" << book.name << "\t"
                 << book.author << "\t" << book.keywords << "\t"
                 << fixed << setprecision(2) << book.price << "\t"
                 << book.quantity << endl;
        }
    }

    // 日志系统
    void showFinance(int count = -1) {
        if (loginStack.back().privilege != ROOT)
            throw runtime_error("Permission denied");

        ifstream finance(FINANCE_LOG);
        string line;
        double income = 0, outcome = 0;
        int cnt = 0;

        while (getline(finance, line) && (count == -1 || cnt < count)) {
            istringstream iss(line);
            char sign;
            double amount;
            time_t timestamp;
            iss >> sign >> amount >> timestamp;
            
            if (sign == '+') income += amount;
            else outcome += amount;
            ++cnt;
        }

        cout << fixed << setprecision(2)
             << "+ " << income << " - " << outcome << endl;
    }

    void generateFinanceReport() {
        ifstream fin(FINANCE_LOG);
        string line;
        cout << "=== 财务报告 ===" << endl;
        cout << "时间\t\t类型\t金额" << endl;

        while (getline(fin, line)) {
            istringstream iss(line);
            char sign;
            double amount;
            time_t timestamp;
            iss >> sign >> amount >> timestamp;

            cout << put_time(localtime(&timestamp), "%F %T") << "\t"
                 << (sign == '+' ? "收入" : "支出") << "\t"
                 << amount << endl;
        }
    }

    void generateEmployeeReport() {
        ifstream logfile(OPERATION_LOG);
        cout << "=== 员工操作记录 ===" << endl;
        string line;
        while (getline(logfile, line)) {
            cout << line << endl;
        }
    }
};

int string_size(string s) {
    int sum = 0;
    string now;
    for (char ch : s) {
        if (ch == ' ') {
            if (now != "") sum++;
            now = "";
        }
        else now += ch;
    }
    if (now != "") sum++;
    return sum;
}
// 指令解析器

class CommandParser {
public:
    static void parse(BookstoreSystem& system, const string& cmd) {
        istringstream iss(cmd);
        int size = string_size(cmd);
        //cout<<size<<endl;
        string operation;
        iss >> operation;
        try {
            if (operation == "su") {
                string userID, password;
                if(size!=2&&size!=3){throw runtime_error("words is not right");}
                iss >> userID;
                if (iss >> password) system.su(userID, password);
                else system.su(userID);
            }
            else if (operation == "logout") system.logout();
            else if (operation == "register") {
                string userID, password, username;
                if(size!=4){throw runtime_error("words is not right");}
                iss >> userID >> password >> username;
                system.registerUser(userID, password, username);
            }
            else if (operation == "passwd") {
                string userID, currentPassword, newPassword;
                iss >> userID;
                if(size!=3&&size!=4){throw runtime_error("words is not right");}
                if (size > 3) {
                    iss >>  currentPassword >> newPassword ;
                    system.changePassword(userID, newPassword, currentPassword);
                } else {
                    iss >> newPassword;
                    //cout<<newPassword<<endl;
                    system.changePassword(userID, newPassword);
                }
            }
            else if (operation == "useradd") {
                string userID, password, privilege, username;
                if(size!=5){throw runtime_error("words is not right");}
                iss >> userID >> password >> privilege >> username;
                system.createUser(userID, password, stoi(privilege), username);
            }
            else if (operation == "delete") {
                string userID;
                if(size!=2){throw runtime_error("words is not right");}
                iss >> userID;
                system.deleteUser(userID);
            }
            else if (operation == "select") {
                string ISBN;
                if(size!=2){throw runtime_error("words is not right");}
                iss >> ISBN;
                system.selectBook(ISBN);
            }
            else if (operation == "modify") {
                vector<pair<string, string>> modifications;
                string token;
                while (iss >> token) {
                    size_t eq = token.find('=');
                    if (eq == string::npos) throw runtime_error("Invalid format");
                    string key = token.substr(0, eq);
                    string value = token.substr(eq + 1);
                    modifications.emplace_back(key, value);
                }
                system.modifyBook(modifications);
            }
            else if (operation == "buy") {
                string ISBN;
                int quantity;
                if(size!=3){throw runtime_error("words is not right");}
                iss >> ISBN >> quantity;
                system.purchaseBook(ISBN, quantity);
            }
            else if (operation == "import") {
                int quantity;
                double totalCost;
                if(size!=3){throw runtime_error("words is not right");}
                iss >> quantity >> totalCost;
                system.importBooks(quantity, totalCost);
            }
            else if (operation == "show") {
                string subcmd;
                iss >> subcmd;
                if (subcmd == "finance") {
                    string count;
                    if (iss >> count) system.showFinance(stoi(count));
                    else system.showFinance();
                }
                else {
                    string filterType, filterValue;
                    iss >> filterType >> filterValue;
                    system.searchBooks(filterType, filterValue);
                }
            }
            else if (operation == "report") {
                string subcmd;
                iss >> subcmd;
                if(size!=2){throw runtime_error("words is not right");}
                if (subcmd == "finance") system.generateFinanceReport();
                else if (subcmd == "employee") system.generateEmployeeReport();
            }
            else if (operation == "log") {
                system.generateEmployeeReport();
            }
            else if (operation == "exit" || operation == "quit") {
                exit(0);
            }
            else {
                throw runtime_error("Unknown command");
            }
        } catch (const exception& e) {
            cout<<"Invalid\n";

        }
    }
};

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