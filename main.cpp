#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>
#include <windows.h> // مكتبة التحكم بالألوان وشاشة الويندوز

using namespace std;

// أكواد الألوان في شاشة الأوامر (Windows Console Colors)
#define COLOR_RESET   7
#define COLOR_BLUE    11
#define COLOR_GREEN   10
#define COLOR_YELLOW  14
#define COLOR_RED     12
#define COLOR_WHITE   15

enum ElevatorState { IDLE, MOVING_UP, MOVING_DOWN, DOOR_OPEN, FAULT_STOP };
enum OperationMode { MODE_NORMAL, MODE_INSPECTION };
enum FaultCode { NONE = 0, E01_OVERLOAD, E02_DOOR_OPEN, E05_INSPECTION_REJECT };

class HyundaiColoredElevator {
private:
    int currentFloor;
    int totalFloors;
    ElevatorState state;
    OperationMode opMode;
    FaultCode activeFault;
    bool isDoorOpen;
    HANDLE hConsole;

    // دالة لتغيير لون النص
    void setColor(int color) {
        SetConsoleTextAttribute(hConsole, color);
    }

    void clearScreen() {
        system("cls");
    }

public:
    HyundaiColoredElevator(int floors)
        : totalFloors(floors), currentFloor(0), state(IDLE), opMode(MODE_NORMAL), activeFault(NONE), isDoorOpen(false) {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    // رسم اللوحة بتنسيق وألوان ممتازة
    void renderPanel(int stepTime = 0, string arrowState = "   ") {
        clearScreen();

        setColor(COLOR_BLUE);
        cout << "=========================================================\n";
        cout << "       [ HYUNDAI ELEVATOR SMART CONTROL PANEL hamdy ]          \n";
        cout << "=========================================================\n";

        // عرض الوضع (Normal / Inspection)
        setColor(COLOR_WHITE);
        cout << "  MODE   : ";
        if (opMode == MODE_NORMAL) {
            setColor(COLOR_GREEN);
            cout << "[ NORMAL ]\n";
        } else {
            setColor(COLOR_YELLOW);
            cout << "[ INSPECTION (MAINTENANCE) ]\n";
        }

        // عرض حالة الباب
        setColor(COLOR_WHITE);
        cout << "  DOOR   : ";
        if (isDoorOpen) {
            setColor(COLOR_YELLOW);
            cout << "< OPEN >\n";
        } else {
            setColor(COLOR_GREEN);
            cout << "] CLOSED [\n";
        }

        // عرض الأعطال
        setColor(COLOR_WHITE);
        cout << "  STATUS : ";
        if (activeFault != NONE) {
            setColor(COLOR_RED);
            if (activeFault == E02_DOOR_OPEN) cout << "FAULT E02 - DOOR IS OPEN WHILE MOVING!\n";
            else if (activeFault == E05_INSPECTION_REJECT) cout << "FAULT E05 - REJECTED! INSPECTION MODE ACTIVE.\n";
        } else {
            setColor(COLOR_GREEN);
            cout << "ALL SYSTEMS OK\n";
        }

        setColor(COLOR_BLUE);
        cout << "---------------------------------------------------------\n";

        // رسم بئر المصعد والأدوار
        for (int f = totalFloors - 1; f >= 0; --f) {
            setColor(COLOR_WHITE);
            cout << "  [Floor " << f << "]  ";

            if (f == currentFloor) {
                if (isDoorOpen) {
                    setColor(COLOR_YELLOW);
                    cout << "|  <-- [ Elevator Car ] -->  |";
                } else {
                    setColor(COLOR_GREEN);
                    cout << "|  [|  Elevator Car  |]  |";
                }

                if (state == MOVING_UP || state == MOVING_DOWN) {
                    setColor(COLOR_YELLOW);
                    cout << "  <<< " << arrowState << " [" << stepTime << "s] >>>";
                }
            } else {
                setColor(7);
                cout << "|        |         |     |";
            }
            cout << "\n";
            setColor(COLOR_BLUE);
            cout << "          +-----------------------+\n";
        }
        cout << "=========================================================\n";
        setColor(COLOR_WHITE);
    }

    void toggleMode() {
        opMode = (opMode == MODE_NORMAL) ? MODE_INSPECTION : MODE_NORMAL;
        renderPanel();
    }

    void pressOpenDoor() {
        if (state == MOVING_UP || state == MOVING_DOWN) {
            activeFault = E02_DOOR_OPEN;
            state = FAULT_STOP;
            renderPanel();
            return;
        }
        isDoorOpen = true;
        state = DOOR_OPEN;
        renderPanel();
    }

    void pressCloseDoor() {
        isDoorOpen = false;
        state = IDLE;
        renderPanel();
    }

    void resetFaults() {
        activeFault = NONE;
        state = IDLE;
        renderPanel();
    }

    void inspectionMove(bool up) {
        if (opMode != MODE_INSPECTION) {
            setColor(COLOR_RED);
            cout << "\n[ERROR]: Switch to INSPECTION mode first!\n";
            setColor(COLOR_WHITE);
            this_thread::sleep_for(chrono::seconds(2));
            return;
        }
        if (isDoorOpen) {
            activeFault = E02_DOOR_OPEN;
            state = FAULT_STOP;
            renderPanel();
            return;
        }

        if (up && currentFloor < totalFloors - 1) currentFloor++;
        else if (!up && currentFloor > 0) currentFloor--;

        renderPanel();
    }

    // زمن الحركة (محاكاة سريعة ثانية واحدة لكل دور)
    void moveOneFloor(int targetFloor) {
        if (isDoorOpen) {
            activeFault = E02_DOOR_OPEN;
            state = FAULT_STOP;
            renderPanel();
            return;
        }

        bool goingUp = targetFloor > currentFloor;
        state = goingUp ? MOVING_UP : MOVING_DOWN;
        string arrowSymbol = goingUp ? "^^^" : "vvv";

        // وقت تحرك الدور: ثانية واحدة مع سهم متوهج
        for (int sec = 1; sec <= 5; ++sec) {
            renderPanel(sec, arrowSymbol);
            this_thread::sleep_for(chrono::seconds(1));
        }

        currentFloor = targetFloor;
        state = IDLE;
        renderPanel();
    }

    void callFloor(int target) {
        if (opMode == MODE_INSPECTION) {
            activeFault = E05_INSPECTION_REJECT;
            state = FAULT_STOP;
            renderPanel();
            return;
        }

        if (activeFault != NONE || target < 0 || target >= totalFloors) return;

        int step = (target > currentFloor) ? 1 : -1;
        while (currentFloor != target && state != FAULT_STOP) {
            moveOneFloor(currentFloor + step);
        }

        // فتح الباب لمدة 5 ثوانٍ كاملة عند الوصول
        if (state != FAULT_STOP) {
            pressOpenDoor();
            this_thread::sleep_for(chrono::seconds(5)); // <--- تأخير 5 ثوانٍ هنا
            pressCloseDoor();
        }
    }
};

int main() {
    // ضبط الترميز للويندوز
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    HyundaiColoredElevator elevator(25); // 6 أدوار (0 لـ 5)
    int choice = 0;

    while (true) {
        elevator.renderPanel();

        cout << "\n [ CONTROL BUTTONS - CHOOSE AN OPTION ]: \n";
        cout << "  1. CALL FLOOR (Automatic Move)\n";
        cout << "  2. SWITCH MODE (NORMAL / INSPECTION)\n";
        cout << "  3. INSPECTION UP   (Manual ^)\n";
        cout << "  4. INSPECTION DOWN (Manual v)\n";
        cout << "  5. OPEN DOOR\n";
        cout << "  6. CLOSE DOOR\n";
        cout << "  7. RESET FAULTS\n";
        cout << "  8. EXIT\n";
        cout << "\n Enter Button Number (1-8): ";
        cin >> choice;

        if (choice == 1) {
            int target;
            cout << " Enter target floor (0 to 24): ";
            cin >> target;
            elevator.callFloor(target);
        }
        else if (choice == 2) elevator.toggleMode();
        else if (choice == 3) elevator.inspectionMove(true);
        else if (choice == 4) elevator.inspectionMove(false);
        else if (choice == 5) elevator.pressOpenDoor();
        else if (choice == 6) elevator.pressCloseDoor();
        else if (choice == 7) elevator.resetFaults();
        else if (choice == 8) break;
    }

    return 0;
}
