#include "Menu.h"

#include <iostream>
#include "WeakPtr.hpp"

struct Empl {
    int value = 0;
    WeakPtr<Empl> Collegue;

    explicit Empl(int NewValue): value(NewValue) {}

    ~Empl() {
        std::cout << "Объект " << value << " уничтожен\n";
    }
};

int main() {
    WeakPtr<Empl> Observer;

    {
        ShrdPtr<Empl> first(UnqPtr<Empl>(new Empl(1)));
        ShrdPtr<Empl> second(UnqPtr<Empl>(new Empl(2)));

        first->Collegue = second;
        second->Collegue = first;
        Observer = first;

        std::cout << "Сильные владельцы: " << first.UseCount() << " " << second.UseCount() << "\n";
        if (first.UseCount() != 1 || second.UseCount() != 1) {return 1;}

        {
            ShrdPtr<Empl> Colleague = first->Collegue.lock();
            if (!Colleague || Colleague->value != 2 || second.UseCount() != 2) {return 1;}
            std::cout << "Через lock получил объект " << Colleague->value << "\n";
            std::cout << "Владельцев второго объекта: " << second.UseCount() << "\n";
        }

        std::cout << "После уничтожения временного владельца: " << second.UseCount() << "\n";
        if (second.UseCount() != 1) {return 1;}
    }

    std::cout << "Объект больше не существует: " << Observer.expired() << "\n";
    ShrdPtr<Empl> Result = Observer.lock();
    std::cout << "lock вернул пустой указатель: " << (!Result) << "\n";

    if (!Observer.expired() || Observer.UseCount() != 0 || Result) {return 1;}
    
    return RunMenu();
}

