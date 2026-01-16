
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


json readJsonFile(const string& filename) {
    ifstream file(filename);
    json data;
    if (!file.is_open()) {
        cout << "отсутсвует JSON отчет. Создайте JSON файл с названием: data_{number}.json в папке с программой.";
    }
    else {
        file >> data;
        return data;
    }

}

void generateJsonFiles(int fileCount) {
    mt19937 gen(time(nullptr));
    uniform_real_distribution<> valueDist(0.0, 100.0);
    uniform_int_distribution<> errorDist(0, 20);

    for (int f = 0; f < fileCount; f++) {
        json data = json::array();

        for (int i = 0; i < 60; i++) {
            json record;
            record["sensor"] = "s1";

            ostringstream ts;
            ts << "2025-06-01T00:"
                << setw(2) << setfill('0') << i
                << ":00Z";

            if (errorDist(gen) != 0)
                record["ts"] = ts.str();

            double value = valueDist(gen);
            if (errorDist(gen) == 1)
                value = -50;

            record["value"] = value;
            data.push_back(record);
        }

        ofstream file("data_" + to_string(f) + ".json");
        file << data.dump(4);
    }
}
void generateCorrectJsonFiles(int fileCount) {
    mt19937 gen(time(nullptr));
    uniform_real_distribution<> valueDist(0.0, 100.0);

    for (int f = 0; f < fileCount; f++) {
        json data = json::array();

        for (int i = 0; i < 60; i++) {
            json record;
            record["sensor"] = "s1";

            ostringstream ts;
            ts << "2025-06-01T00:"
                << setw(2) << setfill('0') << i
                << ":00Z";


            record["ts"] = ts.str();

            double value = valueDist(gen);

            record["value"] = value;
            data.push_back(record);
        }

        ofstream file("data_" + to_string(f) + ".json");
        file << data.dump(4);
    }
}






void validateData(const json& data) {
    set<string> timestamps;

    for (const auto& item : data) {
        if (!item.contains("ts")) {
            cout << "отсутсвует ts" << endl;
            continue;
        }

        string ts = item["ts"];
        if (!timestamps.insert(ts).second)
            cout << "Дубликат ts: " << ts << endl;

        if (!item.contains("value") || !item["value"].is_number()) {
            cout << "Неверный тип value" << endl;
        }
        else {
            double value = item["value"];
            if (value < 0 || value > 100)
                cout << "Невозможное значение value: " << value << endl;
        }
    }
}

void analyzeSensorData(const json& data, const string& sensorFilter, int windowSize, bool timecheck) {
    vector<double> values;


    for (const auto& item : data) {
        if (item.contains("sensor") && item["sensor"] == sensorFilter &&
            item.contains("value") && item["value"].is_number()) {
            values.push_back(item["value"]);
        }
    }

    if (values.empty()) {
        cout << "Нет данных для сенсора " << sensorFilter << endl;
        return;
    }

    auto startMedian = chrono::high_resolution_clock::now();
    double sum = 0;
    double minVal = values[0];
    double maxVal = values[0];
    for (double v : values) {
        sum += v;
        if (v < minVal) {
            minVal = v;
        }
        if (v > maxVal) {
            maxVal = v;
        }
    }
    double avg = sum / values.size();

    vector<double> sorted = values;
    sort(sorted.begin(), sorted.end());
    double median;
    int n = sorted.size();
    if (n % 2 == 1)
        median = sorted[n / 2];
    else
        median = (sorted[n / 2 - 1] + sorted[n / 2]) / 2.0;

    auto endMedian = chrono::high_resolution_clock::now();
    auto medianDuration = chrono::duration_cast<chrono::microseconds>(endMedian - startMedian).count();

    auto startWindow = chrono::high_resolution_clock::now();

    vector<int> anomalies;
    for (int i = 0; i + windowSize <= values.size(); i++) {
        double windowSum = 0;
        for (int j = 0; j < windowSize; j++)
            windowSum += values[i + j];
        double windowAvg = windowSum / windowSize;

        double current = values[i + windowSize - 1];
        if (current > windowAvg * 1.5 || current < windowAvg * 0.5)
            anomalies.push_back(i + windowSize - 1);

    }

    auto endWindow = chrono::high_resolution_clock::now();
    auto windowDuration = chrono::duration_cast<chrono::microseconds>(endWindow - startWindow).count();


    if (timecheck == 0) {
        cout << endl << "===== ОТЧЕТ =====" << endl;
        cout << "Сенсор: " << sensorFilter << endl;
        cout << "Количество: " << values.size() << endl;
        cout << "Min: " << minVal << endl;
        cout << "Max: " << maxVal << endl;
        cout << "Среднее: " << avg << endl;
        cout << "Медиана: " << median << endl;
        cout << "Аномалий найдено: " << anomalies.size() << endl;

        if (!anomalies.empty()) {
            cout << endl << "СПИСОК АНОМАЛИЙ:" << endl;
            for (int idx : anomalies)
                cout << "Index " << idx << ", value = " << values[idx] << endl;
        }
        cout << "----------------------" << endl;
    }



    if (timecheck == 1) {
        cout << "Время вычисления медианы: " << medianDuration << " microseconds" << endl;
        cout << "Время вычисления скользящего окна: " << windowDuration << " microseconds" << endl;
    }
}


int main() {
    setlocale(LC_ALL, "");



    cout << "SENSOR_TELEMETRY" << endl;

    cout << "Создайте JSON файл с названием: data_{number}.json" << endl << "Исчисление файлов начинается с нуля" << endl;

    while (true) {
        cout << "Выберите действие" << endl;
        cout << "1) Замер мин, макс, сред значения, медианы и нахождения аномалий" << endl;
        cout << "2) Режим отладки" << endl;
        cout << "3) Выйти из программы" << endl;
        string choose;
        cin >> choose;
        if (choose == "2") {
            cout << "Выберите действие" << endl;
            cout << "1) Сгенерировать JSON файлы с ошибками" << endl;
            cout << "2) Сгенерировать JSON файлы без ошибок" << endl;
            cout << "3) Проверить время измерения" << endl;
            cin >> choose;
            if (choose == "1") {
                cout << "Введите количество файлов" << endl;
                int amount;
                cin >> amount;
                generateJsonFiles(amount);
                cout << amount << " файлов сгенерировано" << endl;
            }
            else if (choose == "2") {
                cout << "Введите количество файлов" << endl;
                int amount;
                cin >> amount;
                generateCorrectJsonFiles(amount);
                cout << amount << " файлов сгенерировано" << endl;
            }
            else if (choose == "3") {
                cout << "Введите количество файлов" << endl;
                int amount2;
                cin >> amount2;
                string sensorFilter;
                cout << "Введите название сенсора (пример: s1): ";
                cin >> sensorFilter;

                int windowSize;
                cout << "Введите размер окна: ";
                cin >> windowSize;
                auto totalStart = chrono::high_resolution_clock::now();
                for (int i = 0; i < amount2; i++) {

                    string filename = "data_" + to_string(i) + ".json";
                    ifstream test(filename);
                    if (!test.is_open()) continue;

                    json data = readJsonFile(filename);
                    cout << "Анализ " << filename << "\n";
                    analyzeSensorData(data, sensorFilter, windowSize, 1);

                }
                auto totalEnd = chrono::high_resolution_clock::now();
                auto totalDuration = chrono::duration_cast<chrono::milliseconds>(totalEnd - totalStart).count();

                cout << "\n===== ЗАМЕР ВРЕМЕНИ =====\n";
                cout << "Файлов проверено: " << amount2 << endl;
                cout << "Время анализа: " << totalDuration << " мс\n";
            }
            else {
                cout << "Неккоректный выбор(1-3!)" << endl;
            }
        }
        else if (choose == "1") {
            cout << "Проверка валидности" << endl << "Введите количество " << endl;
            int amount2;
            cin >> amount2;
            for (int i = 0; i < amount2; i++) {

                string filename = "data_" + to_string(i) + ".json";
                json data = readJsonFile(filename);

                cout << "Проверка " << filename << "\n";
                validateData(data);
                cout << "----------------\n";
            }
            string sensorFilter;
            cout << "Введите название сенсора (пример: s1): ";
            cin >> sensorFilter;

            int windowSize;
            cout << "Введите размер окна: ";
            cin >> windowSize;

            for (int i = 0; i < amount2; i++) {
                string filename = "data_" + to_string(i) + ".json";
                ifstream test(filename);
                if (!test.is_open()) continue;

                json data = readJsonFile(filename);
                cout << "Анализ " << filename << "\n";
                analyzeSensorData(data, sensorFilter, windowSize, 0);
            }

        }
        else if (choose == "3") {
            return 0;
        }
        else {
            cout << "Неккоректный выбор (1-3!)";
        }

    }

}
