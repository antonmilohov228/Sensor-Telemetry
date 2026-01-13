#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <fstream>
#include "json.hpp" // Убедитесь, что файл подключен

using namespace std;
using json = nlohmann::json;

void CheckSensorLogs(const json& logs) {
    set<string> history; 

    for (const auto& record : logs) {

        if (record.contains("value") && record["value"].is_number()) {
            double val = record["value"];
            if (val > 100.0 || val < 0.0) {
                cout << "[Ошибка] Выход за границы (0-100): " << val << endl;
            }
        }
        else {
            cout << "[Ошибка] Поле 'value' отсутствует или не является числом" << endl;
        }

        if (!record.contains("ts")) {
            cout << "[Ошибка] В записи нет поля 'ts'" << endl;
            continue; // Пропускаем работу с сетом, если нет ключа
        }

        string current_ts = record["ts"];

        if (history.count(current_ts)) {
            cout << "[Дубликат] Найден повтор времени: " << current_ts << endl;
        }
        else {
            history.insert(current_ts);
        }
    }
}

// Тест с корректными данными
void run_positive_test() {
    cout << "--- Тест 1: Нормальные данные ---" << endl;
    json valid_batch = {
        {{"sensor", "s1"}, {"value", 10.5}, {"ts", "2025-06-01T00:00:00Z"}}, 
        {{"sensor", "s1"}, {"value", 12.0}, {"ts", "2025-06-01T00:01:00Z"}}
    };
    CheckSensorLogs(valid_batch);
    cout << endl;
}

// Тест с ошибками
void run_negative_test() {
    cout << "--- Тест 2: Данные с ошибками ---" << endl;
    json bad_batch = {
        {{"sensor", "s1"}, {"ts", "2025-06-01T00:00:00Z"}, {"value", -5}},   // Отрицательное
        {{"sensor", "s1"}, {"ts", "2025-06-01T00:00:00Z"}, {"value", 200}},  // Дубликат времени + > 100
        {{"sensor", "s1"}}                                                   // Пустая запись без value и ts
    };
    CheckSensorLogs(bad_batch);
    cout << endl;
}

int main() {
    setlocale(LC_ALL, "Russian"); // Или пустая строка "" для системной локали

    run_positive_test();
    run_negative_test();

    return 0;
}