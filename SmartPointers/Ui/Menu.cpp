#include "Menu.h"

#include <iostream>
#include <limits>
#include <utility>
#include <exception>

#include "UnqPtr.hpp"
#include "ShrdPtr.hpp"
#include "TestObject.hpp"
#include "UnqPtrTests.hpp"
#include "ShrdPtrTests.hpp"
#include "PolymorphismTests.hpp"
#include "StressTests.hpp"

// Вспомогательные функции нужны только внутри Menu.cpp.
namespace {

// Читаем одно целое число на строке.
// Отклоняем буквы, дроби, переполнение int и несколько чисел на строке.
// false означает конец ввода или ошибку потока: повторять ввод уже нельзя.
bool ReadInt(const char* Prompt, int& Value) {
    while (true) {
        std::cout << Prompt << std::flush;
        int NewValue = 0;
        if (!(std::cin >> NewValue)) {
            if (std::cin.eof() || std::cin.bad()) {return false;}
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Ошибка: введите одно целое число в диапазоне int.\n";
            continue;
        }

        bool ExtraCharacters = false;
        char Symbol = '\0';
        while (std::cin.get(Symbol)) {
            if (Symbol == '\n') {break;}
            if (Symbol != ' ' && Symbol != '\t' && Symbol != '\r') {ExtraCharacters = true;}
        }
        if (std::cin.bad()) {return false;}
        if (ExtraCharacters) {
            std::cout << "Ошибка: в строке должно быть только одно целое число.\n";
            if (std::cin.eof()) {return false;}
            continue;
        }
        Value = NewValue;
        return true;
    }
}

bool ReadChoice(const char* Prompt, int MinValue, int MaxValue, int& Choice) {
    while (ReadInt(Prompt, Choice)) {
        if (Choice >= MinValue && Choice <= MaxValue) {return true;}
        std::cout << "Выберите число от " << MinValue << " до " << MaxValue << ".\n";
    }
    return false;
}

void ShowUnqPointer(const char* Name, const UnqPtr<TestObject>& pointer) {
    std::cout << Name << ": ";
    if (!pointer) {
        std::cout << "пусто\n";
        return;
    }
    std::cout << "адрес = " << static_cast<const void*>(pointer.get()) << ", значение = " << pointer->GetValue() << "\n";
}

void ShowShrdPointer(const char* Name, const ShrdPtr<TestObject>& pointer) {
    std::cout << Name << ": ";
    if (pointer) {
        std::cout << "адрес = " << static_cast<const void*>(pointer.get()) << ", значение = " << pointer->GetValue();
    } else {
        std::cout << "пусто";
    }
    std::cout << ", UseCount = " << pointer.UseCount() << ", unique = " << (pointer.unique() ? "true" : "false") << "\n";
}

// Владельцы локальные: при выходе из демонстрации объекты освобождаются.
// false означает конец ввода: нужно завершить и главное меню.
bool RunUnqDemo() {
    UnqPtr<TestObject> first;
    UnqPtr<TestObject> second;
    std::cout << "\nUnqPtr: один объект может иметь только одного владельца.\n";

    while (true) {
        std::cout << "\n--- Состояние UnqPtr ---\n";
        ShowUnqPointer("first", first);
        ShowUnqPointer("second", second);
        std::cout << "\n1. Создать или заменить объект\n"
                  << "2. Показать состояние\n"
                  << "3. Изменить значение объекта\n"
                  << "4. Переместить владение\n"
                  << "5. Обменять first и second\n"
                  << "6. Освободить объект (reset)\n"
                  << "0. Вернуться в главное меню\n";

        int Choice = 0;
        if (!ReadChoice("Ваш выбор: ", 0, 6, Choice)) {return false;}
        if (Choice == 0) {return true;}
        if (Choice == 2) {continue;}
        if (Choice == 5) {
            first.swap(second);
            std::cout << "Владельцы обменяны.\n";
            continue;
        }

        int Number = 0;
        const char* Prompt = Choice == 4 ? "Источник (1 = first, 2 = second): " : "Указатель (1 = first, 2 = second): ";
        if (!ReadChoice(Prompt, 1, 2, Number)) {return false;}
        UnqPtr<TestObject>& Selected = Number == 1 ? first : second;
        UnqPtr<TestObject>& Other = Number == 1 ? second : first;

        if (Choice == 1) {
            int Value = 0;
            if (!ReadInt("Значение нового объекта: ", Value)) {return false;}
            Selected.reset(new TestObject(Value));
            std::cout << "Новый объект создан, прежний объект выбранного владельца освобожден, если он был.\n";
        } else if (Choice == 3) {
            if (!Selected) {
                std::cout << "Указатель пуст: сначала создайте объект.\n";
                continue;
            }
            int Value = 0;
            if (!ReadInt("Новое значение: ", Value)) {return false;}
            Selected->SetValue(Value);
        } else if (Choice == 4) {
            if (!Selected) {std::cout << "Источник пуст: другой указатель будет очищен.\n";}
            Other = std::move(Selected);
            std::cout << "Владение передано другому указателю. Источник теперь пуст.\n";
        } else if (Choice == 6) {
            Selected.reset();
            std::cout << "Выбранный указатель очищен.\n";
        }
    }
}

bool RunShrdDemo() {
    ShrdPtr<TestObject> first;
    ShrdPtr<TestObject> second;
    std::cout << "\nShrdPtr: несколько владельцев могут удерживать один объект.\n";

    while (true) {
        std::cout << "\n--- Состояние ShrdPtr ---\n";
        ShowShrdPointer("first", first);
        ShowShrdPointer("second", second);
        std::cout << "\n1. Создать или заменить объект\n"
                  << "2. Показать состояние\n"
                  << "3. Изменить значение объекта\n"
                  << "4. Переместить владение\n"
                  << "5. Обменять first и second\n"
                  << "6. Отказаться от владения (reset)\n"
                  << "7. Скопировать совместное владение\n"
                  << "0. Вернуться в главное меню\n";

        int Choice = 0;
        if (!ReadChoice("Ваш выбор: ", 0, 7, Choice)) {return false;}
        if (Choice == 0) {return true;}
        if (Choice == 2) {continue;}
        if (Choice == 5) {
            first.swap(second);
            std::cout << "Адреса и связанные с ними счетчики обменяны.\n";
            continue;
        }

        int Number = 0;
        const char* Prompt = Choice == 4 || Choice == 7 ? "Источник (1 = first, 2 = second): " : "Указатель (1 = first, 2 = second): ";
        if (!ReadChoice(Prompt, 1, 2, Number)) {return false;}
        ShrdPtr<TestObject>& Selected = Number == 1 ? first : second;
        ShrdPtr<TestObject>& Other = Number == 1 ? second : first;

        if (Choice == 1) {
            int Value = 0;
            if (!ReadInt("Значение нового объекта: ", Value)) {return false;}
            UnqPtr<TestObject> NewPointer(new TestObject(Value));
            Selected.reset(std::move(NewPointer));
            std::cout << "Создан новый объект с одним владельцем. Другие владельцы прежнего объекта сохраняют его.\n";
        } else if (Choice == 3) {
            if (!Selected) {
                std::cout << "Указатель пуст: сначала создайте объект.\n";
                continue;
            }
            int Value = 0;
            if (!ReadInt("Новое значение: ", Value)) {return false;}
            Selected->SetValue(Value);
            std::cout << "Изменение видно всем владельцам этого объекта.\n";
        } else if (Choice == 4) {
            if (!Selected) {std::cout << "Источник пуст: другой указатель будет очищен.\n";}
            Other = std::move(Selected);
            std::cout << "Владение передано другому указателю. Источник теперь пуст.\n";
        } else if (Choice == 6) {
            Selected.reset();
            std::cout << "Выбранный указатель очищен. Объект удален только если больше нет владельцев.\n";
        } else if (Choice == 7) {
            if (!Selected) {std::cout << "Источник пуст: другой указатель будет очищен.\n";}
            Other = Selected;
            std::cout << "Другому указателю присвоено совместное владение выбранного. Сам объект не копировался.\n";
        }
    }
}

// Номера совпадают с пунктами главного меню.
int RunSelectedTests(int Choice) {
    if (TestObject::GetAliveCount() != 0) {
        std::cout << "Тесты не запущены: остались живые TestObject. Нужно проверить освобождение объектов.\n";
        return 1;
    }
    switch (Choice) {
        case 3: return run_unq_ptr_tests();
        case 4: return run_shrd_ptr_tests();
        case 5: return run_polymorphism_tests();
        case 6: return run_stress_tests();
    }
    return 1;
}

void ShowTestResult(int Failed) {
    if (Failed == 0) {
        std::cout << "\nРезультат: все выполненные проверки прошли.\n";
    } else {
        std::cout << "\nРезультат: зарегистрировано ошибок — " << Failed << ". Подробности выше.\n";
    }
}

} // namespace

int RunMenu() {
    bool HadTestFailures = false;
    bool ContinueInput = true;

    while (ContinueInput) {
        std::cout << "\n========== УМНЫЕ УКАЗАТЕЛИ ==========\n"
                  << "1. Демонстрация UnqPtr\n"
                  << "2. Демонстрация ShrdPtr\n"
                  << "3. Тесты UnqPtr\n"
                  << "4. Тесты ShrdPtr\n"
                  << "5. Тесты полиморфизма\n"
                  << "6. Стресс-тесты\n"
                  << "7. Запустить все тесты\n"
                  << "0. Выход\n";

        int Choice = 0;
        if (!ReadChoice("Ваш выбор: ", 0, 7, Choice)) {break;}
        if (Choice == 0) {break;}

        try {
            if (Choice == 1) {
                ContinueInput = RunUnqDemo();
            } else if (Choice == 2) {
                ContinueInput = RunShrdDemo();
            } else {
                int Failed = 0;
                if (Choice == 7) {
                    int Completed = 0;
                    for (int TestNumber = 3; TestNumber <= 6; TestNumber++) {
                        if (TestObject::GetAliveCount() != 0) {
                            std::cout << "Общий запуск остановлен: есть живые TestObject. Оставшиеся наборы не запущены.\n";
                            Failed++;
                            break;
                        }
                        Failed += RunSelectedTests(TestNumber);
                        Completed++;
                    }
                    std::cout << "\nЗавершено наборов: " << Completed << " из 4.\n";
                } else {
                    Failed = RunSelectedTests(Choice);
                }
                ShowTestResult(Failed);
                if (Failed != 0) {HadTestFailures = true;}
            }
        } catch (const std::exception& Error) {
            std::cout << "\nОперация прервана: " << Error.what() << "\n";
            if (Choice <= 2) {std::cout << "Локальные владельцы текущей демонстрации освобождены при выходе из нее.\n";}
            if (Choice >= 3) {HadTestFailures = true;}
        } catch (...) {
            std::cout << "\nОперация прервана неизвестным исключением.\n";
            if (Choice >= 3) {HadTestFailures = true;}
        }
    }

    std::cout << "\nПрограмма завершена.\n";
    return HadTestFailures ? 1 : 0;
}
