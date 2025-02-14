#pragma once
#include "MemoryRiver.hpp"
#include "Database.hpp"
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
    int flag = 0;

    Book() = default;
    explicit Book(const string& isbn) {
        strncpy(ISBN, isbn.c_str(), 20);
    }
    void show(){
        cout << ISBN << "\t" << name << "\t"
            << author << "\t" << keywords << "\t"
            << fixed << setprecision(2) << price << "\t"
            << quantity << endl;
    }
};

vector<string> string_split(string s) {
    vector<string>tmp;
    string now;
    for (char ch : s) {
        if (ch == '|') {
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
        //puts("ISBN");
        if(strcmp(current.ISBN,filterValue.c_str())==0)return 1;
        else return 0;
    }
    else if (filterType == "name"){
        if(strcmp(current.name,filterValue.c_str())==0)return 1;
        else return 0;
    }
    else if (filterType == "author"){
        if(strcmp(current.author,filterValue.c_str())==0)return 1;
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
            //cout<<Keys[i]<<endl;
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
    vector<Book>selected;

    // 文件存储实例


    Database<User> userDB;


    Database<Book> bookDB;

    // 日志
    AsyncLogger logger;

    // Hash
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
    BookstoreSystem(): userDB(USER_FILE),bookDB(BOOK_FILE){

        initializeSystem();
    }

    void initializeSystem() {
        hasSelected = false;
        if(!userDB.check("root")){
            User root("root", "sjtu", "Administrator", ROOT);
            userDB.insert("root",root);
        }
        
        
    }

    // 用户管理
    void su(const string& userID, const string& password = "") {
        if (!userDB.check(userID.c_str()) ) {
            throw runtime_error("User not found");
        }

        User user=userDB.find(userID.c_str());
        //cout<<user.password<<" "<<password<<endl;

        if (!password.empty() && strcmp(user.password, password.c_str()) != 0) {

            throw runtime_error("Invalid password");
        }
        if (!password.empty() || (!loginStack.empty()
            &&loginStack.back().privilege > user.privilege)) 
        {
            loginStack.push_back(user);
            Book emptybook;emptybook.flag=1;
            selected.push_back(emptybook);
            hasSelected=0;
            logOperation("SU " + userID);
        } else {
            throw runtime_error("Permission denied");
        }
    }

    void logout() {
        if (loginStack.empty()) throw runtime_error("No login user");
        logOperation("LOGOUT");
        loginStack.pop_back();
        selected.pop_back();
        
        if (loginStack.empty()) hasSelected = false;
        else {
            selectedBook = selected.back();
            if(selectedBook.flag) hasSelected = false;
            else hasSelected = true;
        }
        
    }


    void registerUser(const string& userID, const string& password, const string& username) {

        if (userDB.check(userID.c_str()) ) 
            throw runtime_error("Duplicate userID");

        User newUser(userID, password, username, CUSTOMER);
        userDB.insert(userID.c_str(),newUser);

        logOperation("REGISTER " + userID);
    }

    void changePassword(const string& userID, 
                       const string& newPassword, 
                       const string& currentPassword = "") {
        User& current = loginStack.back();
        if(loginStack.empty())
            throw runtime_error("Permission denied");

        if (current.privilege != ROOT && currentPassword.empty())
            throw runtime_error("Need current password");
        //cout<<newPassword<<endl;
        //cout<<"FFF\n";
        if (!userDB.check(userID.c_str()))
            throw runtime_error("User not found");
        
        User target=userDB.find(userID.c_str());
        //cout<<target.password<<" "<<currentPassword<<endl;;
        if (current.privilege != ROOT && 
            strcmp(target.password, currentPassword.c_str()) != 0)
            throw runtime_error("Wrong password");

            
        userDB.erase(userID.c_str());
        strncpy(target.password, newPassword.c_str(), 30);
        userDB.insert(userID.c_str(),target);
    
        logOperation("PASSWD " + userID);
    }

    void createUser(const string& userID, 
                   const string& password, 
                   int privilege,
                   const string& username) {
        if (loginStack.empty()||loginStack.back().privilege <= privilege)
            throw runtime_error("Insufficient privilege");

        if (userDB.check(userID.c_str()))
            throw runtime_error("Duplicate userID");

        User newUser(userID, password, username, privilege);
        userDB.insert(userID.c_str(),newUser);

        logOperation("USERADD " + userID);
    }

    void deleteUser(const string& userID) {
        if (loginStack.empty()||loginStack.back().privilege != ROOT)
            throw runtime_error("Permission denied");

        if (!userDB.check(userID.c_str()) )
            throw runtime_error("User not exist");

        for (const auto& u : loginStack) {
            if (strcmp(u.userID, userID.c_str()) == 0)
                throw runtime_error("User is logged in");
        }
        userDB.erase(userID.c_str());
        logOperation("DELETE " + userID);
    }

    // 图书管理




    void selectBook(const string& ISBN) {
        if (loginStack.empty()||loginStack.back().privilege < STAFF) 
            throw runtime_error("Permission denied");
        Book newbook;
        
        if(!bookDB.check(ISBN.c_str())){
            strncpy(newbook.ISBN, ISBN.c_str(), 20);
            bookDB.insert(ISBN.c_str(),newbook);
            selectedBook = newbook;
        }
        else selectedBook = bookDB.find(ISBN.c_str());

       // selectedBook.show();
        hasSelected = true;
        selected.pop_back();
        selected.push_back(selectedBook);
        logOperation("SELECT " + ISBN);
    }

    void modifyBook(const vector<pair<string, string>>& modifications) {
        if (!hasSelected) throw runtime_error("No selected book");
        string tmp=selectedBook.ISBN;
        //vector<Book> all=bookDB.getall();

        for (const auto& [field, value] : modifications) {
            //cout<<field<<endl;
            if (field == "ISBN") {
                if(bookDB.check(value.c_str()))
                    throw runtime_error("Duplicate ISBN");
                
                strncpy(selectedBook.ISBN, value.c_str(), 20);
            }
            
            else if (field == "name"){
                strncpy(selectedBook.name, value.c_str(), 60);
            }
            else if (field == "author"){
                strncpy(selectedBook.author, value.c_str(), 60);
                
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
            }
            else if (field == "price"){
                selectedBook.price = convert_double(value);
            }
            
        }
        
        bookDB.erase(tmp.c_str());
        bookDB.insert(selectedBook.ISBN,selectedBook);
        //selectedBook.show();
        logOperation("MODIFY");
    }

    void purchaseBook(const string& ISBN, int quantity) {

        if (loginStack.empty()) 
            throw runtime_error("Permission denied");

        if (quantity <= 0) throw runtime_error("Invalid quantity");

        if (!bookDB.check(ISBN.c_str()))
            throw runtime_error("Book not found");
        
        Book target=bookDB.find(ISBN.c_str());
        //target.show();


        if (target.quantity < quantity)
            throw runtime_error("Insufficient stock");

        target.quantity -= quantity;
        bookDB.erase(ISBN.c_str());

        double total = target.price * quantity;
        bookDB.insert(ISBN.c_str(),target);

        logFinance(total, true);

        cout << fixed << setprecision(2) << total << endl;
        logOperation("BUY " + ISBN);
    }

    void importBooks(int quantity, double totalCost) {
        if (!hasSelected) throw runtime_error("No selected book");

        if (loginStack.empty()||loginStack.back().privilege<3) 
            throw runtime_error("Permission denied");

        if (quantity <= 0 || totalCost <= 0)
            throw runtime_error("Invalid parameters");
        bookDB.erase(selectedBook.ISBN);
        selectedBook.quantity += quantity;
        bookDB.insert(selectedBook.ISBN,selectedBook);
        
        logFinance(totalCost, false);
        logOperation("IMPORT " + to_string(quantity));
    }


    void searchBooks(const string& filterType, const string& filterValue) {
        if (loginStack.empty()) 
            throw runtime_error("Permission denied");

        vector<Book> results,all=bookDB.getall();

        for(int i=0;i<all.size();i++){
            //all[i].show();
            if (matchFilter(all[i], filterType, filterValue)) {
                results.push_back(all[i]);
                //all[i].show();
            }
        }

        sort(results.begin(), results.end(), [](const Book& a, const Book& b) {
            return strcmp(a.ISBN, b.ISBN) < 0;
        });
        if(results.size()==0)cout<<endl;
        for (int i=0;i<results.size();i++) {
            results[i].show();
        }
    }

    void showall() {

        if (loginStack.empty()) 
            throw runtime_error("Permission denied");

        vector<Book> results=bookDB.getall();


        sort(results.begin(), results.end(), [](const Book& a, const Book& b) {
            return strcmp(a.ISBN, b.ISBN) < 0;
        });
        if(results.size()==0)cout<<endl;
        for (int i=0;i<results.size();i++) {
            results[i].show();
        }
    }


    void showFinance(int count = -1) {
        if (loginStack.back().privilege != ROOT)
            throw runtime_error("Permission denied");

        ifstream finance(FINANCE_LOG);
        string line;
        double income = 0, outcome = 0;
        int cnt = 0;
        vector <double> tmp;
        while (getline(finance, line) ) {
            istringstream iss(line);
            char sign;
            double amount;
            time_t timestamp;
            iss >> sign >> amount >> timestamp;
            
            if (sign == '+') tmp.push_back(amount);
            else tmp.push_back(-amount);
            ++cnt;
        }

        if(cnt < count) 
            throw runtime_error("Too much counts");
        
        cnt = cnt - count;
        if(count == -1) cnt = 0;
        for(int i=tmp.size()-1;i>=cnt;i--)
            if(tmp[i] < 0)outcome -= tmp[i];
            else income += tmp[i];
        cout << fixed << setprecision(2)
             << "+ " << income << " - " << outcome << endl;
    }


    // 日志系统
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