#ifndef WEAK_PTR_H
#define WEAK_PTR_H

#include "ShrdPtr.hpp"

template <typename T>
class WeakPtr {
private:
    T* pointer = nullptr;
    ShrdPtrControlBlock* ReferenceCount = nullptr;

    template <typename U>
    friend class WeakPtr;

    void ReleaseCurrentOwnership() noexcept{
        ShrdPtrControlBlock* TempReferenceCount = ReferenceCount;
        pointer = nullptr;
        ReferenceCount = nullptr;

        if (TempReferenceCount == nullptr) {return;}

        --(TempReferenceCount->WeakCount);
        if (TempReferenceCount->WeakCount == 0) {
            delete TempReferenceCount;
        }
    }

public:
    WeakPtr() noexcept = default;

    WeakPtr(const ShrdPtr<T>& other) noexcept: pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount) {(ReferenceCount->WeakCount)++;}
    }

    WeakPtr(const WeakPtr& other) noexcept: pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount) {(ReferenceCount->WeakCount)++;}
    }

    template <typename U>
    requires std::convertible_to<U*, T*>
    WeakPtr(const ShrdPtr<U>& other) noexcept: ReferenceCount(other.ReferenceCount){
        // Преобразую адрес только пока объект существует
        if (other.UseCount() != 0) {pointer = other.pointer;}
        if (ReferenceCount) {(ReferenceCount->WeakCount)++;}
    }

    template <typename U>
    requires std::convertible_to<U*, T*>
    WeakPtr(const WeakPtr<U>& other) noexcept: ReferenceCount(other.ReferenceCount){
        // Преобразую адрес только пока объект существует
        if (other.UseCount() != 0) {pointer = other.pointer;}
        if (ReferenceCount) {(ReferenceCount->WeakCount)++;}
    }

    WeakPtr(WeakPtr&& other) noexcept: pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        other.pointer = nullptr;
        other.ReferenceCount = nullptr;
    }

    template <typename U>
    requires std::convertible_to<U*, T*>
    WeakPtr(WeakPtr<U>&& other) noexcept: ReferenceCount(other.ReferenceCount){
        if (other.UseCount() != 0) {pointer = other.pointer;}
        other.pointer = nullptr;
        other.ReferenceCount = nullptr;
    }

    WeakPtr& operator=(const WeakPtr& other) noexcept{
        if (this == &other) {return *this;}

        T* NewPointer = other.pointer;
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->WeakCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    template <typename U>
    requires std::convertible_to<U*, T*>
    WeakPtr& operator=(const WeakPtr<U>& other) noexcept{
        T* NewPointer = nullptr;
        if (other.UseCount() != 0) {NewPointer = other.pointer;}
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->WeakCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    WeakPtr& operator=(const ShrdPtr<T>& other) noexcept{
        T* NewPointer = other.pointer;
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->WeakCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    template <typename U>
    requires std::convertible_to<U*, T*>
    WeakPtr& operator=(const ShrdPtr<U>& other) noexcept{
        T* NewPointer = nullptr;
        if (other.UseCount() != 0) {NewPointer = other.pointer;}
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->WeakCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) noexcept{
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

    template <typename U>
    requires std::convertible_to<U*, T*>
    WeakPtr& operator=(WeakPtr<U>&& other) noexcept{
        T* NewPointer = nullptr;
        if (other.UseCount() != 0) {NewPointer = other.pointer;}
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        other.pointer = nullptr;
        other.ReferenceCount = nullptr;

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    ~WeakPtr() noexcept{
        ReleaseCurrentOwnership();
    }

    std::size_t UseCount() const noexcept{
        if (ReferenceCount == nullptr) {return 0;}
        return ReferenceCount->StrongCount;
    }

    bool expired() const noexcept{
        return UseCount() == 0;
    }

    [[nodiscard]] ShrdPtr<T> lock() const noexcept{
        ShrdPtr<T> NewPointer;
        if (expired()) {return NewPointer;}

        (ReferenceCount->StrongCount)++;
        NewPointer.pointer = pointer;
        NewPointer.ReferenceCount = ReferenceCount;

        return NewPointer;
    }

    void reset() noexcept{
        ReleaseCurrentOwnership();
    }

    void swap(WeakPtr& other) noexcept{
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
class WeakPtr<T[]> {
private:
    T* pointer = nullptr;
    ShrdPtrControlBlock* ReferenceCount = nullptr;

    template <typename U>
    friend class WeakPtr;

    void ReleaseCurrentOwnership() noexcept{
        ShrdPtrControlBlock* TempReferenceCount = ReferenceCount;
        pointer = nullptr;
        ReferenceCount = nullptr;

        if (TempReferenceCount == nullptr) {return;}

        --(TempReferenceCount->WeakCount);
        if (TempReferenceCount->WeakCount == 0) {
            delete TempReferenceCount;
        }
    }

public:
    WeakPtr() noexcept = default;

    WeakPtr(const ShrdPtr<T[]>& other) noexcept: pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount) {(ReferenceCount->WeakCount)++;}
    }

    WeakPtr(const WeakPtr& other) noexcept: pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        if (ReferenceCount) {(ReferenceCount->WeakCount)++;}
    }

    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    WeakPtr(const ShrdPtr<U[]>& other) noexcept: ReferenceCount(other.ReferenceCount){
        // Преобразую адрес только пока объект существует.
        if (other.UseCount() != 0) {pointer = other.pointer;}
        if (ReferenceCount) {(ReferenceCount->WeakCount)++;}
    }

    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    WeakPtr(const WeakPtr<U[]>& other) noexcept: ReferenceCount(other.ReferenceCount){
        // Преобразую адрес только пока объект существует.
        if (other.UseCount() != 0) {pointer = other.pointer;}
        if (ReferenceCount) {(ReferenceCount->WeakCount)++;}
    }

    WeakPtr(WeakPtr&& other) noexcept: pointer(other.pointer), ReferenceCount(other.ReferenceCount){
        other.pointer = nullptr;
        other.ReferenceCount = nullptr;
    }

    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    WeakPtr(WeakPtr<U[]>&& other) noexcept: ReferenceCount(other.ReferenceCount){
        if (other.UseCount() != 0) {pointer = other.pointer;}
        other.pointer = nullptr;
        other.ReferenceCount = nullptr;
    }

    WeakPtr& operator=(const WeakPtr& other) noexcept{
        if (this == &other) {return *this;}

        T* NewPointer = other.pointer;
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->WeakCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    WeakPtr& operator=(const WeakPtr<U[]>& other) noexcept{
        T* NewPointer = nullptr;
        if (other.UseCount() != 0) {NewPointer = other.pointer;}
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->WeakCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    WeakPtr& operator=(const ShrdPtr<T[]>& other) noexcept{
        T* NewPointer = other.pointer;
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->WeakCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    WeakPtr& operator=(const ShrdPtr<U[]>& other) noexcept{
        T* NewPointer = nullptr;
        if (other.UseCount() != 0) {NewPointer = other.pointer;}
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        if (NewReferenceCount) {(NewReferenceCount->WeakCount)++;}

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) noexcept{
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

    template <typename U>
    requires std::convertible_to<U(*)[], T(*)[]>
    WeakPtr& operator=(WeakPtr<U[]>&& other) noexcept{
        T* NewPointer = nullptr;
        if (other.UseCount() != 0) {NewPointer = other.pointer;}
        ShrdPtrControlBlock* NewReferenceCount = other.ReferenceCount;

        other.pointer = nullptr;
        other.ReferenceCount = nullptr;

        ReleaseCurrentOwnership();

        pointer = NewPointer;
        ReferenceCount = NewReferenceCount;

        return *this;
    }

    ~WeakPtr() noexcept{
        ReleaseCurrentOwnership();
    }

    std::size_t UseCount() const noexcept{
        if (ReferenceCount == nullptr) {return 0;}
        return ReferenceCount->StrongCount;
    }

    bool expired() const noexcept{
        return UseCount() == 0;
    }

    [[nodiscard]] ShrdPtr<T[]> lock() const noexcept{
        ShrdPtr<T[]> NewPointer;
        if (expired()) {return NewPointer;}

        (ReferenceCount->StrongCount)++;
        NewPointer.pointer = pointer;
        NewPointer.ReferenceCount = ReferenceCount;

        return NewPointer;
    }

    void reset() noexcept{
        ReleaseCurrentOwnership();
    }

    void swap(WeakPtr& other) noexcept{
        T* TempPointer = pointer;
        ShrdPtrControlBlock* TempReferenceCount = ReferenceCount;

        pointer = other.pointer;
        ReferenceCount = other.ReferenceCount;

        other.pointer = TempPointer;
        other.ReferenceCount = TempReferenceCount;
    }
};

template <typename T>
void swap(WeakPtr<T>& first, WeakPtr<T>& second) noexcept {
    first.swap(second);
}

#endif // WEAK_PTR_H
