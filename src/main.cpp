#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <sstream>
#include <iomanip>
#include <random>
#include <algorithm>
#include <vector>
#include <chrono>

#include "json.hpp"

using namespace std;
using json = nlohmann::json;


// Функция чтения файла
json load_json(const string& fname)
{
    ifstream f(fname);
    if (!f.is_open()) {
        cerr << "[Ошибка] Не удалось открыть файл: " << fname << endl;
        return json{}; // Возвращаем пустой объект
    }
    json j;
    f >> j;
    return j;
}

// Генератор файлов 
void create_files(int count, bool with_errors)
{
    mt19937 gen(time(nullptr));
    uniform_real_distribution<> dist_val(0.0, 100.0);
    uniform_int_distribution<> dist_err(0, 20); // Шанс ошибки

    for (int k = 0; k < count; k++) {
        json arr = json::array();

        for (int i = 0; i < 60; i++) {
            json obj;
            obj["sensor"] = "s1";

            // Формируем дату
            ostringstream ss;
            ss << "2025-06-01T00:" << setw(2) << setfill('0') << i << ":00Z";
            
            // Если нужны ошибки, иногда портим дату
            if (with_errors && dist_err(gen) != 0) {
                 obj["ts"] = ss.str();
            } else if (!with_errors) {
                 obj["ts"] = ss.str();
            }
            // Иначе ts просто не запишется (пропуск поля)

            double val = dist_val(gen);
            
            // Если нужны ошибки, иногда делаем некорректное значение
            if (with_errors && dist_err(gen) == 1) val = -50.0;
            
            obj["value"] = val;
            arr.push_back(obj);
        }

        string fname = "data_" + to_string(k) + ".json";
        ofstream out(fname);
        out << arr.dump(4);
    }
}

// Проверка данных на корректность
void check_validity(const json& j_data)
{
    set<string> unique_ts;

    for (const auto& el : j_data) {
        // 1. Проверка поля ts
        if (!el.contains("ts")) {
            cout << "Warning: пропущено поле 'ts'" << endl;
            continue;
        }

        string t = el["ts"];
        // Проверка на дубли
        if (unique_ts.find(t) != unique_ts.end()) {
            cout << "Warning: дубликат времени -> " << t << endl;
        }
        unique_ts.insert(t);

        // 2. Проверка значения
        if (!el.contains("value") || !el["value"].is_number()) {
            cout << "Warning: некорректный тип 'value'" << endl;
        }
        else {
            double v = el["value"];
            if (v < 0 || v > 100) {
                cout << "Warning: значение вне диапазона (0-100) -> " << v << endl;
            }
        }
    }
}

// Основная логика анализа
void process_sensor(const json& j_data, const string& target_sensor, int win_size, bool show_timing)
{
    vector<double> vals;

    // Фильтруем данные только нужного сенсора
    for (const auto& el : j_data) {
        if (el.value("sensor", "") == target_sensor && el.contains("value")) {
            if (el["value"].is_number()) {
                vals.push_back(el["value"]);
            }
        }
    }

    if (vals.empty()) {
        // cout << "Данные для сенсора " << target_sensor << " не найдены." << endl;
        return;
    }

    // --- Блок 1: Статистика (Min, Max, Avg, Median) ---
    auto t1 = chrono::high_resolution_clock::now();

    double sum = 0;
    double min_v = vals[0];
    double max_v = vals[0];

    for (double x : vals) {
        sum += x;
        if (x < min_v) min_v = x;
        if (x > max_v) max_v = x;
    }
    double avg = sum / vals.size();

    // Медиана 
    vector<double> temp = vals;
    sort(temp.begin(), temp.end());
    
    double median = 0;
    size_t sz = temp.size();
    if (sz % 2 == 1) 
        median = temp[sz / 2];
    else 
        median = (temp[sz / 2 - 1] + temp[sz / 2]) / 2.0;

    auto t2 = chrono::high_resolution_clock::now();
    auto dur_stat = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();

    // --- Блок 2: Скользящее окно и аномалии ---
    auto t3 = chrono::high_resolution_clock::now();
    
    vector<int> anomaly_indices;
    // Проходим окном
    for (size_t i = 0; i + win_size <= vals.size(); ++i) {
        double w_sum = 0;
        for (int k = 0; k < win_size; ++k) {
            w_sum += vals[i + k];
        }
        double w_avg = w_sum / win_size;
        
        // Текущий элемент (последний в окне)
        double cur = vals[i + win_size - 1];

        // Критерий аномалии: отклонение более чем на 50% от среднего по окну
        if (cur > w_avg * 1.5 || cur < w_avg * 0.5) {
            anomaly_indices.push_back(i + win_size - 1);
        }
    }

    auto t4 = chrono::high_resolution_clock::now();
    auto dur_win = chrono::duration_cast<chrono::microseconds>(t4 - t3).count();

    // Вывод результатов (если это не просто замер времени)
    if (!show_timing) {
        cout << "\n=== Результаты для: " << target_sensor << " ===" << endl;
        cout << "Всего записей: " << vals.size() << endl;
        cout << "Min: " << min_v << " | Max: " << max_v << endl;
        cout << "Avg: " << avg   << " | Median: " << median << endl;
        cout << "Найдено аномалий: " << anomaly_indices.size() << endl;
        
        if (!anomaly_indices.empty()) {
            cout << "[Список первых 5 аномалий]:" << endl;
            int count = 0;
            for (int idx : anomaly_indices) {
                cout << "  idx: " << idx << " -> val: " << vals[idx] << endl;
                if (++count >= 5) { cout << "  ..." << endl; break; }
            }
        }
    } 
    else {
        // Вывод для бенчмарка
       // cout << "Median calc: " << dur_stat << " mcs, Window calc: " << dur_win << " mcs" << endl;
    }
}

int main()
{
setlocale(LC_ALL, "Russian");
    setConsoleColor(); 
    
    printHeader();
    showLoadingBar("Инициализация системного ядра");
    showLoadingBar("Загрузка парсера nlohmann/json");
    showLoadingBar("Проверка калибровки сенсоров");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    cout << "=== SensorTelemetry v1.0 ===" << endl;
    cout << "Студент: Милохов А.А." << endl;
    cout << "============================" << endl;

    while (true) {
        printHeader();
        cout << "\nМеню:" << endl;
        cout << "1. Анализ данных (чтение -> валидация -> статистика)" << endl;
        cout << "2. Генерация/Тесты (Debug Mode)" << endl;
        cout << "3. Выход" << endl;
        cout << "> ";
        
        string choice;
        cin >> choice;

        if (choice == "1") {
            int cnt;
            cout << "Сколько файлов обработать? ";
            cin >> cnt;
            
            string sens;
            cout << "Имя сенсора (например, s1): ";
            cin >> sens;
            
            int w_size;
            cout << "Размер окна: ";
            cin >> w_size;

            for (int i = 0; i < cnt; i++) {
                string fn = "data_" + to_string(i) + ".json";
                cout << "Загрузка " << fn << "... ";
                json j = load_json(fn);
                
                if (j.empty()) continue; // Пропуск если файл битый

                check_validity(j);
                process_sensor(j, sens, w_size, false);
            }
        }
        else if (choice == "2") {
            cout << "\n--- Debug Menu ---" << endl;
            cout << "1. Создать файлы с ошибками" << endl;
            cout << "2. Создать чистые файлы" << endl;
            cout << "3. Бенчмарк (замер скорости)" << endl;
            cout << "> ";
            
            string sub_c;
            cin >> sub_c;

            if (sub_c == "1" || sub_c == "2") {
                int n;
                cout << "Количество файлов: ";
                cin >> n;
                bool err = (sub_c == "1");
                create_files(n, err);
                cout << "Готово! Создано " << n << " файлов." << endl;
            }
            else if (sub_c == "3") {
                int n, w;
                cout << "Файлов: "; cin >> n;
                cout << "Окно: "; cin >> w;
                
                auto total_start = chrono::high_resolution_clock::now();
                
                for (int i = 0; i < n; i++) {
                    string fn = "data_" + to_string(i) + ".json";
                    json j = load_json(fn);
                    if (!j.empty()) {
                        process_sensor(j, "s1", w, true); // true = режим замера
                    }
                }
                
                auto total_end = chrono::high_resolution_clock::now();
                auto ms = chrono::duration_cast<chrono::milliseconds>(total_end - total_start).count();
                
                cout << "\n[BENCHMARK RESULT]" << endl;
                cout << "Файлов: " << n << endl;
                cout << "Общее время: " << ms << " ms" << endl;
            }
        }
        else if (choice == "3") {
            break;
        }
        else {
            cout << "Неверный ввод, повторите." << endl;
        }
    }
    return 0;
}
