#ifndef STRESS_TESTS_HPP
#define STRESS_TESTS_HPP

#include <iostream>
#include <utility>
#include <chrono>

#include "UnqPtr.hpp"
#include "ShrdPtr.hpp"
#include "TestObject.hpp"

// Сохранить как StressTests.hpp. Сборка: C++20.
// <chrono> используется только для времени; тестовых библиотек нет.
// Запускать последовательно, без других живых TestObject.
// Это повторяемые нагрузки с проверкой результата, не строгий бенчмарк.
inline void check_stress(bool condition, const char* test_name, int& tests_passed, int& tests_failed) {
    if (condition) {
        std::cout << "  Успешно: " << test_name << "\n";
        tests_passed++;
    } else {
        std::cout << "  Провалено: " << test_name << "\n";
        tests_failed++;
    }
}

// Общий вывод после нагрузки. Счетчики сбрасываются перед каждым сценарием.
inline void finish_stress(bool Correct, int ExpectedCreated, int PeakAlive, double TimeMs, int& tests_passed, int& tests_failed) {
    check_stress(Correct, "Все итерации сохранили ожидаемые адреса, значения и владение", tests_passed, tests_failed);
    check_stress(static_cast<int>(TestObject::GetCreatedCount()) == ExpectedCreated, "Создано ожидаемое число объектов", tests_passed, tests_failed);
    check_stress(TestObject::GetAliveCount() == 0, "После нагрузки нет живых TestObject", tests_passed, tests_failed);
    check_stress(TestObject::GetCreatedCount() == TestObject::GetDestroyedCount(), "Каждый созданный TestObject уничтожен", tests_passed, tests_failed);
    check_stress(TestObject::GetCopyConstructorCount() == 0 && TestObject::GetMoveConstructorCount() == 0, "Объекты не копировались и не перемещались", tests_passed, tests_failed);
    check_stress(TestObject::GetCopyAssignmentCount() == 0 && TestObject::GetMoveAssignmentCount() == 0, "Не было присваиваний самих TestObject", tests_passed, tests_failed);
    std::cout << "  Время нагрузки с проверками: " << TimeMs << " мс\n";
    std::cout << "  Пик живых TestObject: " << PeakAlive << "\n";
    std::cout << "  Объем самих TestObject на пике: " << PeakAlive * sizeof(TestObject) << " байт\n";
}

// Возвращает количество проваленных проверок. Объем нагрузки меняется здесь.
inline int run_stress_tests() {
    const int Iterations = 1000;
    const int OwnersCount = 64;
    const int ArraySize = 64;
    int tests_passed = 0;
    int tests_failed = 0;
    std::cout << "\n--- Стресс-тесты указателей ---\n";
    if (TestObject::GetAliveCount() != 0) {
        std::cout << "Нельзя начинать тесты: есть живые TestObject.\n";
        return 1;
    }

    try {
        // 1. Одновременно удерживаем несколько объектов, затем передаем владение.
        // OwnersCount должен быть положительным четным числом.
        std::cout << "\nUnqPtr<T>: " << Iterations << " серий по " << OwnersCount << " владельцев\n";
        TestObject::ResetCounters();
        {
            bool Correct = true;
            int PeakAlive = 0;
            auto Start = std::chrono::steady_clock::now();
            for (int iteration = 0; iteration < Iterations; iteration++) {
                UnqPtr<TestObject> Objects[OwnersCount];
                UnqPtr<TestObject> Moved[OwnersCount];
                for (int index = 0; index < OwnersCount; index++) {
                    Objects[index].reset(new TestObject(index));
                }
                for (int index = 0; index < OwnersCount; index += 2) {
                    Objects[index].swap(Objects[index + 1]);
                }
                for (int index = 0; index < OwnersCount; index++) {
                    int ExpectedValue = index % 2 == 0 ? index + 1 : index - 1;
                    TestObject* RawPointer = Objects[index].get();
                    UnqPtr<TestObject> TempPointer(std::move(Objects[index]));
                    Moved[index] = std::move(TempPointer);
                    if (Objects[index] || TempPointer || Moved[index].get() != RawPointer || !Moved[index] || Moved[index]->GetValue() != ExpectedValue) {Correct = false;}
                }
                TestObject* Replacement = new TestObject(-1);
                int Alive = static_cast<int>(TestObject::GetAliveCount());
                if (Alive > PeakAlive) {PeakAlive = Alive;}
                Moved[0].reset(Replacement);
                if (!Moved[0] || Moved[0]->GetValue() != -1 || TestObject::GetAliveCount() != OwnersCount) {Correct = false;}
                for (int index = 0; index < OwnersCount; index++) {
                    TestObject* OriginalPointer = Moved[index].get();
                    TestObject* RawPointer = Moved[index].release();
                    if (Moved[index] || RawPointer != OriginalPointer) {Correct = false;}
                    delete RawPointer;
                }
                if (TestObject::GetAliveCount() != 0) {Correct = false;}
            }
            auto Finish = std::chrono::steady_clock::now();
            double TimeMs = std::chrono::duration<double, std::milli>(Finish - Start).count();
            finish_stress(Correct, Iterations * (OwnersCount + 1), PeakAlive, TimeMs, tests_passed, tests_failed);
        }
        if (TestObject::GetAliveCount() != 0) {return tests_failed;}

        // 2. Разные длины массивов, запись каждого элемента, move и reset.
        std::cout << "\nUnqPtr<T[]>: " << Iterations << " серий с массивами\n";
        TestObject::ResetCounters();
        {
            bool Correct = true;
            int ExpectedCreated = 0;
            int PeakAlive = 0;
            auto Start = std::chrono::steady_clock::now();
            for (int iteration = 0; iteration < Iterations; iteration++) {
                int Count = iteration % ArraySize + 1;
                UnqPtr<TestObject[]> first(new TestObject[Count]);
                ExpectedCreated += Count;
                for (int index = 0; index < Count; index++) {first[index].SetValue(index + iteration);}
                TestObject* RawPointer = first.get();
                UnqPtr<TestObject[]> second(std::move(first));
                if (first || second.get() != RawPointer || !second) {Correct = false;}
                if (second) {
                    for (int index = 0; index < Count; index++) {
                        if (second[index].GetValue() != index + iteration) {Correct = false;}
                    }
                }
                TestObject* Replacement = new TestObject[ArraySize];
                ExpectedCreated += ArraySize;
                int Alive = static_cast<int>(TestObject::GetAliveCount());
                if (Alive > PeakAlive) {PeakAlive = Alive;}
                second.reset(Replacement);
                first.swap(second);
                if (second || first.get() != Replacement || TestObject::GetAliveCount() != ArraySize) {Correct = false;}
                RawPointer = first.release();
                if (first || RawPointer != Replacement) {Correct = false;}
                delete[] RawPointer;
                if (TestObject::GetAliveCount() != 0) {Correct = false;}
            }
            auto Finish = std::chrono::steady_clock::now();
            double TimeMs = std::chrono::duration<double, std::milli>(Finish - Start).count();
            finish_stress(Correct, ExpectedCreated, PeakAlive, TimeMs, tests_passed, tests_failed);
        }
        if (TestObject::GetAliveCount() != 0) {return tests_failed;}

        // 3. Много владельцев двух разных групп. Проверяем каждое изменение счетчика.
        std::cout << "\nShrdPtr<T>: " << Iterations << " серий по " << OwnersCount << " копий\n";
        TestObject::ResetCounters();
        {
            bool Correct = true;
            int PeakAlive = 0;
            auto Start = std::chrono::steady_clock::now();
            for (int iteration = 0; iteration < Iterations; iteration++) {
                UnqPtr<TestObject> source(new TestObject(iteration));
                ShrdPtr<TestObject> first(std::move(source));
                ShrdPtr<TestObject> Owners[OwnersCount];
                TestObject* RawPointer = first.get();
                for (int index = 0; index < OwnersCount; index++) {
                    Owners[index] = first;
                    if (static_cast<int>(first.UseCount()) != index + 2 || Owners[index].get() != RawPointer) {Correct = false;}
                }
                UnqPtr<TestObject> replacement(new TestObject(-1));
                int Alive = static_cast<int>(TestObject::GetAliveCount());
                if (Alive > PeakAlive) {PeakAlive = Alive;}
                first.reset(std::move(replacement));
                if (source || replacement || !first.unique() || Owners[0].UseCount() != OwnersCount || TestObject::GetAliveCount() != 2) {Correct = false;}
                int RemainingOwners = OwnersCount;
                for (int index = 0; index < OwnersCount; index += 2) {
                    Owners[index] = std::move(Owners[index + 1]);
                    RemainingOwners--;
                    if (Owners[index + 1] || static_cast<int>(Owners[index].UseCount()) != RemainingOwners || Owners[index].get() != RawPointer) {Correct = false;}
                }
                for (int index = OwnersCount - 2; index >= 0; index -= 2) {
                    if (!Owners[index] || Owners[index]->GetValue() != iteration || static_cast<int>(Owners[index].UseCount()) != RemainingOwners) {Correct = false;}
                    Owners[index].reset();
                    RemainingOwners--;
                    int ExpectedAlive = RemainingOwners == 0 ? 1 : 2;
                    if (static_cast<int>(TestObject::GetAliveCount()) != ExpectedAlive) {Correct = false;}
                }
                if (!first || first->GetValue() != -1 || !first.unique()) {Correct = false;}
                first.reset();
                if (TestObject::GetAliveCount() != 0) {Correct = false;}
            }
            auto Finish = std::chrono::steady_clock::now();
            double TimeMs = std::chrono::duration<double, std::milli>(Finish - Start).count();
            finish_stress(Correct, Iterations * 2, PeakAlive, TimeMs, tests_passed, tests_failed);
        }
        if (TestObject::GetAliveCount() != 0) {return tests_failed;}

        // 4. Массив удерживается большим числом копий и удаляется последней.
        std::cout << "\nShrdPtr<T[]>: " << Iterations << " серий, " << ArraySize << " элементов, " << OwnersCount << " копий\n";
        TestObject::ResetCounters();
        {
            bool Correct = true;
            int PeakAlive = 0;
            auto Start = std::chrono::steady_clock::now();
            for (int iteration = 0; iteration < Iterations; iteration++) {
                UnqPtr<TestObject[]> source(new TestObject[ArraySize]);
                ShrdPtr<TestObject[]> first(std::move(source));
                ShrdPtr<TestObject[]> Owners[OwnersCount];
                for (int index = 0; index < ArraySize; index++) {first[index].SetValue(iteration + index);}
                int Alive = static_cast<int>(TestObject::GetAliveCount());
                if (Alive > PeakAlive) {PeakAlive = Alive;}
                for (int index = 0; index < OwnersCount; index++) {
                    ShrdPtr<TestObject[]> TempPointer(first);
                    Owners[index] = std::move(TempPointer);
                    if (TempPointer || static_cast<int>(first.UseCount()) != index + 2) {Correct = false;}
                }
                first.reset();
                if (source || first || Owners[0].UseCount() != OwnersCount) {Correct = false;}
                if (Owners[0]) {Owners[0][0].SetValue(-1);}
                int RemainingOwners = OwnersCount;
                for (int index = OwnersCount - 1; index >= 0; index--) {
                    if (!Owners[index] || Owners[index][0].GetValue() != -1 || static_cast<int>(Owners[index].UseCount()) != RemainingOwners) {Correct = false;}
                    if (Owners[index]) {
                        for (int element = 1; element < ArraySize; element++) {
                            if (Owners[index][element].GetValue() != iteration + element) {Correct = false;}
                        }
                    }
                    ShrdPtr<TestObject[]> TempPointer;
                    TempPointer.swap(Owners[index]);
                    if (Owners[index] || static_cast<int>(TempPointer.UseCount()) != RemainingOwners) {Correct = false;}
                    TempPointer.reset();
                    RemainingOwners--;
                    int ExpectedAlive = RemainingOwners == 0 ? 0 : ArraySize;
                    if (static_cast<int>(TestObject::GetAliveCount()) != ExpectedAlive) {Correct = false;}
                }
            }
            auto Finish = std::chrono::steady_clock::now();
            double TimeMs = std::chrono::duration<double, std::milli>(Finish - Start).count();
            finish_stress(Correct, Iterations * ArraySize, PeakAlive, TimeMs, tests_passed, tests_failed);
        }
    } catch (...) {
        check_stress(false, "Неожиданное исключение: остальные нагрузки не выполнены", tests_passed, tests_failed);
        check_stress(TestObject::GetAliveCount() == 0, "После исключения нет живых TestObject", tests_passed, tests_failed);
    }

    std::cout << "\nСтресс-тесты: успешно " << tests_passed << ", провалено " << tests_failed << "\n";
    return tests_failed;
}

// Важные границы проверки:
// 1. Пик учитывает только TestObject, включая одновременно старую и новую память
//    перед reset. Байты = PeakAlive * sizeof(TestObject), НЕ память процесса.
//    Память владельцев, ReferenceCount и служебные данные new сюда не входят.
// 2. Счетчики объектов не обнаруживают отдельную утечку ReferenceCount.
//    Для проверки всей памяти нужен отдельный запуск с санитайзером.
// 3. Время включает проверки условий, но не вывод внутри finish_stress.
//    Фиксированного ограничения времени нет: результат зависит от компьютера.
// 4. Нет потоков, имитации отказа new и циклов совместного владения.

#endif // STRESS_TESTS_HPP
