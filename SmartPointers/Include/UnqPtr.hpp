#ifndef UNQ_PTR_H
#define UNQ_PTR_H

#include <cstddef>
#include <concepts>

template <typename T>
class UnqPtr {
private:
    T* pointer = nullptr;

public:
    // пустой конструктор 
    UnqPtr() noexcept = default;

    // Принятие владения сырым указателем
    explicit UnqPtr(T* NewPointer) noexcept : pointer(NewPointer) {}

    // Деструктор 
    ~UnqPtr() noexcept{
        delete pointer;
    }

    // запрещенное копирование 
    UnqPtr(const UnqPtr& other) = delete;
    UnqPtr& operator=(const UnqPtr& other) = delete;

    // перемещения конструктор 
    UnqPtr(UnqPtr&& other) noexcept: pointer(other.pointer) {other.pointer = nullptr;}

    template <typename U>
    requires std::convertible_to<U*, T*>
    UnqPtr(UnqPtr<U>&& other) noexcept: pointer(other.release()) {}

    UnqPtr& operator=(UnqPtr&& other) noexcept{
        if (this != &other){
            delete this->pointer;

            this->pointer = other.pointer;
            other.pointer = nullptr;

        }

        return *this;
    }

    template <typename U>
    requires std::convertible_to<U*, T*>
    UnqPtr& operator=(UnqPtr<U>&& other) noexcept{
        reset(other.release());

        return *this;
    }

    // доступ 
    T* get() const noexcept {return pointer;}

    T& operator*() const noexcept{return *pointer;}

    T* operator->() const noexcept {return pointer;}

    explicit operator bool() const noexcept {return pointer != nullptr;}

    // управление влоадением
    [[nodiscard]] T* release() noexcept{
        T* RawPointer = this->pointer;
        this->pointer = nullptr;

        return RawPointer;
    }

    void reset(T* NewPointer = nullptr) noexcept{
        if (pointer == NewPointer) {return;}

        delete pointer;

        pointer = NewPointer;
    }

    void swap(UnqPtr& other) noexcept{
        T* TempPointer = pointer;

        pointer = other.pointer;
        other.pointer = TempPointer;
    }

};

// специализация для массивов
template <typename T>
class UnqPtr<T[]> {
private:
    T* pointer = nullptr;

public:
    // пустой конструктор
    UnqPtr() noexcept = default;

    // Принятие владения сырым указателем на массив
    explicit UnqPtr(T* NewPointer) noexcept: pointer(NewPointer) {}

    // Деструктор массива
    ~UnqPtr() noexcept{
        delete[] pointer;
    }

    // запрещенное копирование
    UnqPtr(const UnqPtr& other) = delete;
    UnqPtr& operator=(const UnqPtr& other) = delete;

    // перемещения конструктор
    UnqPtr(UnqPtr&& other) noexcept: pointer(other.pointer) {other.pointer = nullptr;}

    UnqPtr& operator=(UnqPtr&& other) noexcept{
        if (this != &other){
            delete[] this->pointer;

            this->pointer = other.pointer;
            other.pointer = nullptr;
        }

        return *this;
    }

    // доступ
    T* get() const noexcept{return pointer;}

    T& operator[](std::size_t Index) const noexcept{return pointer[Index];}

    explicit operator bool() const noexcept{return pointer != nullptr;}

    // управление владением
    [[nodiscard]] T* release() noexcept{
        T* RawPointer = this->pointer;
        this->pointer = nullptr;

        return RawPointer;
    }

    void reset(T* NewPointer = nullptr) noexcept{
        if (pointer == NewPointer) {return;}

        delete[] pointer;

        pointer = NewPointer;
    }

    void swap(UnqPtr& other) noexcept{
        T* TempPointer = pointer;

        pointer = other.pointer;
        other.pointer = TempPointer;
    }

};

template <typename T>
void swap(UnqPtr<T>& first, UnqPtr<T>& second) noexcept {
    first.swap(second);
}


#endif // UNQ_PTR_H