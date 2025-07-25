#include <iostream>
#include <list>
#include <functional>
#include <sstream>
#include <vector>
#include <string>
#include <map>
#include <variant>
#include <chrono>
#include <thread>

using namespace std;

class Variable {
public:
    string name;
    variant<int, string> value;
};

class Led {
public:
    string name;
    int pin;
    mutable bool state;

    map<string, void (Led::*)()> methods;

    Led() : name(""), pin(0), state(false) {
        initMethods();
    }

    Led(const string& n, int p, bool s) : name(n), pin(p), state(s) {
        initMethods();
    }

    void initMethods() {
        methods["toggle"] = &Led::toggle;
        methods["on"] = &Led::on;
        methods["off"] = &Led::off;
    }

    void toggle() {
        state = !state;
        cout << name << ": state = " << (state ? "ON" : "OFF") << endl;
    }

    void on() {
        state = true;
        cout << name << " turned ON." << endl;
    }

    void off() {
        state = false;
        cout << name << " turned OFF." << endl;
    }
};

string resolveVariable(const string& token, map<string, variant<int, string, bool, float>>& variables) {
    if (token.empty()) return "";

    if (token[0] == '%') {
        string key = token.substr(1);
        if (variables.count(key) == 0) {
            cout << "Variable '%" << key << "' does not exist!" << endl;
            return "";
        }

        auto& val = variables[key];
        if (holds_alternative<int>(val)) return to_string(get<int>(val));
        else if (holds_alternative<float>(val)) return to_string(get<float>(val));
        else if (holds_alternative<bool>(val)) return get<bool>(val) ? "true" : "false";
        else if (holds_alternative<string>(val)) return get<string>(val);
    }

    return token;
}

void printLedStatesBeforeAndAfter(Led& led, const string& action, void (Led::*method)()) {
    bool before = led.state;
    cout << "[Before] " << led.name << " = " << (before ? "ON" : "OFF") << endl;

    (led.*method)();

    bool after = led.state;
    cout << "[After]  " << led.name << " = " << (after ? "ON" : "OFF") << endl;
}

int main() {
    map<string, Led> ledSystem;
    map<string, variant<int, string, bool, float>> variables;

    ledSystem["red"] = Led("RedLED", 0, false);
    ledSystem["blue"] = Led("BlueLED", 1, false);
    ledSystem["green"] = Led("GreenLED", 2, false);

    while (true) {
        string input;
        vector<string> words;

        cout << "\nCommand: ";
        getline(cin, input);

        istringstream iss(input);
        string word;

        while (iss >> word) {
            word = resolveVariable(word, variables);
            if (!word.empty()) words.push_back(word);
        }

        if (words.empty()) continue;

        // --- LED command ---
        if (words.size() >= 3 && words[0] == "led") {
            string action = words[1];
            string color = words[2];

            if (ledSystem.count(color) == 0) {
                cout << "LED '" << color << "' not found." << endl;
            } else if (ledSystem[color].methods.count(action) == 0) {
                cout << "Method '" << action << "' not available." << endl;
            } else {
                printLedStatesBeforeAndAfter(ledSystem[color], action, ledSystem[color].methods[action]);
            }
        }

        // --- set command ---
        else if (words.size() >= 3 && words[0] == "set") {
            variant<int, string, bool, float> value;

            if (words[2] == "true" || words[2] == "false") {
                value = (words[2] == "true");
            } else {
                try {
                    size_t idx;
                    int i = stoi(words[2], &idx);
                    if (idx == words[2].length()) value = i;
                    else value = stof(words[2]);
                } catch (...) {
                    value = words[2];
                }
            }

            auto result = variables.insert({words[1], value});
            if (result.second) {
                cout << "Inserted: " << words[1] << " = " << words[2] << endl;
            } else {
                cout << "Key '" << words[1] << "' already exists." << endl;
            }

            auto& val = variables[words[1]];
            cout << "Stored type: ";
            if (holds_alternative<int>(val)) cout << "int" << endl;
            else if (holds_alternative<float>(val)) cout << "float" << endl;
            else if (holds_alternative<bool>(val)) cout << "bool" << endl;
            else if (holds_alternative<string>(val)) cout << "string" << endl;
        }

        // --- echo command ---
        else if (words.size() >= 2 && words[0] == "echo") {
            for (size_t i = 1; i < words.size(); ++i)
                cout << words[i] << ' ';
            cout << endl;
        }

        // --- sleep command ---
        else if (words.size() >= 3 && words[0] == "sleep") {
            string unit = words[1];
            int amount = 0;

            try {
                amount = stoi(words[2]);
            } catch (...) {
                cout << "Invalid amount for sleep." << endl;
                continue;
            }

            cout << "Sleeping for " << amount << " " << unit << "..." << endl;

            if (unit == "seconds")
                this_thread::sleep_for(chrono::seconds(amount));
            else if (unit == "milliseconds")
                this_thread::sleep_for(chrono::milliseconds(amount));
            else if (unit == "minutes")
                this_thread::sleep_for(chrono::minutes(amount));
            else
                cout << "Unsupported time unit: " << unit << endl;
        }

        else {
            cout << "Unrecognized command." << endl;
        }
    }

    return 0;
}
