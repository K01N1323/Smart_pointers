#ifndef SHRD_PTR_H
#define SHRD_PTR_H

#include <cstddef>
#include <concepts>

#include "UnqPtr.hpp"


struct ShrdPtrControlBlock {
    std::size_t StrongCount = 1;
    std::size_t WeakCount = 1;
};

template <typename T>
class WeakPtr;

template <typename T>
class ShrdPtr {
private:
    T* pointer = nullptr;
    ShrdPtrControlBlock* ReferenceCount = nullptr;

    template <typename U>
    friend class ShrdPtr;

    template <typename U>
    friend class WeakPtr;

    void ReleaseCurrentOwnership() noexcept{
        T* TempPointer = pointer;
        ShrdPtrControlBlock* TempReferenceCount = ReferenceCount;

        pointer = nullptr;
        ReferenceCount = nullptr;

        if (TempReferenceCount == nullptr) {return;}

        --(TempReferenceCount->StrongCount);

        if (TempReferenceCount->StrongCount == 0) {
            delete TempPointer;

     
            --(TempReferenceCount->WeakCount);
            if (TempReferenceCount->WeakCount == 0) {
                delete TempReferenceCount;
            }
        }
    }

public:
    // пустой конструктор
    ShrdPtr() noexcept = default;

    // конструктор move из уник птр
    explicit ShrdPtr(UnqPtr<T>&& other){
        if (!other) {
            return;
        }

        ShrdPtrControlBlock* NewReferenceCount = new ShrdPtrControlBlock();

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

        ShrdPtrControlBlock* NewReferenceCount = new ShrdPtrControlBlock();

        pointer = other.release();
        ReferenceCount = NewReferenceCount;
    }

    // копирование 
    ShrdPtr(const ShrdPtr& other): pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount){
            (ReferenceCount->StrongCount)++;
        }
    }

    // копирование совместного владения из дочернего типа
    template <typename U>
    requires std::convertible_to<U*, T*>
    ShrdPtr(const ShrdPtr<U>& other): pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount){
            (ReferenceCount->StrongCount)++;
        }
    }

    ShrdPtr& operator=(const ShrdPtr& other){
        if (this == &other) {return *this;}

        T* NewPointer = other.pointer;
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->StrongCount)++;}

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
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->StrongCount)++;}

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
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

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
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

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
        return ReferenceCount->StrongCount;
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

        ShrdPtrControlBlock* NewReferenceCount = new ShrdPtrControlBlock();
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

        ShrdPtrControlBlock* NewReferenceCount = new ShrdPtrControlBlock();
        T* NewPointer = other.release();

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;
    }

    void swap(ShrdPtr& other) noexcept{
        T* TempPointer = pointer;
        ShrdPtrControlBlock* TempReferenceCount = ReferenceCount;

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
    ShrdPtrControlBlock* ReferenceCount = nullptr;

    template <typename U>
    friend class ShrdPtr;

    template <typename U>
    friend class WeakPtr;

    void ReleaseCurrentOwnership() noexcept{
        T* TempPointer = pointer;
        ShrdPtrControlBlock* TempReferenceCount = ReferenceCount;

        pointer = nullptr;
        ReferenceCount = nullptr;

        if (TempReferenceCount == nullptr) {return;}

        --(TempReferenceCount->StrongCount);

        if (TempReferenceCount->StrongCount == 0) {
            delete[] TempPointer;

            // Убираю служебную ссылку после уничтожения объекта и его полей.
            --(TempReferenceCount->WeakCount);
            if (TempReferenceCount->WeakCount == 0) {
                delete TempReferenceCount;
            }
        }
    }

public:
    // пустой конструктор
    ShrdPtr() noexcept = default;

    // конструктор move из уник птр массива
    explicit ShrdPtr(UnqPtr<T[]>&& other){
        if (!other) {
            return;
        }

        ShrdPtrControlBlock* NewReferenceCount = new ShrdPtrControlBlock();

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

        ShrdPtrControlBlock* NewReferenceCount = new ShrdPtrControlBlock();

        pointer = other.release();
        ReferenceCount = NewReferenceCount;
    }

    // копирование совместного владения массивом
    ShrdPtr(const ShrdPtr& other): pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount){
            (ReferenceCount->StrongCount)++;
        }
    }

    // копирование совместного владения массивом совместимого типа
    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    ShrdPtr(const ShrdPtr<U[]>& other): pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount){
            (ReferenceCount->StrongCount)++;
        }
    }

    ShrdPtr& operator=(const ShrdPtr& other){
        if (this == &other) {return *this;}

        T* NewPointer = other.pointer;
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->StrongCount)++;}

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
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->StrongCount)++;}

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
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

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
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

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

        return ReferenceCount->StrongCount;
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

        ShrdPtrControlBlock* NewReferenceCount = new ShrdPtrControlBlock();
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

        ShrdPtrControlBlock* NewReferenceCount = new ShrdPtrControlBlock();
        T* NewPointer = other.release();

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;
    }

    void swap(ShrdPtr& other) noexcept{
        T* TempPointer = pointer;
        ShrdPtrControlBlock* TempReferenceCount = ReferenceCount;

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

