#include <iostream> // для вводу-виводу
#include <string> 
#include <cmath>
#include <iomanip> // для форматування виводу
#include <limits> // для numeric_limits
#include <chrono> // для роботи з часом
#include <random> // для генерації випадкових чисел
#ifdef _WIN32 
#define NOMINMAX // для уникнення конфлікту з min/max
#include <windows.h>
#endif
using namespace std; // для зручності використання стандартної бібліоте
using Clock = chrono::steady_clock; // для роботи з часом

const int N = 4;
const int MAX_ORDERS = 10;
const int STREET_COUNT = 10;
const double BASE_PRICE = 50;
const double PRICE_PER_KM = 8;
const double PRICE_PER_KG = 5;
const double AVG_SPEED = 25;
const int SECONDS_PER_MINUTE = 20;

struct Street {
    string name; double x, y;
};
Street streets[STREET_COUNT] = {
    { "Хрещатик",0,0 },{ "Саксаганського",2,3 },
    { "Володимирська",1,2 },{ "Антоновича",3,4 },
    { "Велика Васильківська",2,1 },{ "Жилянська",4,2 },
    { "Шевченка",-2,3 },{ "Богдана Хмельницького",-1,1 },
    { "Лесі Українки",5,3 },{ "Перемоги",-4,2 }
};
string courierName[N] = { "Олег", "Ірина", "Андрій", "Тарас" };
bool courierOnline[N] = { true, true, false, true };
double courierX[N] = { 2.0, -4.0, 0.5, 6.0 };
double courierY[N] = { 3.0, 1.5, 0.5, -2.0 };

struct Order {
    string trackingNumber, clientName, phone;
    string fromAddress, toAddress;
    double fromX = 0, fromY = 0, toX = 0, toY = 0;
    double weight = 0, cost = 0;
    string status;
    int courier = -1;
    int etaMinutes = 0;
    Clock::time_point deliveryStart{};
};
Order orders[MAX_ORDERS];
int orderCount = 0;

double distance(double x1, double y1, double x2, double y2) {
    return hypot(x1 - x2, y1 - y2);
}
int arrivalTime(double km) {
    return static_cast<int>(ceil(km / AVG_SPEED * 60));
}
int readInt(const string& prompt) {
    int value;
    while (true) {
        cout << prompt;
        if (cin >> value) return value;
        if (cin.eof()) return 0;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Введіть ціле число.\n";
    }
}
void showStreets() {
    cout << "\nДоступні вулиці Києва:\n";
    for (int i = 0;i < STREET_COUNT;i++)
        cout << i + 1 << ". " << streets[i].name << '\n';
}
void inputAddress(string& address, double& x, double& y) {
    showStreets();
    int choice;
    do {
        choice = readInt("Оберіть номер вулиці: ");
    } while (choice<1 || choice>STREET_COUNT);
    string building;
    cout << "Номер будинку: "; cin >> building;
    const Street& street = streets[choice - 1];
    address = "вул. " + street.name + ", " + building;
    x = street.x; y = street.y;
}
void inputAddresses(Order& order) {
    cout << "\nАДРЕСА ВІДПРАВЛЕННЯ\n";
    inputAddress(order.fromAddress, order.fromX, order.fromY);
    cout << "\nАДРЕСА ОТРИМАННЯ\n";
    inputAddress(order.toAddress, order.toX, order.toY);
}
bool validPhone(const string& phone) {
    if (phone.size() != 13 || phone.compare(0, 4, "+380") != 0)
        return false;
    for (size_t i = 4; i < phone.size(); ++i)
        if (phone[i] < '0' || phone[i] > '9') return false;
    return true;
}

bool validClientName(const string& name) {
    if (name.empty()) return false;
    bool hasLetter = false;
    bool previousSpace = true;
    for (size_t i = 0; i < name.size();) {
        unsigned char c = static_cast<unsigned char>(name[i]);
        unsigned int cp = 0;
        size_t count = 0;
        if (c < 0x80) {
            cp = c; count = 1;
        }
        else if ((c & 0xE0) == 0xC0) {
            cp = c & 0x1F; count = 2;
        }
        else if ((c & 0xF0) == 0xE0) {
            cp = c & 0x0F; count = 3;
        }
        else if ((c & 0xF8) == 0xF0) {
            cp = c & 0x07; count = 4;
        }
        else return false;
        if (i + count > name.size()) return false;
        for (size_t j = 1; j < count; ++j) {
            unsigned char next = static_cast<unsigned char>(name[i + j]);
            if ((next & 0xC0) != 0x80) return false;
            cp = (cp << 6) | (next & 0x3F);
        }
        if (cp == ' ') {
            if (previousSpace) return false;
            previousSpace = true;
        }
        else if ((cp >= 'A' && cp <= 'Z') ||
            (cp >= 'a' && cp <= 'z') ||
            (cp >= 0x0400 && cp <= 0x052F)) {
            hasLetter = true;
            previousSpace = false;
        }
        else return false;
        i += count;
    }
    return hasLetter && !previousSpace;
}

void inputOrder(Order& order) {
    while (true) {
        cout << "Ім'я клієнта: ";
        if (!getline(cin >> ws, order.clientName)) return;
        if (validClientName(order.clientName)) break;
        cout << "Помилка! Ім'я може містити лише літери (українські або латинські) та пробіли.\n";
    }
    while (true) {
        cout << "Телефон (+380XXXXXXXXX): ";
        if (!(cin >> order.phone)) return;
        if (validPhone(order.phone)) break;
        cout << "Помилка! Введіть +380 та 9 цифр без пробілів і літер.\n";
    }
    inputAddresses(order);
    do {
        cout << "Вага посилки (0-30 кг): ";
        if (cin >> order.weight && order.weight > 0 && order.weight <= 30) break;
        if (cin.eof()) return;
        cin.clear(); cin.ignore(numeric_limits<streamsize>::max(), '\n');
        cout << "Некоректна вага.\n";
    } while (true);
}
bool inDeliveryZone(const Order& o) {
    return distance(o.fromX, o.fromY, 0, 0) <= 25 &&
        distance(o.toX, o.toY, 0, 0) <= 25;
}
double calculateCost(const Order& o) {
    double km = distance(o.fromX, o.fromY, o.toX, o.toY);
    return round((BASE_PRICE + PRICE_PER_KM * km + PRICE_PER_KG * o.weight) * 100) / 100;
}
void payOrder() {
    int method;
    do {
        method = readInt("Спосіб оплати (1 - картка, 2 - готівка): ");
    } while (method != 1 && method != 2);
    if (method == 1) {
        cout << "Підтвердіть результат оплати.\n";
        while (readInt("Оплата успішна? (1 - так, 0 - ні): ") != 1) {
        }
    }
}
int findCourier(double x, double y) {
    int best = -1; double minDist = 1e9;
    for (int i = 0;i < N;i++) {
        if (!courierOnline[i]) continue;
        double d = distance(x, y, courierX[i], courierY[i]);
        if (d < minDist) {
            minDist = d; best = i;
        }
    }
    return best;
}
int findOrder(const string& number) {
    for (int i = 0;i < orderCount;i++)
        if (orders[i].trackingNumber == number) return i;
    return -1;
}

string generateTrackingNumber() {
    static mt19937 generator(random_device{}());
    static uniform_int_distribution<int> digit(0, 9);
    string number;
    do {
        number = "KR";
        for (int i = 0; i < 10; ++i)
            number += static_cast<char>('0' + digit(generator));
    } while (findOrder(number) != -1);
    return number;
}

void updateStatuses() {
    for (int i = 0;i < orderCount;i++) {
        Order& o = orders[i];
        if (o.status != "В дорозі") continue;
        auto seconds = chrono::duration_cast<chrono::seconds>(Clock::now() - o.deliveryStart).count();
        if (seconds >= static_cast<long long>(o.etaMinutes) * SECONDS_PER_MINUTE) {
            o.status = "Доставлено";
            if (o.courier >= 0) {
                courierX[o.courier] = o.toX;
                courierY[o.courier] = o.toY;
            }
            cout << "\n[АВТОМАТИЧНО] Замовлення " << o.trackingNumber
                << " завершено за розрахованим часом.\n";
            cout << "[PUSH клієнту] Доставку завершено за таймером.\n";
        }
    }
}
void createOrder() {
    if (orderCount >= MAX_ORDERS) {
        cout << "Досягнуто максимальної кількості замовлень.\n"; return;
    }
    Order o;
    inputOrder(o);
    while (!inDeliveryZone(o)) {
        cout << "Адреса поза зоною доставки.\n";
        inputAddresses(o);
    }
    o.cost = calculateCost(o);
    double km = distance(o.fromX, o.fromY, o.toX, o.toY);
    cout << fixed << setprecision(2);
    cout << "\nВідправлення: " << o.fromAddress
        << "\nОтримання: " << o.toAddress
        << "\nУмовна відстань: " << km << " км"
        << "\nВартість: " << o.cost << " грн\n";
    payOrder();
    if (readInt("Підтвердити замовлення? (1 - так, 0 - ні): ") != 1) {
        cout << "Замовлення скасовано.\n"; return;
    }
    o.trackingNumber = generateTrackingNumber();
    o.courier = findCourier(o.fromX, o.fromY);
    o.status = (o.courier < 0) ? "Створено" : "Кур'єра призначено";
    orders[orderCount++] = o;
    cout << "\n[SMS] Замовлення " << o.trackingNumber << " створено.\n";
    cout << "Статус: " << o.status << '\n';
    if (o.courier >= 0) {
        double d = distance(courierX[o.courier], courierY[o.courier], o.fromX, o.fromY);
        cout << "Кур'єр: " << courierName[o.courier]
            << ", до відправника приблизно " << arrivalTime(d) << " хв.\n";
    }
}
void trackDelivery() {
    updateStatuses();
    string number;
    cout << "Трекінг-номер: "; cin >> number;
    int i = findOrder(number);
    if (i < 0) {
        cout << "Замовлення не знайдено.\n"; return;
    }
    const Order& o = orders[i];
    cout << "\nЗамовлення: " << o.trackingNumber
        << "\nВідправлення: " << o.fromAddress
        << "\nОтримання: " << o.toAddress
        << "\nСтатус: " << o.status << '\n';
    if (o.courier >= 0) cout << "Кур'єр: " << courierName[o.courier] << '\n';
    if (o.status == "В дорозі") {
        auto elapsed = chrono::duration_cast<chrono::seconds>(Clock::now() - o.deliveryStart).count();
        long long total = static_cast<long long>(o.etaMinutes) * SECONDS_PER_MINUTE;
        long long remaining = max(0LL, total - elapsed);
        cout << "Залишилося за таймером: " << remaining << " секунд"
            << '\n';
    }
}
void startDelivery(Order& o) {
    int c = o.courier;
    if (c < 0) {
        cout << "Кур'єра ще не призначено.\n"; return;
    }
    courierX[c] = o.fromX; courierY[c] = o.fromY;
    o.status = "В дорозі";
    double km = distance(o.fromX, o.fromY, o.toX, o.toY);
    o.etaMinutes = arrivalTime(km);
    o.deliveryStart = Clock::now();
    cout << "\nДоставку розпочато! Статус: В дорозі\n"
        << "Адреса отримання: " << o.toAddress << '\n'
        << "Орієнтовний час: " << o.etaMinutes << " хв\n"
        << "[PUSH] Кур'єр у дорозі.\n";
}
void handOver(Order& o) {
    string code;
    do {
        cout << "Введіть штрих-код (трекінг-номер): "; cin >> code;
    } while (code != o.trackingNumber);
    while (readInt("Підпис отримувача отримано? (1 - так, 0 - ні): ") != 1) {
    }
    o.status = "Доставлено";
    courierX[o.courier] = o.toX;
    courierY[o.courier] = o.toY;
    cout << "Посилку вручено. Статус: Доставлено.\n";
}
void deliverOrder() {
    updateStatuses();
    string number;
    cout << "Трекінг-номер: "; cin >> number;
    int i = findOrder(number);
    if (i < 0) {
        cout << "Замовлення не знайдено.\n"; return;
    }
    Order& o = orders[i];
    if (o.status == "Кур'єра призначено") startDelivery(o);
    else if (o.status == "В дорозі") handOver(o);
    else cout << "Поточний статус: " << o.status << '\n';
}
int main() {
    cout << "АС «Коробочка»\n";
    while (true) {
        updateStatuses();
        cout << "\n===== АС «Коробочка» =====\n"
            << "1 - Оформити замовлення\n"
            << "2 - Відстежити доставку\n"
            << "3 - Доставити замовлення (кур'єр)\n"
            << "0 - Вихід\n";
        int choice = readInt("Ваш вибір: ");
        switch (choice) {
        case 1: createOrder(); break;
        case 2: trackDelivery(); break;
        case 3: deliverOrder(); break;
        case 0: return 0;
        default: cout << "Неправильний вибір.\n";
        }
    }
}
