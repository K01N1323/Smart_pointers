#ifndef POLYMORPHISM_TESTS_HPP
#define POLYMORPHISM_TESTS_HPP

#include <iostream>
#include <utility>

#include "UnqPtr.hpp"
#include "ShrdPtr.hpp"
#include "TestObject.hpp"


inline void check_polymorphism(bool condition, const char* test_name, int& tests_passed, int& tests_failed) {
    if (condition) {
        std::cout << "  Успешно: " << test_name << "\n";
        tests_passed++;
    } else {
        std::cout << "  Провалено: " << test_name << "\n";
        tests_failed++;
    }
}



class PolyTestBase {
private:
    TestObject value;

public:
    explicit PolyTestBase(int NewValue) noexcept: value(NewValue) {}
    virtual ~PolyTestBase() noexcept = default;
    int GetValue() const noexcept {return value.GetValue();}
    virtual int Calculate() const noexcept = 0;
};

class PolyTestDerived : public PolyTestBase {
private:
    TestObject ExtraValue;

public:
    explicit PolyTestDerived(int NewValue) noexcept: PolyTestBase(NewValue) {}
    ~PolyTestDerived() noexcept override = default;
    int Calculate() const noexcept override {return GetValue() * 2;}
};

// Три уровня наследования
class PolyTestLeaf : public PolyTestDerived {
private:
    TestObject LeafValue;

public:
    explicit PolyTestLeaf(int NewValue) noexcept: PolyTestDerived(NewValue) {}
    ~PolyTestLeaf() noexcept override = default;
    int Calculate() const noexcept override {return GetValue() * 3;}
};

class PolyTestSide {
private:
    int Marker = 77;

public:
    virtual ~PolyTestSide() noexcept = default;
    int GetMarker() const noexcept {return Marker;}
};


class PolyTestMultiple : public PolyTestSide, public PolyTestBase {
private:
    TestObject ExtraValue;

public:
    explicit PolyTestMultiple(int NewValue) noexcept: PolyTestBase(NewValue) {}
    ~PolyTestMultiple() noexcept override = default;
    int Calculate() const noexcept override {return GetValue() + GetMarker();}
};

inline int read_polymorphic_value(const PolyTestBase& Object) noexcept {
    return Object.Calculate();
}

// Возвращает число проваленных проверок.
inline int run_polymorphism_tests() {
    int tests_passed = 0;
    int tests_failed = 0;
    std::cout << "\n--- Тестирование полиморфизма ---\n";
    if (TestObject::GetAliveCount() != 0) {
        std::cout << "Нельзя начинать тесты: есть живые TestObject.\n";
        return 1;
    }
    TestObject::ResetCounters();

    try {
        // 1. UnqPtr: виртуальный вызов через абстрактный Base и ссылку.
        {
            UnqPtr<PolyTestDerived> first(new PolyTestDerived(10));
            PolyTestBase* RawPointer = first.get();
            UnqPtr<PolyTestBase> second(std::move(first));
            check_polymorphism(!first && second.get() == RawPointer, "UnqPtr: конструктор Derived -> Base", tests_passed, tests_failed);
            check_polymorphism(second && second->Calculate() == 20, "Виртуальный вызов через operator->", tests_passed, tests_failed);
            check_polymorphism(second && read_polymorphic_value(*second) == 20, "Виртуальный вызов через ссылку Base&", tests_passed, tests_failed);
            check_polymorphism(TestObject::GetAliveCount() == 2, "Дочерняя часть не потеряна при преобразовании", tests_passed, tests_failed);
        }
        check_polymorphism(TestObject::GetAliveCount() == 0, "UnqPtr удалил базовую и дочернюю части", tests_passed, tests_failed);

        // 2. UnqPtr: цепочка Leaf -> Derived -> Base и замена объекта.
        {
            UnqPtr<PolyTestLeaf> first(new PolyTestLeaf(10));
            UnqPtr<PolyTestDerived> second(std::move(first));
            UnqPtr<PolyTestBase> third(std::move(second));
            check_polymorphism(!first && !second && third && third->Calculate() == 30, "Три уровня сохраняют переопределение Leaf", tests_passed, tests_failed);
            UnqPtr<PolyTestDerived> replacement(new PolyTestDerived(7));
            third = std::move(replacement);
            check_polymorphism(!replacement && third && third->Calculate() == 14, "Преобразующее присваивание заменяет динамический тип", tests_passed, tests_failed);
            check_polymorphism(TestObject::GetAliveCount() == 2, "При замене удалены все три части Leaf", tests_passed, tests_failed);
        }
        check_polymorphism(TestObject::GetAliveCount() == 0, "После цепочки UnqPtr нет живых объектов", tests_passed, tests_failed);

        // 3. ShrdPtr: общий счетчик у владельцев разных статических типов.
        {
            UnqPtr<PolyTestLeaf> source(new PolyTestLeaf(10));
            ShrdPtr<PolyTestLeaf> first(std::move(source));
            ShrdPtr<PolyTestDerived> second(first);
            ShrdPtr<PolyTestBase> third(second);
            check_polymorphism(first.UseCount() == 3 && second.UseCount() == 3 && third.UseCount() == 3, "Leaf, Derived и Base используют один счетчик", tests_passed, tests_failed);
            check_polymorphism(third && third->Calculate() == 30, "ShrdPtr<Base> вызывает реализацию Leaf", tests_passed, tests_failed);
            second.reset();
            first.reset();
            check_polymorphism(third.unique() && TestObject::GetAliveCount() == 3, "Последний Base удерживает весь Leaf", tests_passed, tests_failed);
        }
        check_polymorphism(TestObject::GetAliveCount() == 0, "Последний ShrdPtr<Base> удалил все три части Leaf", tests_passed, tests_failed);

        // 4. ShrdPtr: преобразующие присваивания и move-конструктор.
        {
            UnqPtr<PolyTestDerived> a(new PolyTestDerived(5));
            UnqPtr<PolyTestLeaf> b(new PolyTestLeaf(7));
            ShrdPtr<PolyTestBase> first(std::move(a));
            ShrdPtr<PolyTestLeaf> second(std::move(b));
            first = second;
            check_polymorphism(first.UseCount() == 2 && first && first->Calculate() == 21, "Копирующее присваивание Leaf -> Base", tests_passed, tests_failed);
            check_polymorphism(TestObject::GetAliveCount() == 3, "Прежний Derived полностью уничтожен", tests_passed, tests_failed);
            ShrdPtr<PolyTestDerived> third(std::move(second));
            check_polymorphism(!second && third.UseCount() == 2, "Преобразующий move-конструктор не добавляет владельца", tests_passed, tests_failed);
            first = std::move(third);
            check_polymorphism(!third && first.unique() && first && first->Calculate() == 21, "Преобразующее move-присваивание внутри группы", tests_passed, tests_failed);
        }
        check_polymorphism(TestObject::GetAliveCount() == 0, "После присваиваний ShrdPtr нет живых объектов", tests_passed, tests_failed);

        // 5. ShrdPtr: reset новым дочерним объектом при наличии старой копии.
        {
            UnqPtr<PolyTestDerived> source(new PolyTestDerived(10));
            ShrdPtr<PolyTestBase> first(std::move(source));
            ShrdPtr<PolyTestBase> keeper(first);
            UnqPtr<PolyTestLeaf> replacement(new PolyTestLeaf(10));
            first.reset(std::move(replacement));
            check_polymorphism(!replacement && first.unique() && keeper.unique(), "reset разделяет старую и новую группы", tests_passed, tests_failed);
            check_polymorphism(first && first->Calculate() == 30 && keeper && keeper->Calculate() == 20, "Обе группы сохраняют свои динамические типы", tests_passed, tests_failed);
            check_polymorphism(TestObject::GetAliveCount() == 5, "Оба объекта целиком остаются живы", tests_passed, tests_failed);
            keeper.reset();
            check_polymorphism(TestObject::GetAliveCount() == 3, "Удаление старой группы не затрагивает Leaf", tests_passed, tests_failed);
        }
        check_polymorphism(TestObject::GetAliveCount() == 0, "После преобразующего reset нет живых объектов", tests_passed, tests_failed);

        // 6. Множественное наследование: адрес Base проверяем обычным приведением.
        {
            UnqPtr<PolyTestMultiple> first(new PolyTestMultiple(10));
            PolyTestBase* BasePointer = static_cast<PolyTestBase*>(first.get());
            UnqPtr<PolyTestBase> second(std::move(first));
            check_polymorphism(!first && second.get() == BasePointer, "UnqPtr корректно преобразует адрес второго базового класса", tests_passed, tests_failed);
            check_polymorphism(second && second->Calculate() == 87, "Множественное наследование: виртуальный вызов UnqPtr", tests_passed, tests_failed);
        }
        check_polymorphism(TestObject::GetAliveCount() == 0, "UnqPtr удаляет полный объект при множественном наследовании", tests_passed, tests_failed);
        {
            UnqPtr<PolyTestMultiple> source(new PolyTestMultiple(10));
            ShrdPtr<PolyTestMultiple> first(std::move(source));
            PolyTestBase* BasePointer = static_cast<PolyTestBase*>(first.get());
            ShrdPtr<PolyTestBase> second(first);
            ShrdPtr<PolyTestBase> third(std::move(first));
            check_polymorphism(!first && second.get() == BasePointer && third.get() == BasePointer, "ShrdPtr корректирует адрес при копировании и move", tests_passed, tests_failed);
            second.reset();
            check_polymorphism(third.unique() && third && third->Calculate() == 87, "Второй базовый класс остается последним владельцем", tests_passed, tests_failed);
        }
        check_polymorphism(TestObject::GetAliveCount() == 0, "ShrdPtr удаляет полный объект при множественном наследовании", tests_passed, tests_failed);

        // 7. Разные динамические типы за единым интерфейсом.
        {
            UnqPtr<PolyTestBase> Objects[3];
            Objects[0].reset(new PolyTestDerived(2));
            Objects[1].reset(new PolyTestLeaf(3));
            Objects[2].reset(new PolyTestMultiple(4));
            int Sum = 0;
            for (int index = 0; index < 3; index++) {
                if (Objects[index]) {Sum += Objects[index]->Calculate();}
            }
            check_polymorphism(Sum == 94, "Единый цикл вызывает три разные реализации", tests_passed, tests_failed);
        }
        check_polymorphism(TestObject::GetAliveCount() == 0, "Массив владельцев уничтожил все полиморфные объекты", tests_passed, tests_failed);
    } catch (...) {
        check_polymorphism(false, "Неожиданное исключение: остальные сценарии не выполнены", tests_passed, tests_failed);
    }

    check_polymorphism(TestObject::GetAliveCount() == 0 && TestObject::GetCreatedCount() == TestObject::GetDestroyedCount(), "Все созданные TestObject уничтожены", tests_passed, tests_failed);
    check_polymorphism(TestObject::GetCopyConstructorCount() == 0 && TestObject::GetMoveConstructorCount() == 0, "Передача владения не копирует и не перемещает объекты", tests_passed, tests_failed);
    check_polymorphism(TestObject::GetCopyAssignmentCount() == 0 && TestObject::GetMoveAssignmentCount() == 0, "Объектам не присваиваются чужие значения при передаче владения", tests_passed, tests_failed);
    std::cout << "\nПолиморфизм: успешно " << tests_passed << ", провалено " << tests_failed << "\n";
    return tests_failed;
}



#endif // POLYMORPHISM_TESTS_HPP
