#ifndef SHRD_PTR_H
#define SHRD_PTR_H

#include <cstddef>
#include <concepts>

#include "UnqPtr.hpp"

template <typename T>
class ShrdPtr {
private:
    T* pointer = nullptr;
    std::size_t* ReferenceCount = nullptr;

    template <typename U>
    friend class ShrdPtr;

    void ReleaseCurrentOwnership() noexcept{
        if (ReferenceCount == nullptr) {
            pointer = nullptr;
            return;
        }

        --(*ReferenceCount);

        if (*ReferenceCount == 0) {
            delete pointer;
            delete ReferenceCount;
        }

        pointer = nullptr;
        ReferenceCount = nullptr;
    }

public:
    // пустой конструктор
    ShrdPtr() noexcept = default;

    // конструктор move из уник птр
    explicit ShrdPtr(UnqPtr<T>&& other){
        if (!other) {
            return;
        }

        std::size_t* NewReferenceCount = new std::size_t(1);

        pointer = other.release();
        ReferenceCount = NewReferenceCount;
    }

    // конструктор move из уник птр дочернего типа
    template <typename U>
    requires std::convertible_to<U*, T*>
    explicit ShrdPtr(UnqPtr<U>&& other){
        if (!other) {
            return;
        }

        std::size_t* NewReferenceCount = new std::size_t(1);

        pointer = other.release();
        ReferenceCount = NewReferenceCount;
    }

    // копирование 
    ShrdPtr(const ShrdPtr& other): pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount){
            (*ReferenceCount)++;
        }
    }

    // копирование совместного владения из дочернего типа
    template <typename U>
    requires std::convertible_to<U*, T*>
    ShrdPtr(const ShrdPtr<U>& other): pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount){
            (*ReferenceCount)++;
        }
    }

    ShrdPtr& operator=(const ShrdPtr& other){
        if (this == &other) {return *this;}

        T* NewPointer = other.pointer;
        std::size_t* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(*NewReferenceCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    // копирование  владения из дочернего типа
    template <typename U>
    requires std::convertible_to<U*, T*>
    ShrdPtr& operator=(const ShrdPtr<U>& other){
        T* NewPointer = other.pointer;
        std::size_t* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(*NewReferenceCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    // перемещение  владельца
    ShrdPtr(ShrdPtr&& other) noexcept: pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        other.pointer = nullptr;
        other.ReferenceCount = nullptr;
    }

    // перемещение  дочернего типа
    template <typename U>
    requires std::convertible_to<U*, T*>
    ShrdPtr(ShrdPtr<U>&& other) noexcept: pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        other.pointer = nullptr;
        other.ReferenceCount = nullptr;
    }

    ShrdPtr& operator=(ShrdPtr&& other) noexcept{
        if (this == &other) {return *this;}

        T* NewPointer = other.pointer;
        std::size_t* NewReferenceCount = other.ReferenceCount;

        other.pointer = nullptr;
        other.ReferenceCount = nullptr;

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    // перемещение  владельца дочернего типа
    template <typename U>
    requires std::convertible_to<U*, T*>
    ShrdPtr& operator=(ShrdPtr<U>&& other) noexcept{
        T* NewPointer = other.pointer;
        std::size_t* NewReferenceCount = other.ReferenceCount;

        other.pointer = nullptr;
        other.ReferenceCount = nullptr;

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    // отказ от  владения / или деструктор 
    ~ShrdPtr() noexcept{
        ReleaseCurrentOwnership();
    }

    // доступ
    T* get() const noexcept{ return pointer;}
    T& operator*() const noexcept {return *pointer;}
    T* operator->() const noexcept {return pointer;}
    explicit operator bool() const noexcept { return pointer != nullptr;}
    
    // информация о владении
    std::size_t UseCount() const noexcept {
        if (ReferenceCount == nullptr) {return 0;}
        return *ReferenceCount;
    }

    bool unique() const noexcept{
        if (UseCount() == 1) {return true;}
        return false;
    }

    // управление владением
    void reset() noexcept{
        ReleaseCurrentOwnership();
    }

    void reset(UnqPtr<T>&& other){
        if (!other) {
            ReleaseCurrentOwnership();
            return;
        }

        std::size_t* NewReferenceCount = new std::size_t(1);
        T* NewPointer = other.release();

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;
    }

    // получение владения из уник птр дочернего типа
    template <typename U>
    requires std::convertible_to<U*, T*>
    void reset(UnqPtr<U>&& other){
        if (!other) {
            ReleaseCurrentOwnership();
            return;
        }

        std::size_t* NewReferenceCount = new std::size_t(1);
        T* NewPointer = other.release();

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;
    }

    void swap(ShrdPtr& other) noexcept{
        T* TempPointer = pointer;
        std::size_t* TempReferenceCount = ReferenceCount;

        pointer = other.pointer;
        ReferenceCount = other.ReferenceCount;

        other.pointer = TempPointer;
        other.ReferenceCount = TempReferenceCount;
    }
};

// специализация для массивов
template <typename T>
class ShrdPtr<T[]> {
private:
    T* pointer = nullptr;
    std::size_t* ReferenceCount = nullptr;

    template <typename U>
    friend class ShrdPtr;

    void ReleaseCurrentOwnership() noexcept{
        if (ReferenceCount == nullptr) {
            pointer = nullptr;
            return;
        }

        --(*ReferenceCount);

        if (*ReferenceCount == 0) {
            delete[] pointer;
            delete ReferenceCount;
        }

        pointer = nullptr;
        ReferenceCount = nullptr;
    }

public:
    // пустой конструктор
    ShrdPtr() noexcept = default;

    // конструктор move из уник птр массива
    explicit ShrdPtr(UnqPtr<T[]>&& other){
        if (!other) {
            return;
        }

        std::size_t* NewReferenceCount = new std::size_t(1);

        pointer = other.release();
        ReferenceCount = NewReferenceCount;
    }

    // конструктор move из уник птр массива совместимого типа
    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    explicit ShrdPtr(UnqPtr<U[]>&& other){
        if (!other) {
            return;
        }

        std::size_t* NewReferenceCount = new std::size_t(1);

        pointer = other.release();
        ReferenceCount = NewReferenceCount;
    }

    // копирование совместного владения массивом
    ShrdPtr(const ShrdPtr& other): pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount){
            (*ReferenceCount)++;
        }
    }

    // копирование совместного владения массивом совместимого типа
    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    ShrdPtr(const ShrdPtr<U[]>& other): pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount){
            (*ReferenceCount)++;
        }
    }

    ShrdPtr& operator=(const ShrdPtr& other){
        if (this == &other) {return *this;}

        T* NewPointer = other.pointer;
        std::size_t* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(*NewReferenceCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    // копирующее присваивание массива совместимого типа
    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    ShrdPtr& operator=(const ShrdPtr<U[]>& other){
        T* NewPointer = other.pointer;
        std::size_t* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(*NewReferenceCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    // перемещение совместного владельца массива
    ShrdPtr(ShrdPtr&& other) noexcept: pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        other.pointer = nullptr;
        other.ReferenceCount = nullptr;
    }

    // перемещение совместного владельца массива совместимого типа
    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    ShrdPtr(ShrdPtr<U[]>&& other) noexcept: pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        other.pointer = nullptr;
        other.ReferenceCount = nullptr;
    }

    ShrdPtr& operator=(ShrdPtr&& other) noexcept{
        if (this == &other) {return *this;}

        T* NewPointer = other.pointer;
        std::size_t* NewReferenceCount = other.ReferenceCount;

        other.pointer = nullptr;
        other.ReferenceCount = nullptr;

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    // перемещающее присваивание массива совместимого типа
    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    ShrdPtr& operator=(ShrdPtr<U[]>&& other) noexcept{
        T* NewPointer = other.pointer;
        std::size_t* NewReferenceCount = other.ReferenceCount;

        other.pointer = nullptr;
        other.ReferenceCount = nullptr;

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    // отказ от совместного владения массивом
    ~ShrdPtr() noexcept{
        ReleaseCurrentOwnership();
    }

    // доступ к массиву
    T* get() const noexcept{return pointer;}

    T& operator[](std::size_t Index) const noexcept{return pointer[Index];}

    explicit operator bool() const noexcept{return pointer != nullptr;}

    // информация о владении
    std::size_t UseCount() const noexcept{
        if (ReferenceCount == nullptr) {return 0;}

        return *ReferenceCount;
    }

    bool unique() const noexcept{
        if (UseCount() == 1) {return true;}

        return false;
    }

    // управление владением
    void reset() noexcept{
        ReleaseCurrentOwnership();
    }

    void reset(UnqPtr<T[]>&& other){
        if (!other) {
            ReleaseCurrentOwnership();
            return;
        }

        std::size_t* NewReferenceCount = new std::size_t(1);
        T* NewPointer = other.release();

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;
    }

    // получение владения массивом совместимого типа
    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    void reset(UnqPtr<U[]>&& other){
        if (!other) {
            ReleaseCurrentOwnership();
            return;
        }

        std::size_t* NewReferenceCount = new std::size_t(1);
        T* NewPointer = other.release();

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;
    }

    void swap(ShrdPtr& other) noexcept{
        T* TempPointer = pointer;
        std::size_t* TempReferenceCount = ReferenceCount;

        pointer = other.pointer;
        ReferenceCount = other.ReferenceCount;

        other.pointer = TempPointer;
        other.ReferenceCount = TempReferenceCount;
    }
};

template <typename T>
void swap(ShrdPtr<T>& first, ShrdPtr<T>& second) noexcept {
    first.swap(second);
}

#endif // SHRD_PTR_H

