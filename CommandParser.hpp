#pragma once
#include "user.hpp"
#include "book.hpp"
#include <bits/stdc++.h>
using namespace std;
class CommandParser {
public:
    static void parse(BookstoreSystem& system, const string& cmd) {
        istringstream iss(cmd);
        string operation;
        iss >> operation;

        try {
            if (operation == "su") {
                string userID, password;
                iss >> userID;
                if (iss >> password) system.su(userID, password);
                else system.su(userID);
            }
            else if (operation == "logout") system.logout();
            else if (operation == "register") {
                string userID, password, username;
                iss >> userID >> password >> username;
                system.registerUser(userID, password, username);
            }
            else if (operation == "passwd") {
                string userID, currentPassword, newPassword;
                iss >> userID;
                if (iss >> currentPassword >> newPassword) {
                    system.changePassword(userID, newPassword, currentPassword);
                } else {
                    iss >> newPassword;
                    system.changePassword(userID, newPassword);
                }
            }
            else if (operation == "useradd") {
                string userID, password, privilege, username;
                iss >> userID >> password >> privilege >> username;
                system.createUser(userID, password, stoi(privilege), username);
            }
            else if (operation == "delete") {
                string userID;
                iss >> userID;
                system.deleteUser(userID);
            }
            else if (operation == "select") {
                string ISBN;
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
                iss >> ISBN >> quantity;
                system.purchaseBook(ISBN, quantity);
            }
            else if (operation == "import") {
                int quantity;
                double totalCost;
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
            cout<<"Invalid\n"<<" "<<e.what()<<endl;

        }
    }
};