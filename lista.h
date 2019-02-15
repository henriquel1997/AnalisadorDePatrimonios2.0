//
// Created by Henrique on 14/02/2019.
//

template <typename T>
struct Lista{
    T* array;
    unsigned int size;
};

template <typename T>
void addTo(Lista<T>* lista, T newValue);
template <typename T>
void addTo(Lista<T>* dest, Lista<T>* source);
template <typename T>
void freeLista(Lista<T> *lista);