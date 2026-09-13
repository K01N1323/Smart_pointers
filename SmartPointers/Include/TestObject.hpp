#ifndef TEST_OBJECT_H
#define TEST_OBJECT_H

#include <cstddef>
#include <stdexcept>

// Объект для однопоточных функциональных тестов
class TestObject {
private:
    int value = 0;

    // счетчики
    static inline std::size_t AliveCount = 0;
    static inline std::size_t CreatedCount = 0;
    static inline std::size_t DestroyedCount = 0;
    static inline std::size_t CopyConstructorCount = 0;
    static inline std::size_t MoveConstructorCount = 0;
    static inline std::size_t CopyAssignmentCount = 0;
    static inline std::size_t MoveAssignmentCount = 0;

public:
    // Создание пустого объекта, в том числе элемента массива
    TestObject() noexcept: TestObject(0) {}

    // Создание объекта с заданным значением
    explicit TestObject(int NewValue) noexcept: value(NewValue){
        AliveCount++;
        CreatedCount++;
    }

    // Копирование создает новый живой объект
    TestObject(const TestObject& other) noexcept: value(other.value){
        AliveCount++;
        CreatedCount++;
        CopyConstructorCount++;
    }

    // Перемещение создает новый объект, источник остается живым
    TestObject(TestObject&& other) noexcept: value(other.value){
        other.value = 0;

        AliveCount++;
        CreatedCount++;
        MoveConstructorCount++;
    }

    // Присваивание не создает новый объект
    TestObject& operator=(const TestObject& other) noexcept{
        CopyAssignmentCount++;

        if (this != &other){
            value = other.value;
        }

        return *this;
    }

    TestObject& operator=(TestObject&& other) noexcept{
        MoveAssignmentCount++;

        if (this != &other){
            value = other.value;
            other.value = 0;
        }

        return *this;
    }

    ~TestObject() noexcept{
        AliveCount--;
        DestroyedCount++;
    }

    // Доступ к значению
    int GetValue() const noexcept{return value;}

    void SetValue(int NewValue) noexcept{value = NewValue;}

    // Чтение счетчиков
    static std::size_t GetAliveCount() noexcept{return AliveCount;}

    static std::size_t GetCreatedCount() noexcept{return CreatedCount;}

    static std::size_t GetDestroyedCount() noexcept{return DestroyedCount;}

    static std::size_t GetCopyConstructorCount() noexcept{return CopyConstructorCount;}

    static std::size_t GetMoveConstructorCount() noexcept{return MoveConstructorCount;}

    static std::size_t GetCopyAssignmentCount() noexcept{return CopyAssignmentCount;}

    static std::size_t GetMoveAssignmentCount() noexcept{return MoveAssignmentCount;}

    // Сброс допустим только между тестами, когда живых объектов нет
    static void ResetCounters(){
        if (AliveCount != 0) {
            throw std::logic_error("Нельзя сбрасывать счетчики при наличии живых TestObject");
        }

        CreatedCount = 0;
        DestroyedCount = 0;
        CopyConstructorCount = 0;
        MoveConstructorCount = 0;
        CopyAssignmentCount = 0;
        MoveAssignmentCount = 0;
    }
};

#endif // TEST_OBJECT_H