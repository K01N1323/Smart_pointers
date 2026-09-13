#ifndef UNQ_PTR_TESTS_HPP
#define UNQ_PTR_TESTS_HPP

#include <iostream>
#include <utility>

#include "UnqPtr.hpp"
#include "TestObject.hpp"

inline void check_unq(bool condition, const char* test_name, int& tests_passed, int& tests_failed) {
    if (condition) {
        std::cout << "  Успешно: " << test_name << "\n";
        tests_passed++;
    } else {
        std::cout << "  Провалено: " << test_name << "\n";
        tests_failed++;
    }
}

// У каждой части объекта свой TestObject: счетчики проверяют уничтожение обеих.
// В нашей реализации удаление через Base требует виртуального деструктора.
class UnqTestBase {
private:
    TestObject value;

public:
    UnqTestBase() noexcept = default;
    virtual ~UnqTestBase() noexcept = default;
    virtual int GetKind() const noexcept {return 1;}
};

class UnqTestDerived : public UnqTestBase {
private:
    TestObject ExtraValue;

public:
    UnqTestDerived() noexcept = default;
    ~UnqTestDerived() noexcept override = default;
    int GetKind() const noexcept override {return 2;}
};

// Возвращает количество проваленных проверок. 0 означает успешный запуск.
inline int run_unq_ptr_tests() {
    int tests_passed = 0;
    int tests_failed = 0;

    std::cout << "\n--- Тестирование UnqPtr ---\n";

    if (TestObject::GetAliveCount() != 0) {
        std::cout << "Нельзя начинать тесты: есть живые TestObject.\n";
        return 1;
    }
    TestObject::ResetCounters();

    try {

        // 1. Пустой указатель
        {
            UnqPtr<TestObject> pointer;
            UnqPtr<TestObject> second(nullptr);
            check_unq(pointer.get() == nullptr, "Пустой get возвращает nullptr", tests_passed, tests_failed);
            check_unq(!pointer && !second, "Пустые указатели преобразуются в false", tests_passed, tests_failed);
            check_unq(pointer.release() == nullptr, "release пустого указателя возвращает nullptr", tests_passed, tests_failed);
            pointer.reset();
            pointer.reset(nullptr);
            check_unq(!pointer, "Повторный reset пустого указателя безопасен", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Пустой указатель: все объекты уничтожены", tests_passed, tests_failed);

        // 2. Создание, доступ и уничтожение
        {
            TestObject* RawPointer = new TestObject(10);
            UnqPtr<TestObject> pointer(RawPointer);
            check_unq(pointer.get() == RawPointer, "Конструктор сохраняет исходный адрес", tests_passed, tests_failed);
            check_unq(static_cast<bool>(pointer), "Непустой указатель преобразуется в true", tests_passed, tests_failed);
            check_unq(pointer && (*pointer).GetValue() == 10, "operator* дает доступ к объекту", tests_passed, tests_failed);
            if (pointer) {pointer->SetValue(20);}
            check_unq(pointer && pointer->GetValue() == 20, "operator-> позволяет изменять объект", tests_passed, tests_failed);
            check_unq(TestObject::GetAliveCount() == 1, "Указатель не создает копию объекта", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Создание, доступ и уничтожение: все объекты уничтожены", tests_passed, tests_failed);

        // 3. Константный владелец и константный объект
        {
            const UnqPtr<TestObject> pointer(new TestObject(15));
            if (pointer) {pointer->SetValue(25);}
            check_unq(pointer && (*pointer).GetValue() == 25, "const владелец не запрещает изменение объекта", tests_passed, tests_failed);
            UnqPtr<const TestObject> second(new TestObject(30));
            check_unq(second && second->GetValue() == 30, "Владение константным объектом", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Константный владелец и константный объект: все объекты уничтожены", tests_passed, tests_failed);

        // 4. Перемещающий конструктор
        {
            UnqPtr<TestObject> first(new TestObject(10));
            TestObject* RawPointer = first.get();
            UnqPtr<TestObject> second(std::move(first));
            check_unq(!first && first.get() == nullptr, "Источник после move-конструктора пуст", tests_passed, tests_failed);
            check_unq(second.get() == RawPointer, "move-конструктор сохраняет адрес объекта", tests_passed, tests_failed);
            check_unq(second && second->GetValue() == 10, "move-конструктор сохраняет значение", tests_passed, tests_failed);
            first.reset(new TestObject(20));
            check_unq(first && first->GetValue() == 20, "Источник можно использовать повторно", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Перемещающий конструктор: все объекты уничтожены", tests_passed, tests_failed);

        // 5. Перемещающий конструктор из пустого указателя
        {
            UnqPtr<TestObject> first;
            UnqPtr<TestObject> second(std::move(first));
            check_unq(!first && !second, "Перемещение пустого указателя", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Перемещающий конструктор из пустого указателя: все объекты уничтожены", tests_passed, tests_failed);

        // 6. Перемещающее присваивание непустому владельцу
        {
            UnqPtr<TestObject> first(new TestObject(10));
            UnqPtr<TestObject> second(new TestObject(20));
            TestObject* RawPointer = first.get();
            UnqPtr<TestObject>& Result = (second = std::move(first));
            check_unq(&Result == &second, "Присваивание возвращает ссылку на получателя", tests_passed, tests_failed);
            check_unq(!first && second.get() == RawPointer, "Присваивание передает владение", tests_passed, tests_failed);
            check_unq(TestObject::GetAliveCount() == 1, "Предыдущий объект получателя удален", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Перемещающее присваивание непустому владельцу: все объекты уничтожены", tests_passed, tests_failed);

        // 7. Пустые состояния при присваивании
        {
            UnqPtr<TestObject> first(new TestObject(10));
            UnqPtr<TestObject> second;
            second = std::move(first);
            check_unq(!first && second, "Присваивание непустого источника пустому получателю", tests_passed, tests_failed);
            second = std::move(first);
            check_unq(!second && TestObject::GetAliveCount() == 0, "Пустой источник освобождает получателя", tests_passed, tests_failed);
            second = std::move(first);
            check_unq(!first && !second, "Присваивание пустого пустому", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Пустые состояния при присваивании: все объекты уничтожены", tests_passed, tests_failed);

        // 8. Самоперемещение
        {
            UnqPtr<TestObject> pointer(new TestObject(10));
            TestObject* RawPointer = pointer.get();
            UnqPtr<TestObject>& Alias = pointer;
            pointer = std::move(Alias);
            check_unq(pointer.get() == RawPointer, "Самоперемещение сохраняет объект", tests_passed, tests_failed);
            check_unq(TestObject::GetAliveCount() == 1, "Самоперемещение не удаляет объект", tests_passed, tests_failed);
            pointer.reset();
            pointer = std::move(Alias);
            check_unq(!pointer, "Самоперемещение пустого указателя", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Самоперемещение: все объекты уничтожены", tests_passed, tests_failed);

        // 9. release
        {
            UnqPtr<TestObject> pointer(new TestObject(10));
            TestObject* OriginalPointer = pointer.get();
            TestObject* RawPointer = pointer.release();
            check_unq(!pointer && RawPointer == OriginalPointer, "release возвращает адрес и очищает владельца", tests_passed, tests_failed);
            check_unq(TestObject::GetAliveCount() == 1, "release не удаляет объект", tests_passed, tests_failed);
            check_unq(pointer.release() == nullptr, "Повторный release возвращает nullptr", tests_passed, tests_failed);
            delete RawPointer;
            check_unq(TestObject::GetAliveCount() == 0, "Освобожденный через release объект удаляется вручную", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "release: все объекты уничтожены", tests_passed, tests_failed);

        // 10. reset
        {
            UnqPtr<TestObject> pointer(new TestObject(10));
            TestObject* NewPointer = new TestObject(20);
            pointer.reset(NewPointer);
            check_unq(pointer.get() == NewPointer, "reset принимает новый адрес", tests_passed, tests_failed);
            check_unq(pointer && pointer->GetValue() == 20, "reset сохраняет новый объект", tests_passed, tests_failed);
            check_unq(TestObject::GetAliveCount() == 1, "reset удаляет прежний объект", tests_passed, tests_failed);
            pointer.reset(pointer.get());
            check_unq(pointer.get() == NewPointer && TestObject::GetAliveCount() == 1, "reset(get()) безопасен в нашей реализации", tests_passed, tests_failed);
            pointer.reset(nullptr);
            check_unq(!pointer && TestObject::GetAliveCount() == 0, "reset(nullptr) удаляет объект", tests_passed, tests_failed);
            pointer.reset(new TestObject(30));
            pointer.reset();
            check_unq(!pointer && TestObject::GetAliveCount() == 0, "reset() удаляет объект", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "reset: все объекты уничтожены", tests_passed, tests_failed);

        // 11. Обмен
        {
            UnqPtr<TestObject> first(new TestObject(10));
            UnqPtr<TestObject> second(new TestObject(20));
            TestObject* FirstPointer = first.get();
            TestObject* SecondPointer = second.get();
            first.swap(second);
            check_unq(first.get() == SecondPointer && second.get() == FirstPointer, "Метод swap обменивает адреса", tests_passed, tests_failed);
            swap(first, second);
            check_unq(first.get() == FirstPointer && second.get() == SecondPointer, "Свободная функция swap", tests_passed, tests_failed);
            first.swap(first);
            check_unq(first.get() == FirstPointer, "Обмен указателя с самим собой", tests_passed, tests_failed);
            UnqPtr<TestObject> empty;
            first.swap(empty);
            check_unq(!first && empty.get() == FirstPointer, "Обмен непустого и пустого указателя", tests_passed, tests_failed);
            UnqPtr<TestObject> another;
            first.swap(another);
            check_unq(!first && !another, "Обмен двух пустых указателей", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Обмен: все объекты уничтожены", tests_passed, tests_failed);

        // 12. Преобразование дочернего типа: конструктор
        {
            UnqPtr<UnqTestDerived> first(new UnqTestDerived);
            UnqTestBase* RawPointer = first.get();
            UnqPtr<UnqTestBase> second(std::move(first));
            check_unq(!first && second.get() == RawPointer, "Derived -> Base передает адрес", tests_passed, tests_failed);
            check_unq(second && second->GetKind() == 2, "После преобразования работает виртуальный метод", tests_passed, tests_failed);
            check_unq(TestObject::GetAliveCount() == 2, "Базовая и дочерняя части объекта живы", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Преобразование дочернего типа: конструктор: все объекты уничтожены", tests_passed, tests_failed);

        // 13. Преобразование дочернего типа: присваивание
        {
            UnqPtr<UnqTestBase> first(new UnqTestBase);
            UnqPtr<UnqTestDerived> second(new UnqTestDerived);
            UnqTestBase* RawPointer = second.get();
            UnqPtr<UnqTestBase>& Result = (first = std::move(second));
            check_unq(&Result == &first && !second && first.get() == RawPointer, "Присваивание Derived -> Base", tests_passed, tests_failed);
            check_unq(TestObject::GetAliveCount() == 2, "Прежний базовый объект удален", tests_passed, tests_failed);
            UnqPtr<UnqTestDerived> empty;
            first = std::move(empty);
            check_unq(!first && TestObject::GetAliveCount() == 0, "Пустой Derived освобождает Base", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Преобразование дочернего типа: присваивание: все объекты уничтожены", tests_passed, tests_failed);

        // 14. Пустое преобразование и добавление const
        {
            UnqPtr<UnqTestDerived> empty;
            UnqPtr<UnqTestBase> base(std::move(empty));
            check_unq(!empty && !base, "Преобразующий конструктор из пустого указателя", tests_passed, tests_failed);
            UnqPtr<TestObject> first(new TestObject(10));
            UnqPtr<const TestObject> second(std::move(first));
            check_unq(!first && second && second->GetValue() == 10, "Перемещение T -> const T", tests_passed, tests_failed);
            UnqPtr<TestObject> third(new TestObject(20));
            second = std::move(third);
            check_unq(!third && second && second->GetValue() == 20, "Присваивание T -> const T", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Пустое преобразование и добавление const: все объекты уничтожены", tests_passed, tests_failed);

        // 15. Пустой массив
        {
            UnqPtr<TestObject[]> pointer;
            UnqPtr<TestObject[]> second(nullptr);
            check_unq(!pointer && !second && pointer.get() == nullptr, "Пустые владельцы массивов", tests_passed, tests_failed);
            check_unq(pointer.release() == nullptr, "release пустого массива", tests_passed, tests_failed);
            pointer.reset();
            pointer.reset(nullptr);
            check_unq(!pointer, "reset пустого массива", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Пустой массив: все объекты уничтожены", tests_passed, tests_failed);

        // 16. Массив: создание и индексация
        {
            TestObject* RawPointer = new TestObject[3];
            UnqPtr<TestObject[]> pointer(RawPointer);
            check_unq(pointer.get() == RawPointer && pointer, "Конструктор массива сохраняет адрес", tests_passed, tests_failed);
            if (pointer) {
                pointer[0].SetValue(10);
                pointer[1].SetValue(20);
                pointer[2].SetValue(30);
            }
            check_unq(pointer && pointer[0].GetValue() == 10 && pointer[2].GetValue() == 30, "operator[] первого и последнего элемента", tests_passed, tests_failed);
            const UnqPtr<TestObject[]>& ConstPointer = pointer;
            if (ConstPointer) {ConstPointer[1].SetValue(40);}
            check_unq(ConstPointer && ConstPointer[1].GetValue() == 40, "Индексация через const владельца", tests_passed, tests_failed);
            check_unq(TestObject::GetAliveCount() == 3, "Созданы все три элемента массива", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Массив: создание и индексация: все объекты уничтожены", tests_passed, tests_failed);

        // 17. Массив: move-конструктор
        {
            UnqPtr<TestObject[]> first(new TestObject[3]);
            TestObject* RawPointer = first.get();
            UnqPtr<TestObject[]> second(std::move(first));
            check_unq(!first && second.get() == RawPointer, "move-конструктор массива", tests_passed, tests_failed);
            check_unq(TestObject::GetAliveCount() == 3, "При move элементы не пересоздаются", tests_passed, tests_failed);
            UnqPtr<TestObject[]> third(std::move(first));
            check_unq(!first && !third, "move-конструктор из пустого массива", tests_passed, tests_failed);
            first.reset(new TestObject[1]);
            check_unq(first && TestObject::GetAliveCount() == 4, "Повторное использование перемещенного владельца массива", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Массив: move-конструктор: все объекты уничтожены", tests_passed, tests_failed);

        // 18. Массив: move-присваивание
        {
            UnqPtr<TestObject[]> first(new TestObject[3]);
            UnqPtr<TestObject[]> second(new TestObject[2]);
            TestObject* RawPointer = first.get();
            UnqPtr<TestObject[]>& Result = (second = std::move(first));
            check_unq(&Result == &second && !first && second.get() == RawPointer, "Присваивание массива передает адрес", tests_passed, tests_failed);
            check_unq(TestObject::GetAliveCount() == 3, "Прежние элементы получателя удалены", tests_passed, tests_failed);
            first = std::move(second);
            check_unq(first.get() == RawPointer && !second, "Присваивание массива пустому получателю", tests_passed, tests_failed);
            first = std::move(second);
            check_unq(!first && TestObject::GetAliveCount() == 0, "Пустой источник удаляет прежний массив", tests_passed, tests_failed);
            first = std::move(second);
            check_unq(!first && !second, "Присваивание двух пустых массивов", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Массив: move-присваивание: все объекты уничтожены", tests_passed, tests_failed);

        // 19. Массив: самоперемещение
        {
            UnqPtr<TestObject[]> pointer(new TestObject[2]);
            TestObject* RawPointer = pointer.get();
            UnqPtr<TestObject[]>& Alias = pointer;
            pointer = std::move(Alias);
            check_unq(pointer.get() == RawPointer && TestObject::GetAliveCount() == 2, "Самоперемещение массива", tests_passed, tests_failed);
            pointer.reset();
            pointer = std::move(Alias);
            check_unq(!pointer, "Самоперемещение пустого массива", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Массив: самоперемещение: все объекты уничтожены", tests_passed, tests_failed);

        // 20. Массив: release
        {
            UnqPtr<TestObject[]> pointer(new TestObject[3]);
            TestObject* OriginalPointer = pointer.get();
            TestObject* RawPointer = pointer.release();
            check_unq(!pointer && RawPointer == OriginalPointer, "release массива передает исходный адрес", tests_passed, tests_failed);
            check_unq(TestObject::GetAliveCount() == 3, "release массива не удаляет элементы", tests_passed, tests_failed);
            check_unq(pointer.release() == nullptr, "Повторный release массива", tests_passed, tests_failed);
            delete[] RawPointer;
            check_unq(TestObject::GetAliveCount() == 0, "После release используется delete[]", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Массив: release: все объекты уничтожены", tests_passed, tests_failed);

        // 21. Массив: reset
        {
            UnqPtr<TestObject[]> pointer(new TestObject[3]);
            TestObject* NewPointer = new TestObject[2];
            pointer.reset(NewPointer);
            check_unq(pointer.get() == NewPointer && TestObject::GetAliveCount() == 2, "reset удаляет все прежние элементы", tests_passed, tests_failed);
            pointer.reset(pointer.get());
            check_unq(pointer.get() == NewPointer && TestObject::GetAliveCount() == 2, "reset(get()) массива сохраняет элементы", tests_passed, tests_failed);
            pointer.reset(nullptr);
            check_unq(!pointer && TestObject::GetAliveCount() == 0, "reset(nullptr) массива", tests_passed, tests_failed);
            pointer.reset(new TestObject[1]);
            pointer.reset();
            check_unq(!pointer && TestObject::GetAliveCount() == 0, "reset() массива", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Массив: reset: все объекты уничтожены", tests_passed, tests_failed);

        // 22. Массив: swap
        {
            UnqPtr<TestObject[]> first(new TestObject[2]);
            UnqPtr<TestObject[]> second(new TestObject[3]);
            TestObject* FirstPointer = first.get();
            TestObject* SecondPointer = second.get();
            first.swap(second);
            check_unq(first.get() == SecondPointer && second.get() == FirstPointer, "Метод swap массивов", tests_passed, tests_failed);
            swap(first, second);
            check_unq(first.get() == FirstPointer && second.get() == SecondPointer, "Свободный swap массивов", tests_passed, tests_failed);
            first.swap(first);
            check_unq(first.get() == FirstPointer, "Обмен массива с самим собой", tests_passed, tests_failed);
            UnqPtr<TestObject[]> empty;
            first.swap(empty);
            check_unq(!first && empty.get() == FirstPointer, "Обмен массива с пустым владельцем", tests_passed, tests_failed);
            UnqPtr<TestObject[]> another;
            first.swap(another);
            check_unq(!first && !another, "Обмен двух пустых владельцев массивов", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Массив: swap: все объекты уничтожены", tests_passed, tests_failed);

        // 23. Массив константных элементов
        {
            UnqPtr<const TestObject[]> pointer(new TestObject[2]);
            check_unq(pointer && pointer[0].GetValue() == 0, "Владение массивом const T", tests_passed, tests_failed);
            pointer.reset(new TestObject[3]);
            check_unq(pointer && pointer[2].GetValue() == 0 && TestObject::GetAliveCount() == 3, "reset массива const T", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Массив константных элементов: все объекты уничтожены", tests_passed, tests_failed);

        // 24. Многократная передача владения
        {
            bool Correct = true;
            for (int index = 0; index < 1000; index++) {
                UnqPtr<TestObject> first(new TestObject(index));
                UnqPtr<TestObject> second(std::move(first));
                UnqPtr<TestObject> third;
                third = std::move(second);
                if (first || second || !third || third->GetValue() != index) {Correct = false;}
            }
            check_unq(Correct, "1000 последовательностей передачи владения", tests_passed, tests_failed);
        }
        check_unq(TestObject::GetAliveCount() == 0, "Многократная передача владения: все объекты уничтожены", tests_passed, tests_failed);

    } catch (...) {
        check_unq(false, "Неожиданное исключение: оставшиеся сценарии не выполнены", tests_passed, tests_failed);
    }

    check_unq(TestObject::GetAliveCount() == 0, "В конце нет живых TestObject", tests_passed, tests_failed);
    check_unq(TestObject::GetCreatedCount() == TestObject::GetDestroyedCount(), "Число созданных и уничтоженных объектов совпадает", tests_passed, tests_failed);
    check_unq(TestObject::GetCopyConstructorCount() == 0, "Владение не копирует сам TestObject", tests_passed, tests_failed);
    check_unq(TestObject::GetMoveConstructorCount() == 0, "Владение не перемещает сам TestObject", tests_passed, tests_failed);
    check_unq(TestObject::GetCopyAssignmentCount() == 0, "Не вызывается копирующее присваивание TestObject", tests_passed, tests_failed);
    check_unq(TestObject::GetMoveAssignmentCount() == 0, "Не вызывается перемещающее присваивание TestObject", tests_passed, tests_failed);

    std::cout << "\nUnqPtr: успешно " << tests_passed
              << ", провалено " << tests_failed << "\n";

    return tests_failed;
}

// Отдельные РУЧНЫЕ проверки запретов компиляции.
// Это НЕ выполненные проверки: они не входят в счетчики выше.
// Временно вставляй в функцию по ОДНОМУ примеру и пробуй собрать проект.
// Каждый пример должен вызвать ошибку компиляции; затем убери его.
//
// 1. Запрет копирования одиночного владельца:
// UnqPtr<int> first(new int(1));
// UnqPtr<int> second(first);
//
// 2. Запрет копирующего присваивания:
// UnqPtr<int> first(new int(1));
// UnqPtr<int> second;
// second = first;
//
// 3. Запрет копирования массива (проверить отдельно и конструктор, и =):
// UnqPtr<int[]> first(new int[2]);
// UnqPtr<int[]> second(first);
// second = first;
//
// 4. Запрет Derived[] -> Base[] в конструкторе:
// UnqPtr<UnqTestBase[]> pointer(new UnqTestDerived[2]);
//
// 5. Запрет Derived[] -> Base[] в reset:
// UnqPtr<UnqTestBase[]> pointer;
// pointer.reset(new UnqTestDerived[2]);
//
// 6. Запрет обратного преобразования Base -> Derived:
// UnqPtr<UnqTestBase> first(new UnqTestBase);
// UnqPtr<UnqTestDerived> second(std::move(first));
//
// Не выполняй примеры 4 и 5, если они компилируются:
// это ошибка ограничений UnqPtr<T[]>, а запуск может привести к UB.
// Виртуальный деструктор НЕ делает преобразование массивов безопасным.

#endif // UNQ_PTR_TESTS_HPP
