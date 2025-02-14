#pragma once
#include "User.hpp"
#include "AsyncLogger.hpp"

#include <bits/stdc++.h>
using namespace std;
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
                    string key = token.substr(1, eq-1);
                    string value;
                    if(key=="ISBN"||key=="price"){
                        value=token.substr(eq + 1);
                    }
                    else {
                        string tmp;
                        tmp=token.substr(eq + 2);
                        size_t endpos = tmp.find('"');
                        value=tmp.substr(0,endpos);
                    }
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
                if(size==1){
                    system.showall();
                    return;
                }
                iss >> subcmd;
                if (subcmd == "finance") {
                    string count;
                    if (iss >> count) system.showFinance(stoi(count));
                    else system.showFinance();
                }
                else {
                    size_t eq = subcmd.find('=');
                    if (eq == string::npos) throw runtime_error("Invalid format");
                    string filterType = subcmd.substr(1, eq-1);
                    string filterValue;
                    if(filterType=="ISBN"||filterType=="price"){
                        filterValue=subcmd.substr(eq + 1);
                    }
                    else {
                        string tmp;
                        tmp=subcmd.substr(eq + 2);
                        size_t endpos = tmp.find('"');
                        filterValue=tmp.substr(0,endpos);
                    }
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