#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>
#include <cstdlib>

using namespace std;

// أكواد الألوان القياسية (ANSI Colors) الشغالة على أندرويد ولينكس وباقي الأنظمة
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[1;36m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_YELLOW  "\033[1;33m"
#define COLOR_RED     "\033[1;31m"
#define COLOR_WHITE   "\033[1;37m"

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

    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

public:
    HyundaiColoredElevator(int floors)
        : totalFloors(floors), currentFloor(0), state(IDLE), opMode(MODE_NORMAL), activeFault(NONE), isDoorOpen(false) {}

    // رسم اللوحة بتنسيق وألوان ممتازة
    void renderPanel(int stepTime = 0, string arrowState = "   ") {
        clearScreen();

        cout << COLOR_BLUE << "=========================================================\n";
        cout << "       [ HYUNDAI ELEVATOR SMART CONTROL PANEL hamdy ]          \n";
        cout << "=========================================================\n" << COLOR_RESET;

        // عرض الوضع (Normal / Inspection)
        cout << COLOR_WHITE << "  MODE   : ";
        if (opMode == MODE_NORMAL) {
            cout << COLOR_GREEN << "[ NORMAL ]\n" << COLOR_RESET;
        } else {
            cout << COLOR_YELLOW << "[ INSPECTION (MAINTENANCE) ]\n" << COLOR_RESET;
        }

        // عرض حالة الباب
        cout << COLOR_WHITE << "  DOOR   : ";
        if (isDoorOpen) {
            cout << COLOR_YELLOW << "< OPEN >\n" << COLOR_RESET;
        } else {
            cout << COLOR_GREEN << "] CLOSED [\n" << COLOR_RESET;
        }

        // عرض الأعطال
        cout << COLOR_WHITE << "  STATUS : ";
        if (activeFault != NONE) {
            cout << COLOR_RED;
            if (activeFault == E02_DOOR_OPEN) cout << "FAULT E02 - DOOR IS OPEN WHILE MOVING!\n";
            else if (activeFault == E05_INSPECTION_REJECT) cout << "FAULT E05 - REJECTED! INSPECTION MODE ACTIVE.\n";
            cout << COLOR_RESET;
        } else {
            cout << COLOR_GREEN << "ALL SYSTEMS OK\n" << COLOR_RESET;
        }

        cout << COLOR_BLUE << "---------------------------------------------------------\n" << COLOR_RESET;

        // رسم بئر المصعد والأدوار
        for (int f = totalFloors - 1; f >= 0; --f) {
            cout << COLOR_WHITE << "  [Floor " << (f < 10 ? "0" : "") << f << "]  ";

            if (f == currentFloor) {
                if (isDoorOpen) {
                    cout << COLOR_YELLOW << "|  <-- [ Elevator Car ] -->  |" << COLOR_RESET;
                } else {
                    cout << COLOR_GREEN << "|  [|  Elevator Car  |]  |" << COLOR_RESET;
                }

                if (state == MOVING_UP || state == MOVING_DOWN) {
                    cout << COLOR_YELLOW << "  <<< " << arrowState << " [" << stepTime << "s] >>>" << COLOR_RESET;
                }
            } else {
                cout << COLOR_RESET << "|        |         |     |";
            }
            cout << "\n";
            cout << COLOR_BLUE << "          +-----------------------+\n" << COLOR_RESET;
        }
        cout << COLOR_BLUE << "=========================================================\n" << COLOR_RESET;
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
            cout << COLOR_RED << "\n[ERROR]: Switch to INSPECTION mode first!\n" << COLOR_RESET;
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

        // وقت تحرك الدور: 5 ثوانٍ مع سهم متوهج
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
            this_thread::sleep_for(chrono::seconds(5)); // تأخير 5 ثوانٍ
            pressCloseDoor();
        }
    }
};

int main() {
    HyundaiColoredElevator elevator(25); // 25 دور (0 لـ 24)
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
