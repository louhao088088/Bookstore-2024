#pragma once
#include "MemoryRiver.hpp"
#include "AsyncLogger.hpp"
#include <bits/stdc++.h>
using namespace std;
// 常量定义

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

vector<string> string_split(string s) {
    vector<string>tmp;
    string now;
    for (char ch : s) {
        if (ch == ' ') {
            if (now != "") tmp.push_back(now);
            now = "";
        }
        else now += ch;
    }
    if (now != "") tmp.push_back(now);
    return tmp;
}

double convert_double(string s){
    double sum = 0;
    int F = 0;
     for (char ch : s) {
        if(ch == '.'){F=1;continue;}
        if(!F) sum = sum * 10 + ch-'0';
        else if(F==1) sum = sum + 1.0* (ch-'0') / 10, F++;
        else sum = sum + 1.0* (ch-'0') / 100, F++;
    }
    return sum;
}

bool matchFilter(Book current, const string &filterType, const string &filterValue){
    if (filterType == "ISBN") {
        if(strcmp(current.ISBN,filterValue.c_str()))return 1;
        else return 0;
    }
    else if (filterType == "name"){
        if(strcmp(current.name,filterValue.c_str()))return 1;
        else return 0;
    }
    else if (filterType == "author"){
        if(strcmp(current.author,filterValue.c_str()))return 1;
        else return 0;
    }
    else if (filterType == "price"){
        double price = convert_double(filterValue);
        if(fabs(price-current.price)<1e-5)return 1;
        return 0;
    }
    else if (filterType == "keyword"){
        vector<string>Key = string_split(filterValue);
        if(Key.size()>1){
            throw runtime_error("More Key words");
        }
        vector<string>Keys = string_split(current.keywords);
        for(int i=0; i<Keys.size(); i++){
            if(filterValue==Keys[i])return 1;
        }
        return 0;
    }
            
    return 0;
}


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
            cout<<field<<endl;
            if (field == "ISBN") {
                if (value == selectedBook.ISBN) 
                    throw runtime_error("Duplicate ISBN");
                size_t oldHash = hashString(selectedBook.ISBN);
                bookIndex.erase(oldHash);
                strncpy(selectedBook.ISBN, value.c_str(), 20);
                bookIndex[hashString(selectedBook.ISBN)] = bookDB.write(selectedBook);
            }
            
            else if (field == "name"){
                strncpy(selectedBook.name, value.c_str(), 60);
                bookIndex[hashString(selectedBook.ISBN)] = bookDB.write(selectedBook);
            }
            else if (field == "author"){
                strncpy(selectedBook.author, value.c_str(), 60);
                bookIndex[hashString(selectedBook.ISBN)] = bookDB.write(selectedBook);
            }
            else if (field == "keyword"){
                vector<string>Key = string_split(value);
                for(int i = 0; i < Key.size(); i++)
                    for(int j = i + 1; j < Key.size(); j++){
                        if(Key[i] == Key[j]){
                            throw runtime_error("Duplicate keyword");
                        }
                    }
                strncpy(selectedBook.keywords, value.c_str(), 60);
                bookIndex[hashString(selectedBook.ISBN)] = bookDB.write(selectedBook);
            }
            else if (field == "price"){
                selectedBook.price = convert_double(value);
                bookIndex[hashString(selectedBook.ISBN)] = bookDB.write(selectedBook);
            }
        }
        cout << selectedBook.ISBN << "\t" << selectedBook.name << "\t"
                 << selectedBook.author << "\t" << selectedBook.keywords << "\t"
                 << fixed << setprecision(2) << selectedBook.price << "\t"
                 << selectedBook.quantity << endl;
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
        if(results.size()==0)cout<<endl;
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