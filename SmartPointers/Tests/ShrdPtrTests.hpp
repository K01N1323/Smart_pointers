#ifndef SHRD_PTR_TESTS_HPP
#define SHRD_PTR_TESTS_HPP

#include <iostream>
#include <utility>

#include "UnqPtr.hpp"
#include "ShrdPtr.hpp"
#include "TestObject.hpp"

// Сохранить как ShrdPtrTests.hpp. Сборка: C++20.
// Каждый вызов check_shrd — отдельная проверка.
// inline нужен для подключения этого hpp в несколько cpp без ошибки линковки.
// Тесты запускаются последовательно, без других живых TestObject.
// Это функциональные проверки, не замеры производительности.

inline void check_shrd(bool condition, const char* test_name, int& tests_passed, int& tests_failed) {
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
class ShrdTestBase {
private:
    TestObject value;

public:
    ShrdTestBase() noexcept = default;
    virtual ~ShrdTestBase() noexcept = default;
    virtual int GetKind() const noexcept {return 1;}
};

class ShrdTestDerived : public ShrdTestBase {
private:
    TestObject ExtraValue;

public:
    ShrdTestDerived() noexcept = default;
    ~ShrdTestDerived() noexcept override = default;
    int GetKind() const noexcept override {return 2;}
};

// Возвращает количество проваленных проверок. 0 означает успешный запуск.
inline int run_shrd_ptr_tests() {
    int tests_passed = 0;
    int tests_failed = 0;

    std::cout << "\n--- Тестирование ShrdPtr ---\n";

    if (TestObject::GetAliveCount() != 0) {
        std::cout << "Нельзя начинать тесты: есть живые TestObject.\n";
        return 1;
    }
    TestObject::ResetCounters();

    try {

        // 1. Пустой указатель
        {
            ShrdPtr<TestObject> pointer;
            check_shrd(!pointer && pointer.get() == nullptr, "Пустой указатель", tests_passed, tests_failed);
            check_shrd(pointer.UseCount() == 0 && !pointer.unique(), "У пустого указателя нет владельцев", tests_passed, tests_failed);
            pointer.reset();
            pointer.reset();
            check_shrd(!pointer && pointer.UseCount() == 0, "Повторный reset пустого указателя", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Пустой указатель: все объекты уничтожены", tests_passed, tests_failed);

        // 2. Получение владения из UnqPtr и доступ
        {
            UnqPtr<TestObject> source(new TestObject(10));
            TestObject* RawPointer = source.get();
            ShrdPtr<TestObject> pointer(std::move(source));
            check_shrd(!source && pointer.get() == RawPointer, "Конструктор забирает владение из UnqPtr", tests_passed, tests_failed);
            check_shrd(pointer.UseCount() == 1 && pointer.unique(), "Первый совместный владелец единственный", tests_passed, tests_failed);
            check_shrd(pointer && (*pointer).GetValue() == 10, "operator*", tests_passed, tests_failed);
            if (pointer) {pointer->SetValue(20);}
            check_shrd(pointer && pointer->GetValue() == 20, "operator->", tests_passed, tests_failed);
            const ShrdPtr<TestObject>& ConstPointer = pointer;
            if (ConstPointer) {ConstPointer->SetValue(30);}
            check_shrd(ConstPointer && ConstPointer->GetValue() == 30, "const владелец позволяет менять объект", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Получение владения из UnqPtr и доступ: все объекты уничтожены", tests_passed, tests_failed);

        // 3. Получение пустого UnqPtr
        {
            UnqPtr<TestObject> source;
            ShrdPtr<TestObject> pointer(std::move(source));
            check_shrd(!source && !pointer, "Конструктор из пустого UnqPtr", tests_passed, tests_failed);
            check_shrd(pointer.UseCount() == 0 && !pointer.unique(), "Пустой UnqPtr не создает группу владельцев", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Получение пустого UnqPtr: все объекты уничтожены", tests_passed, tests_failed);

        // 4. Копирующий конструктор и время жизни
        {
            UnqPtr<TestObject> source(new TestObject(10));
            ShrdPtr<TestObject> first(std::move(source));
            {
                const ShrdPtr<TestObject>& ConstFirst = first;
                ShrdPtr<TestObject> second(ConstFirst);
                check_shrd(first.get() == second.get(), "Копии хранят один адрес", tests_passed, tests_failed);
                check_shrd(first.UseCount() == 2 && second.UseCount() == 2, "Копирование увеличивает общий счетчик", tests_passed, tests_failed);
                check_shrd(!first.unique() && !second.unique(), "Два владельца не уникальны", tests_passed, tests_failed);
                if (second) {second->SetValue(20);}
                check_shrd(first && first->GetValue() == 20, "Изменение объекта видно через другую копию", tests_passed, tests_failed);
                {
                    ShrdPtr<TestObject> third(second);
                    check_shrd(first.UseCount() == 3 && third.UseCount() == 3, "Третий владелец", tests_passed, tests_failed);
                }
                check_shrd(first.UseCount() == 2 && TestObject::GetAliveCount() == 1, "Удаление третьего владельца не удаляет объект", tests_passed, tests_failed);
            }
            check_shrd(first.UseCount() == 1 && first.unique(), "После удаления копий остается один владелец", tests_passed, tests_failed);
            check_shrd(first && first->GetValue() == 20, "Оставшийся владелец сохраняет доступ", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Копирующий конструктор и время жизни: все объекты уничтожены", tests_passed, tests_failed);

        // 5. Копирование пустого
        {
            const ShrdPtr<TestObject> first;
            ShrdPtr<TestObject> second(first);
            check_shrd(!first && !second && second.UseCount() == 0, "Конструктор копирования пустого указателя", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Копирование пустого: все объекты уничтожены", tests_passed, tests_failed);

        // 6. Копирующее присваивание разных групп
        {
            UnqPtr<TestObject> a(new TestObject(10));
            UnqPtr<TestObject> b(new TestObject(20));
            ShrdPtr<TestObject> first(std::move(a));
            ShrdPtr<TestObject> second(std::move(b));
            ShrdPtr<TestObject>& Result = (second = first);
            check_shrd(&Result == &second, "Копирующее присваивание возвращает получателя", tests_passed, tests_failed);
            check_shrd(first.get() == second.get() && first.UseCount() == 2, "Присваивание объединяет владение", tests_passed, tests_failed);
            check_shrd(TestObject::GetAliveCount() == 1, "Присваивание удаляет прежний единолично удерживаемый объект", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Копирующее присваивание разных групп: все объекты уничтожены", tests_passed, tests_failed);

        // 7. Присваивание не удаляет объект с другим владельцем
        {
            UnqPtr<TestObject> a(new TestObject(10));
            UnqPtr<TestObject> b(new TestObject(20));
            ShrdPtr<TestObject> first(std::move(a));
            ShrdPtr<TestObject> second(std::move(b));
            ShrdPtr<TestObject> keeper(second);
            second = first;
            check_shrd(keeper && keeper->GetValue() == 20 && keeper.UseCount() == 1, "Прежний объект остается у другой копии", tests_passed, tests_failed);
            check_shrd(first.UseCount() == 2 && TestObject::GetAliveCount() == 2, "Счетчики двух групп независимы", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Присваивание не удаляет объект с другим владельцем: все объекты уничтожены", tests_passed, tests_failed);

        // 8. Самокопирование и присваивание внутри одной группы
        {
            UnqPtr<TestObject> source(new TestObject(10));
            ShrdPtr<TestObject> first(std::move(source));
            ShrdPtr<TestObject>& Alias = first;
            first = Alias;
            check_shrd(first.UseCount() == 1 && first, "Самокопирование единственного владельца", tests_passed, tests_failed);
            ShrdPtr<TestObject> second(first);
            first = second;
            check_shrd(first.UseCount() == 2 && second.UseCount() == 2, "Присваивание разных владельцев одной группы", tests_passed, tests_failed);
            first = Alias;
            check_shrd(first.UseCount() == 2, "Самокопирование при совместном владении", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Самокопирование и присваивание внутри одной группы: все объекты уничтожены", tests_passed, tests_failed);

        // 9. Копирующее присваивание с пустыми указателями
        {
            UnqPtr<TestObject> source(new TestObject(10));
            ShrdPtr<TestObject> first(std::move(source));
            ShrdPtr<TestObject> second;
            second = first;
            check_shrd(first.UseCount() == 2 && second.get() == first.get(), "Копирование в пустой указатель", tests_passed, tests_failed);
            ShrdPtr<TestObject> empty;
            second = empty;
            check_shrd(!second && second.UseCount() == 0 && first.UseCount() == 1, "Присваивание пустого одной из копий", tests_passed, tests_failed);
            first = empty;
            check_shrd(!first && TestObject::GetAliveCount() == 0, "Присваивание пустого последнему владельцу", tests_passed, tests_failed);
            first = second;
            ShrdPtr<TestObject>& Alias = first;
            first = Alias;
            check_shrd(!first && first.UseCount() == 0, "Пустое присваивание и самокопирование", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Копирующее присваивание с пустыми указателями: все объекты уничтожены", tests_passed, tests_failed);

        // 10. Перемещающий конструктор
        {
            UnqPtr<TestObject> source(new TestObject(10));
            ShrdPtr<TestObject> first(std::move(source));
            ShrdPtr<TestObject> keeper(first);
            TestObject* RawPointer = first.get();
            ShrdPtr<TestObject> second(std::move(first));
            check_shrd(!first && first.UseCount() == 0 && !first.unique(), "Перемещенный источник пуст", tests_passed, tests_failed);
            check_shrd(second.get() == RawPointer && second.UseCount() == 2 && keeper.UseCount() == 2, "Move сохраняет адрес и количество владельцев", tests_passed, tests_failed);
            ShrdPtr<TestObject> third(std::move(first));
            check_shrd(!third && third.UseCount() == 0, "Перемещающий конструктор из пустого", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Перемещающий конструктор: все объекты уничтожены", tests_passed, tests_failed);

        // 11. Перемещающее присваивание разных групп
        {
            UnqPtr<TestObject> a(new TestObject(10));
            UnqPtr<TestObject> b(new TestObject(20));
            ShrdPtr<TestObject> first(std::move(a));
            ShrdPtr<TestObject> keeper(first);
            ShrdPtr<TestObject> second(std::move(b));
            TestObject* RawPointer = first.get();
            ShrdPtr<TestObject>& Result = (second = std::move(first));
            check_shrd(&Result == &second && !first && first.UseCount() == 0, "Move-присваивание возвращает получателя и очищает источник", tests_passed, tests_failed);
            check_shrd(second.get() == RawPointer && second.UseCount() == 2 && keeper.UseCount() == 2, "Move не увеличивает счетчик группы", tests_passed, tests_failed);
            check_shrd(TestObject::GetAliveCount() == 1, "Move удаляет прежний объект получателя", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Перемещающее присваивание разных групп: все объекты уничтожены", tests_passed, tests_failed);

        // 12. Перемещение внутри группы и самоперемещение
        {
            UnqPtr<TestObject> source(new TestObject(10));
            ShrdPtr<TestObject> first(std::move(source));
            ShrdPtr<TestObject> second(first);
            TestObject* RawPointer = first.get();
            second = std::move(first);
            check_shrd(!first && second.get() == RawPointer && second.UseCount() == 1, "Move между копиями оставляет одного владельца", tests_passed, tests_failed);
            ShrdPtr<TestObject>& Alias = second;
            second = std::move(Alias);
            check_shrd(second.get() == RawPointer && second.UseCount() == 1, "Самоперемещение сохраняет владение", tests_passed, tests_failed);
            second.reset();
            second = std::move(Alias);
            check_shrd(!second && second.UseCount() == 0, "Самоперемещение пустого", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Перемещение внутри группы и самоперемещение: все объекты уничтожены", tests_passed, tests_failed);

        // 13. Перемещающее присваивание с пустыми указателями
        {
            UnqPtr<TestObject> source(new TestObject(10));
            ShrdPtr<TestObject> first(std::move(source));
            ShrdPtr<TestObject> second;
            second = std::move(first);
            check_shrd(!first && second.UseCount() == 1, "Move в пустого получателя", tests_passed, tests_failed);
            second = std::move(first);
            check_shrd(!second && TestObject::GetAliveCount() == 0, "Move из пустого освобождает получателя", tests_passed, tests_failed);
            second = std::move(first);
            check_shrd(!second && second.UseCount() == 0, "Move пустого в пустой", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Перемещающее присваивание с пустыми указателями: все объекты уничтожены", tests_passed, tests_failed);

        // 14. reset без аргумента
        {
            UnqPtr<TestObject> source(new TestObject(10));
            ShrdPtr<TestObject> first(std::move(source));
            ShrdPtr<TestObject> second(first);
            first.reset();
            check_shrd(!first && first.UseCount() == 0 && second.UseCount() == 1, "reset отказывается только от своего владения", tests_passed, tests_failed);
            check_shrd(second && second->GetValue() == 10, "Другой владелец продолжает работать", tests_passed, tests_failed);
            second.reset();
            check_shrd(!second && TestObject::GetAliveCount() == 0, "reset последнего владельца удаляет объект", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "reset без аргумента: все объекты уничтожены", tests_passed, tests_failed);

        // 15. reset из UnqPtr
        {
            UnqPtr<TestObject> a(new TestObject(10));
            ShrdPtr<TestObject> first(std::move(a));
            ShrdPtr<TestObject> keeper(first);
            UnqPtr<TestObject> b(new TestObject(20));
            TestObject* RawPointer = b.get();
            first.reset(std::move(b));
            check_shrd(!b && first.get() == RawPointer && first.UseCount() == 1, "reset из UnqPtr создает новую группу", tests_passed, tests_failed);
            check_shrd(keeper.UseCount() == 1 && keeper && keeper->GetValue() == 10, "reset не уничтожает объект другой копии", tests_passed, tests_failed);
            UnqPtr<TestObject> c(new TestObject(30));
            first.reset(std::move(c));
            check_shrd(!c && first && first->GetValue() == 30 && TestObject::GetAliveCount() == 2, "reset заменяет единолично удерживаемый объект", tests_passed, tests_failed);
            UnqPtr<TestObject> empty;
            first.reset(std::move(empty));
            check_shrd(!first && first.UseCount() == 0 && TestObject::GetAliveCount() == 1, "reset из пустого UnqPtr очищает владельца", tests_passed, tests_failed);
            first.reset(std::move(empty));
            check_shrd(!first, "reset пустого владельца из пустого UnqPtr", tests_passed, tests_failed);
            UnqPtr<TestObject> d(new TestObject(40));
            first.reset(std::move(d));
            check_shrd(!d && first.UseCount() == 1, "reset заполняет пустого владельца", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "reset из UnqPtr: все объекты уничтожены", tests_passed, tests_failed);

        // 16. Обмен группами
        {
            UnqPtr<TestObject> a(new TestObject(10));
            UnqPtr<TestObject> b(new TestObject(20));
            ShrdPtr<TestObject> first(std::move(a));
            ShrdPtr<TestObject> second(std::move(b));
            ShrdPtr<TestObject> keeper(first);
            TestObject* FirstPointer = first.get();
            TestObject* SecondPointer = second.get();
            first.swap(second);
            check_shrd(first.get() == SecondPointer && first.UseCount() == 1, "swap передает адрес вместе со счетчиком", tests_passed, tests_failed);
            check_shrd(second.get() == FirstPointer && second.UseCount() == 2 && keeper.UseCount() == 2, "swap сохраняет связи с другими владельцами", tests_passed, tests_failed);
            swap(first, second);
            check_shrd(first.get() == FirstPointer && first.UseCount() == 2, "Свободная функция swap", tests_passed, tests_failed);
            first.swap(first);
            check_shrd(first.get() == FirstPointer && first.UseCount() == 2, "Самообмен", tests_passed, tests_failed);
            first.swap(keeper);
            check_shrd(first.UseCount() == 2 && keeper.UseCount() == 2, "Обмен внутри одной группы", tests_passed, tests_failed);
            ShrdPtr<TestObject> empty;
            first.swap(empty);
            check_shrd(!first && first.UseCount() == 0 && empty.UseCount() == 2, "Обмен с пустым владельцем", tests_passed, tests_failed);
            ShrdPtr<TestObject> another;
            first.swap(another);
            check_shrd(!first && !another, "Обмен двух пустых владельцев", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Обмен группами: все объекты уничтожены", tests_passed, tests_failed);

        // 17. Наследование: получение из уникального владельца
        {
            UnqPtr<ShrdTestDerived> source(new ShrdTestDerived);
            ShrdTestBase* RawPointer = source.get();
            ShrdPtr<ShrdTestBase> pointer(std::move(source));
            check_shrd(!source && pointer.get() == RawPointer && pointer.UseCount() == 1, "Конструктор из UnqPtr<Derived>", tests_passed, tests_failed);
            check_shrd(pointer && pointer->GetKind() == 2, "Виртуальный метод через Base", tests_passed, tests_failed);
            UnqPtr<ShrdTestDerived> empty;
            ShrdPtr<ShrdTestBase> second(std::move(empty));
            check_shrd(!second && second.UseCount() == 0, "Преобразующий конструктор из пустого UnqPtr", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Наследование: получение из уникального владельца: все объекты уничтожены", tests_passed, tests_failed);

        // 18. Наследование: копирующий и перемещающий конструкторы
        {
            UnqPtr<ShrdTestDerived> source(new ShrdTestDerived);
            ShrdPtr<ShrdTestDerived> first(std::move(source));
            ShrdTestBase* RawPointer = first.get();
            ShrdPtr<ShrdTestBase> second(first);
            check_shrd(second.get() == RawPointer && second.UseCount() == 2 && first.UseCount() == 2, "Копирующий конструктор Derived -> Base", tests_passed, tests_failed);
            ShrdPtr<ShrdTestBase> third(std::move(first));
            check_shrd(!first && first.UseCount() == 0 && third.UseCount() == 2, "Move-конструктор Derived -> Base", tests_passed, tests_failed);
            second.reset();
            check_shrd(third.unique() && third && third->GetKind() == 2, "Последний владелец Base сохраняет дочерний объект", tests_passed, tests_failed);
            ShrdPtr<ShrdTestDerived> empty;
            ShrdPtr<ShrdTestBase> fourth(empty);
            ShrdPtr<ShrdTestBase> fifth(std::move(empty));
            check_shrd(!fourth && !fifth && fourth.UseCount() == 0 && fifth.UseCount() == 0, "Преобразующие конструкторы из пустого ShrdPtr", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Наследование: копирующий и перемещающий конструкторы: все объекты уничтожены", tests_passed, tests_failed);

        // 19. Наследование: присваивания
        {
            UnqPtr<ShrdTestDerived> a(new ShrdTestDerived);
            UnqPtr<ShrdTestBase> b(new ShrdTestBase);
            ShrdPtr<ShrdTestDerived> first(std::move(a));
            ShrdPtr<ShrdTestBase> second(std::move(b));
            ShrdPtr<ShrdTestBase>& Result = (second = first);
            check_shrd(&Result == &second && first.UseCount() == 2 && second.UseCount() == 2, "Копирующее присваивание Derived -> Base", tests_passed, tests_failed);
            check_shrd(TestObject::GetAliveCount() == 2, "Прежний Base удален", tests_passed, tests_failed);
            second = first;
            check_shrd(first.UseCount() == 2 && second.UseCount() == 2, "Преобразующее копирование внутри одной группы", tests_passed, tests_failed);
            ShrdPtr<ShrdTestBase>& MoveResult = (second = std::move(first));
            check_shrd(&MoveResult == &second && !first && second.UseCount() == 1, "Преобразующее move внутри одной группы", tests_passed, tests_failed);
            ShrdPtr<ShrdTestDerived> empty;
            second = empty;
            check_shrd(!second && TestObject::GetAliveCount() == 0, "Преобразующее присваивание пустого", tests_passed, tests_failed);
            second = std::move(empty);
            check_shrd(!second && second.UseCount() == 0, "Преобразующее move двух пустых", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Наследование: присваивания: все объекты уничтожены", tests_passed, tests_failed);

        // 20. Наследование: move между разными группами
        {
            UnqPtr<ShrdTestDerived> a(new ShrdTestDerived);
            UnqPtr<ShrdTestBase> b(new ShrdTestBase);
            ShrdPtr<ShrdTestDerived> first(std::move(a));
            ShrdPtr<ShrdTestBase> second(std::move(b));
            second = std::move(first);
            check_shrd(!first && second.UseCount() == 1 && TestObject::GetAliveCount() == 2, "Преобразующее move заменяет прежний объект", tests_passed, tests_failed);
            ShrdPtr<ShrdTestDerived> empty;
            second = std::move(empty);
            check_shrd(!second && TestObject::GetAliveCount() == 0, "Преобразующее move из пустого освобождает объект", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Наследование: move между разными группами: все объекты уничтожены", tests_passed, tests_failed);

        // 21. Наследование: reset
        {
            UnqPtr<ShrdTestBase> a(new ShrdTestBase);
            ShrdPtr<ShrdTestBase> pointer(std::move(a));
            UnqPtr<ShrdTestDerived> b(new ShrdTestDerived);
            pointer.reset(std::move(b));
            check_shrd(!b && pointer.UseCount() == 1 && pointer && pointer->GetKind() == 2, "reset из UnqPtr<Derived>", tests_passed, tests_failed);
            check_shrd(TestObject::GetAliveCount() == 2, "reset удаляет старый Base", tests_passed, tests_failed);
            UnqPtr<ShrdTestDerived> empty;
            pointer.reset(std::move(empty));
            check_shrd(!pointer && pointer.UseCount() == 0, "Преобразующий reset из пустого UnqPtr", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Наследование: reset: все объекты уничтожены", tests_passed, tests_failed);

        // 22. Добавление const
        {
            UnqPtr<TestObject> source(new TestObject(10));
            ShrdPtr<TestObject> first(std::move(source));
            ShrdPtr<const TestObject> second(first);
            check_shrd(second && second->GetValue() == 10 && first.UseCount() == 2, "Копирование T -> const T", tests_passed, tests_failed);
            ShrdPtr<const TestObject> third(std::move(first));
            check_shrd(!first && second.UseCount() == 2 && third.UseCount() == 2, "Перемещение T -> const T", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Добавление const: все объекты уничтожены", tests_passed, tests_failed);

        // 23. Пустой массив
        {
            ShrdPtr<TestObject[]> pointer;
            check_shrd(!pointer && pointer.get() == nullptr && pointer.UseCount() == 0 && !pointer.unique(), "Пустой совместный владелец массива", tests_passed, tests_failed);
            pointer.reset();
            UnqPtr<TestObject[]> source;
            ShrdPtr<TestObject[]> second(std::move(source));
            check_shrd(!second && second.UseCount() == 0, "Создание из пустого UnqPtr массива", tests_passed, tests_failed);
            ShrdPtr<TestObject[]> third(second);
            ShrdPtr<TestObject[]> fourth(std::move(second));
            check_shrd(!third && !fourth && third.UseCount() == 0 && fourth.UseCount() == 0, "Копирование и перемещение пустого массива", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Пустой массив: все объекты уничтожены", tests_passed, tests_failed);

        // 24. Массив: создание, индексация и совместное владение
        {
            UnqPtr<TestObject[]> source(new TestObject[3]);
            TestObject* RawPointer = source.get();
            ShrdPtr<TestObject[]> first(std::move(source));
            check_shrd(!source && first.get() == RawPointer && first.unique(), "Получение массива из UnqPtr", tests_passed, tests_failed);
            if (first) {
                first[0].SetValue(10);
                first[2].SetValue(30);
            }
            check_shrd(first && first[0].GetValue() == 10 && first[2].GetValue() == 30, "Индексация первого и последнего элемента", tests_passed, tests_failed);
            {
                ShrdPtr<TestObject[]> second(first);
                check_shrd(first.UseCount() == 2 && second.get() == RawPointer && !first.unique(), "Копирование владельца массива", tests_passed, tests_failed);
                const ShrdPtr<TestObject[]>& ConstSecond = second;
                if (ConstSecond) {ConstSecond[1].SetValue(20);}
                check_shrd(first && first[1].GetValue() == 20, "Изменение через const копию видно у другого владельца", tests_passed, tests_failed);
                ShrdPtr<TestObject[]> third(second);
                check_shrd(first.UseCount() == 3, "Третий владелец массива", tests_passed, tests_failed);
            }
            check_shrd(first.UseCount() == 1 && TestObject::GetAliveCount() == 3, "Удаление копий не удаляет элементы", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Массив: создание, индексация и совместное владение: все объекты уничтожены", tests_passed, tests_failed);

        // 25. Массив: копирующее присваивание
        {
            UnqPtr<TestObject[]> a(new TestObject[3]);
            UnqPtr<TestObject[]> b(new TestObject[2]);
            ShrdPtr<TestObject[]> first(std::move(a));
            ShrdPtr<TestObject[]> second(std::move(b));
            ShrdPtr<TestObject[]>& Result = (second = first);
            check_shrd(&Result == &second && first.get() == second.get() && first.UseCount() == 2, "Копирующее присваивание массива", tests_passed, tests_failed);
            check_shrd(TestObject::GetAliveCount() == 3, "Присваивание удаляет прежние элементы", tests_passed, tests_failed);
            second = first;
            check_shrd(first.UseCount() == 2, "Копирование внутри одной группы массива", tests_passed, tests_failed);
            ShrdPtr<TestObject[]>& Alias = first;
            first = Alias;
            check_shrd(first.UseCount() == 2, "Самокопирование массива", tests_passed, tests_failed);
            ShrdPtr<TestObject[]> empty;
            second = empty;
            check_shrd(!second && first.UseCount() == 1, "Присваивание пустого одной копии массива", tests_passed, tests_failed);
            first = empty;
            check_shrd(!first && TestObject::GetAliveCount() == 0, "Присваивание пустого последнему владельцу массива", tests_passed, tests_failed);
            first = second;
            first = Alias;
            check_shrd(!first && first.UseCount() == 0, "Присваивание и самокопирование пустого массива", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Массив: копирующее присваивание: все объекты уничтожены", tests_passed, tests_failed);

        // 26. Массив: копирование с сохранением старой группы
        {
            UnqPtr<TestObject[]> a(new TestObject[3]);
            UnqPtr<TestObject[]> b(new TestObject[2]);
            ShrdPtr<TestObject[]> first(std::move(a));
            ShrdPtr<TestObject[]> second(std::move(b));
            ShrdPtr<TestObject[]> keeper(second);
            second = first;
            check_shrd(keeper.UseCount() == 1 && TestObject::GetAliveCount() == 5, "Старая группа массива остается у другой копии", tests_passed, tests_failed);
            ShrdPtr<TestObject[]> empty;
            empty = first;
            check_shrd(empty.get() == first.get() && first.UseCount() == 3, "Копирование массива пустому получателю", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Массив: копирование с сохранением старой группы: все объекты уничтожены", tests_passed, tests_failed);

        // 27. Массив: move-конструктор
        {
            UnqPtr<TestObject[]> source(new TestObject[3]);
            ShrdPtr<TestObject[]> first(std::move(source));
            ShrdPtr<TestObject[]> keeper(first);
            TestObject* RawPointer = first.get();
            ShrdPtr<TestObject[]> second(std::move(first));
            check_shrd(!first && first.UseCount() == 0, "Move-конструктор массива очищает источник", tests_passed, tests_failed);
            check_shrd(second.get() == RawPointer && second.UseCount() == 2 && keeper.UseCount() == 2, "Move массива сохраняет счетчик", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Массив: move-конструктор: все объекты уничтожены", tests_passed, tests_failed);

        // 28. Массив: move-присваивание
        {
            UnqPtr<TestObject[]> a(new TestObject[3]);
            UnqPtr<TestObject[]> b(new TestObject[2]);
            ShrdPtr<TestObject[]> first(std::move(a));
            ShrdPtr<TestObject[]> keeper(first);
            ShrdPtr<TestObject[]> second(std::move(b));
            TestObject* RawPointer = first.get();
            ShrdPtr<TestObject[]>& Result = (second = std::move(first));
            check_shrd(&Result == &second && !first && first.UseCount() == 0, "Move-присваивание массива очищает источник", tests_passed, tests_failed);
            check_shrd(second.get() == RawPointer && second.UseCount() == 2 && TestObject::GetAliveCount() == 3, "Move-присваивание удаляет старый массив", tests_passed, tests_failed);
            second = std::move(keeper);
            check_shrd(!keeper && second.UseCount() == 1, "Move внутри группы массива", tests_passed, tests_failed);
            ShrdPtr<TestObject[]>& Alias = second;
            second = std::move(Alias);
            check_shrd(second.get() == RawPointer && second.UseCount() == 1, "Самоперемещение массива", tests_passed, tests_failed);
            second.reset();
            second = std::move(Alias);
            check_shrd(!second && second.UseCount() == 0, "Самоперемещение пустого массива", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Массив: move-присваивание: все объекты уничтожены", tests_passed, tests_failed);

        // 29. Массив: move с пустыми владельцами
        {
            UnqPtr<TestObject[]> source(new TestObject[3]);
            ShrdPtr<TestObject[]> first(std::move(source));
            ShrdPtr<TestObject[]> second;
            second = std::move(first);
            check_shrd(!first && second.UseCount() == 1, "Move массива в пустой указатель", tests_passed, tests_failed);
            second = std::move(first);
            check_shrd(!second && TestObject::GetAliveCount() == 0, "Move из пустого удаляет массив", tests_passed, tests_failed);
            second = std::move(first);
            check_shrd(!second && second.UseCount() == 0, "Move двух пустых массивов", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Массив: move с пустыми владельцами: все объекты уничтожены", tests_passed, tests_failed);

        // 30. Массив: reset без аргумента
        {
            UnqPtr<TestObject[]> source(new TestObject[3]);
            ShrdPtr<TestObject[]> first(std::move(source));
            ShrdPtr<TestObject[]> second(first);
            first.reset();
            check_shrd(!first && first.UseCount() == 0 && second.UseCount() == 1, "reset массива уменьшает счетчик", tests_passed, tests_failed);
            check_shrd(TestObject::GetAliveCount() == 3, "reset одной копии не удаляет элементы", tests_passed, tests_failed);
            second.reset();
            second.reset();
            check_shrd(!second && TestObject::GetAliveCount() == 0, "reset последнего владельца удаляет все элементы", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Массив: reset без аргумента: все объекты уничтожены", tests_passed, tests_failed);

        // 31. Массив: reset из UnqPtr
        {
            UnqPtr<TestObject[]> a(new TestObject[3]);
            ShrdPtr<TestObject[]> first(std::move(a));
            ShrdPtr<TestObject[]> keeper(first);
            UnqPtr<TestObject[]> b(new TestObject[2]);
            TestObject* RawPointer = b.get();
            first.reset(std::move(b));
            check_shrd(!b && first.get() == RawPointer && first.UseCount() == 1, "reset создает новую группу массива", tests_passed, tests_failed);
            check_shrd(keeper.UseCount() == 1 && TestObject::GetAliveCount() == 5, "Прежний массив остается у другой копии", tests_passed, tests_failed);
            UnqPtr<TestObject[]> c(new TestObject[1]);
            first.reset(std::move(c));
            check_shrd(!c && first.UseCount() == 1 && TestObject::GetAliveCount() == 4, "reset удаляет прежний единолично удерживаемый массив", tests_passed, tests_failed);
            UnqPtr<TestObject[]> empty;
            first.reset(std::move(empty));
            check_shrd(!first && TestObject::GetAliveCount() == 3, "reset из пустого UnqPtr удаляет текущий массив", tests_passed, tests_failed);
            first.reset(std::move(empty));
            check_shrd(!first && first.UseCount() == 0, "reset пустого массива из пустого источника", tests_passed, tests_failed);
            UnqPtr<TestObject[]> d(new TestObject[2]);
            first.reset(std::move(d));
            check_shrd(!d && first.UseCount() == 1 && TestObject::GetAliveCount() == 5, "reset заполняет пустого владельца массива", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Массив: reset из UnqPtr: все объекты уничтожены", tests_passed, tests_failed);

        // 32. Массив: swap
        {
            UnqPtr<TestObject[]> a(new TestObject[3]);
            UnqPtr<TestObject[]> b(new TestObject[2]);
            ShrdPtr<TestObject[]> first(std::move(a));
            ShrdPtr<TestObject[]> second(std::move(b));
            ShrdPtr<TestObject[]> keeper(first);
            TestObject* FirstPointer = first.get();
            TestObject* SecondPointer = second.get();
            first.swap(second);
            check_shrd(first.get() == SecondPointer && first.UseCount() == 1, "swap массива переносит адрес и счетчик", tests_passed, tests_failed);
            check_shrd(second.get() == FirstPointer && second.UseCount() == 2 && keeper.UseCount() == 2, "swap массива сохраняет общую группу", tests_passed, tests_failed);
            swap(first, second);
            check_shrd(first.get() == FirstPointer && first.UseCount() == 2, "Свободный swap массива", tests_passed, tests_failed);
            first.swap(first);
            check_shrd(first.get() == FirstPointer && first.UseCount() == 2, "Самообмен массива", tests_passed, tests_failed);
            first.swap(keeper);
            check_shrd(first.UseCount() == 2 && keeper.UseCount() == 2, "Обмен внутри одной группы массива", tests_passed, tests_failed);
            ShrdPtr<TestObject[]> empty;
            first.swap(empty);
            check_shrd(!first && first.UseCount() == 0 && empty.UseCount() == 2, "Обмен массива с пустым указателем", tests_passed, tests_failed);
            ShrdPtr<TestObject[]> another;
            first.swap(another);
            check_shrd(!first && !another, "Обмен пустых владельцев массива", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Массив: swap: все объекты уничтожены", tests_passed, tests_failed);

        // 33. Многократное совместное владение
        {
            bool Correct = true;
            for (int index = 0; index < 1000; index++) {
                UnqPtr<TestObject> source(new TestObject(index));
                ShrdPtr<TestObject> first(std::move(source));
                ShrdPtr<TestObject> second(first);
                ShrdPtr<TestObject> third(second);
                first.reset();
                third = std::move(second);
                if (source || first || second || third.UseCount() != 1 || !third || third->GetValue() != index) {Correct = false;}
            }
            check_shrd(Correct, "1000 последовательностей копирования, reset и перемещения", tests_passed, tests_failed);
        }
        check_shrd(TestObject::GetAliveCount() == 0, "Многократное совместное владение: все объекты уничтожены", tests_passed, tests_failed);

    } catch (...) {
        check_shrd(false, "Неожиданное исключение: оставшиеся сценарии не выполнены", tests_passed, tests_failed);
    }

    check_shrd(TestObject::GetAliveCount() == 0, "В конце нет живых TestObject", tests_passed, tests_failed);
    check_shrd(TestObject::GetCreatedCount() == TestObject::GetDestroyedCount(), "Число созданных и уничтоженных объектов совпадает", tests_passed, tests_failed);
    check_shrd(TestObject::GetCopyConstructorCount() == 0, "Владение не копирует сам TestObject", tests_passed, tests_failed);
    check_shrd(TestObject::GetMoveConstructorCount() == 0, "Владение не перемещает сам TestObject", tests_passed, tests_failed);
    check_shrd(TestObject::GetCopyAssignmentCount() == 0, "Не вызывается копирующее присваивание TestObject", tests_passed, tests_failed);
    check_shrd(TestObject::GetMoveAssignmentCount() == 0, "Не вызывается перемещающее присваивание TestObject", tests_passed, tests_failed);

    std::cout << "\nShrdPtr: успешно " << tests_passed
              << ", провалено " << tests_failed << "\n";

    return tests_failed;
}

// Ограничения этих функциональных тестов:
// - Не имитируют нехватку памяти при создании ReferenceCount.
// - Счетчики TestObject не обнаруживают отдельную утечку ReferenceCount:
//   для нее нужна дополнительная проверка санитайзером.
// - Не проверяют многопоточность: текущий счетчик не атомарный.
// - Не создают циклы совместного владения и не выполняют операции с UB.
// - Здесь нет преобразования массивов Derived[] -> Base[]: оно запрещено.

#endif // SHRD_PTR_TESTS_HPP
