#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H

#include <stdexcept>
#include <utility>
#include <type_traits>

#include "UnqPtr.hpp"

// Класс для представления динамического массива
template <class T> class DynamicArray {
private:
    UnqPtr<T[]> items;
    int size = 0;

public:
    // Конструктор пустого массива
    DynamicArray() noexcept = default;

    // Создаю массив заданного размера
    explicit DynamicArray(int size) : DynamicArray(nullptr, size) {}

    // Создаю массив и копирую переданные элементы
    DynamicArray(const T *items, int count) {
        if (count < 0) {
            throw std::invalid_argument("Размер массива не может быть отрицательным");
        }

        if (count == 0) {return;}

        UnqPtr<T[]> NewItems(new T[count]{});

        if (items != nullptr) {
            for (int element = 0; element < count; element++) {
                NewItems[element] = items[element];
            }
        }

        this->size = count;
        this->items = std::move(NewItems);
    }

    // Конструктор копирования
    DynamicArray(const DynamicArray<T> &dynamic_array) {
        int NewSize = dynamic_array.size;
        UnqPtr<T[]> NewItems(NewSize > 0 ? new T[NewSize]{} : nullptr);

        for (int element = 0; element < NewSize; element++) {
            NewItems[element] = dynamic_array.items[element];
        }

        size = NewSize;
        items = std::move(NewItems);
    }

    // Копирующее присваивание
    DynamicArray<T> &operator=(const DynamicArray<T> &dynamic_array) {
        if (this == &dynamic_array) {return *this;}

        int NewSize = dynamic_array.size;
        UnqPtr<T[]> NewItems(NewSize > 0 ? new T[NewSize]{} : nullptr);

        for (int element = 0; element < NewSize; element++) {
            NewItems[element] = dynamic_array.items[element];
        }

        size = NewSize;
        items = std::move(NewItems);

        return *this;
    }

    // Перемещающий конструктор
    DynamicArray(DynamicArray<T> &&dynamic_array) noexcept: items(std::move(dynamic_array.items)), size(dynamic_array.size) {
        dynamic_array.size = 0;
    }

    // Перемещающее присваивание
    DynamicArray<T> &operator=(DynamicArray<T> &&dynamic_array) noexcept {
        if (this == &dynamic_array) {return *this;}

        int NewSize = dynamic_array.size;
        UnqPtr<T[]> NewItems(std::move(dynamic_array.items));
        dynamic_array.size = 0;

        size = NewSize;
        items = std::move(NewItems);

        return *this;
    }

    // Возвращаю размер массива
    int GetSize() const noexcept {
        return size;
    }

    // Возвращаю элемент без возможности изменения
    const T &Get(int index) const {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Индекс невалиден");
        }

        return items[index];
    }

    // Возвращаю изменяемый элемент
    T &Get(int index) {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Индекс невалиден");
        }

        return items[index];
    }

    // Копирую значение в элемент
    void Set(int index, const T &value) {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Индекс невалиден");
        }

        items[index] = value;
    }

    // Перемещаю значение в элемент
    void Set(int index, T &&value) {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Индекс невалиден");
        }

        items[index] = std::move(value);
    }

    // Изменяю размер массива
    void Resize(int new_size) {
        if (new_size < 0) {
            throw std::invalid_argument("Размер массива не может быть отрицательным");
        }

        if (new_size == size) {return;}

        UnqPtr<T[]> NewItems(new_size > 0 ? new T[new_size]{} : nullptr);
        int ElementsToMove = (new_size < size) ? new_size : size;

        for (int element = 0; element < ElementsToMove; element++) {
            if constexpr (std::is_nothrow_move_assignable_v<T> || !std::is_copy_assignable_v<T>) {
                NewItems[element] = std::move(items[element]);
            } else {
                NewItems[element] = items[element];
            }
        }

        size = new_size;
        items = std::move(NewItems);
    }

    // Очищаю массив
    void Clear() noexcept {
        size = 0;
        items.reset();
    }

    // Обмениваю содержимое массивов
    void swap(DynamicArray<T> &other) noexcept {
        items.swap(other.items);

        int TempSize = size;
        size = other.size;
        other.size = TempSize;
    }

    // Память освобождается через UnqPtr
    ~DynamicArray() noexcept = default;
};

template <typename T>
void swap(DynamicArray<T> &first, DynamicArray<T> &second) noexcept {
    first.swap(second);
}

#endif // DYNAMICARRAY_H