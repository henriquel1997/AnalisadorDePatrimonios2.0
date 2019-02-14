//
// Created by Henrique on 14/02/2019.
//

#include <cstdlib>

#include "lista.h"

template <typename T>
void addTo(Lista<T>* lista, T newValue){
    auto newArray = (T*)malloc(sizeof(T)*(lista->size + 1));
    for(int i = 0; i < lista->size; i++){
        newArray[i] = lista->array[i];
    }
    newArray[lista->size] = newValue;
    free(lista->array);
    lista->array = newArray;
    lista->size++;
}

template <typename T>
void addTo(Lista<T>* dest, Lista<T>* source){
    unsigned int novoTamanho = dest->size + source->size;
    auto newArray = (T*)malloc(sizeof(T)*(dest->size + source->size));

    for(unsigned int i = 0; i < dest->size; i++){
        newArray[i] = dest->array[i];
    }

    for(unsigned int i = dest->size; i - dest->size < source->size; i++){
        newArray[i] = source->array[i];
    }

    free(dest->array);
    dest->array = newArray;
    dest->size = novoTamanho;
}