#ifndef DYNAMICARRAY_H
#define DYNAMICARRAY_H

#include <stdexcept>
#include <utility>

#include "UnqPtr.hpp"

// Класс для представления динамического массива
template <class T> class DynamicArray {
private:
    UnqPtr<T[]> items; // Умный указатель на элементы массива
    int size = 0; // Текущий размер массива

public:
    // Конструктор пустого массива
    DynamicArray() noexcept = default;

    // Конструктор: создает массив заданного размера
    explicit DynamicArray(int size) : DynamicArray(nullptr, size) {}

    // Конструктор: проверяет и инициализирует массив переданными элементами
    DynamicArray(const T *items, int count) {
        if (count < 0) {
            throw std::invalid_argument("Размер массива не может быть отрицательным");
        }

        if (count == 0) {
            return;
        }

        UnqPtr<T[]> NewItems(new T[count]{});

        if (items != nullptr) {
            for (int element = 0; element < count; element++) {
                NewItems[element] = items[element];
            }
        }

        this->items = std::move(NewItems);
        this->size = count;
    }

    // Конструктор копирования
    DynamicArray(const DynamicArray<T> &dynamic_array): items(dynamic_array.size > 0 ? new T[dynamic_array.size]{} : nullptr), size(dynamic_array.size) {
        for (int element = 0; element < size; element++) {
            items[element] = dynamic_array.items[element];
        }
    }

    // Копирующее присваивание
    DynamicArray<T> &operator=(const DynamicArray<T> &dynamic_array) {
        if (this == &dynamic_array) {return *this;}

        DynamicArray<T> TempArray(dynamic_array);
        swap(TempArray);

        return *this;
    }

    // Перемещающий конструктор
    DynamicArray(DynamicArray<T> &&dynamic_array) noexcept: items(std::move(dynamic_array.items)), size(dynamic_array.size) {
        dynamic_array.size = 0;
    }

    // Перемещающее присваивание
    DynamicArray<T> &operator=(DynamicArray<T> &&dynamic_array) noexcept {
        if (this == &dynamic_array) {return *this;}

        items = std::move(dynamic_array.items);
        size = dynamic_array.size;
        dynamic_array.size = 0;

        return *this;
    }

    // Возвращает размер массива
    int GetSize() const noexcept {
        return size;
    }

    // Получает элемент по индексу
    const T &Get(int index) const {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Индекс невалиден");
        }

        return items[index];
    }

    // Получает изменяемый элемент по индексу
    T &Get(int index) {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Индекс невалиден");
        }

        return items[index];
    }

    // Устанавливает значение элемента по индексу
    void Set(int index, const T &value) {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Индекс невалиден");
        }

        items[index] = value;
    }

    // Перемещает значение в элемент по индексу
    void Set(int index, T &&value) {
        if (index < 0 || index >= size) {
            throw std::out_of_range("Индекс невалиден");
        }

        items[index] = std::move(value);
    }

    // Изменяет размер массива
    void Resize(int new_size) {
        if (new_size < 0) {
            throw std::invalid_argument("Размер массива не может быть отрицательным");
        }

        if (new_size == size) {return;}

        UnqPtr<T[]> NewItems(new_size > 0 ? new T[new_size]{} : nullptr);
        int ElementsToMove = (new_size < size) ? new_size : size;

        for (int element = 0; element < ElementsToMove; element++) {
            NewItems[element] = std::move_if_noexcept(items[element]);
        }

        items = std::move(NewItems);
        size = new_size;
    }

    // Очищает массив
    void Clear() noexcept {
        items.reset();
        size = 0;
    }

    // Обменивает содержимое массивов
    void swap(DynamicArray<T> &other) noexcept {
        items.swap(other.items);

        int TempSize = size;
        size = other.size;
        other.size = TempSize;
    }

    // Освобождение массива выполняет UnqPtr
    ~DynamicArray() noexcept = default;
};

template <typename T>
void swap(DynamicArray<T> &first, DynamicArray<T> &second) noexcept {
    first.swap(second);
}

#endif // DYNAMICARRAY_H