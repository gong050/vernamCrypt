//
//  main.cpp
//  verman
//
//  Created by Alex Kustov on 21/03/2018.
//  Copyright © 2018 Alex Kustov. All rights reserved.
//

#include <iostream>
#include <fstream>
#include <string>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#define SUCCESS                 0
#define ERROR_CREATE_THREAD     -11
#define ERROR_BARRIER           -12
#define ERROR_BARRIER_DESTROY   -13
#define ERROR_JOIN_THREAD       -14
#define ERROR_FILE_OPEN         -15
#define NUM_THREAD              11  // 10 потоков + main

pthread_barrier_t barrier;

struct keygenParameters
{
    size_t a;
    size_t c;
    size_t m;
    unsigned char seed;
    size_t keySize;
};

struct cryptParam
{
    unsigned char* msg;
    unsigned char* key;
    unsigned char* outputText;
    size_t size;
    size_t downIndex;
    size_t topIndex;
};

void* keyGen(void * params)
{
    keygenParameters *param = (keygenParameters *)params;
    size_t a = param->a;
    size_t m = param->m;
    size_t c = param->c;
    size_t keySize = param->keySize;

    unsigned char* key = new unsigned char[keySize];
    unsigned char element = param->seed;

    for(size_t i = 0; i < keySize; i++){
        key[i] = (a * element + c) % m;
        element = key[i];
    }

    return key;
};

void* crypt(void * cryptParametrs)
{
    int status = 0;

    cryptParam* param = (cryptParam*)cryptParametrs;
    unsigned char* msg = param->msg;
    unsigned char* key = param->key;
    unsigned char* outputText = param->outputText;
    size_t topIndex = param->topIndex;
    size_t downIndex = param->downIndex;

    while(downIndex < topIndex)
    {
        outputText[downIndex] = key[downIndex] ^ msg[downIndex];
        downIndex++;
    }

    param->outputText = outputText;
    delete(param);

    status = pthread_barrier_wait(&barrier);
    if(status == PTHREAD_BARRIER_SERIAL_THREAD)
        pthread_barrier_destroy(&barrier);
    else if(status != 0)
    {
        std::cout << "promblem with pthread_barrier_destroy";
        exit(ERROR_BARRIER_DESTROY);
    }
}

int main(int argc, const char * argv[])
{
    std::string input;
    std::ifstream file ("/home/alex/Desktop/verman/verman/text.txt");

    // Проверяем, подключился ли файл или нет
    if(!file){
        std::cout << "Error with file";
        exit(ERROR_FILE_OPEN);
    }

    // Читаем файл и записываем все содержимое в строку input
    getline(file, input);
    file.close();

    size_t inputSize = input.length();

    keygenParameters keyParam;
    unsigned char* key = new unsigned char[inputSize];
    unsigned char* outputText = new unsigned char[inputSize];

    // Задаем параметры генерации ключа
    keyParam.keySize = inputSize;
    keyParam.a = 32;
    keyParam.c = 50;
    keyParam.m = 43;
    keyParam.seed = 74;

    pthread_t keyGenThread;
    pthread_t cryptThread[NUM_THREAD];
    int status = 0;

    if(pthread_create(&keyGenThread, NULL, keyGen, &keyParam) != 0)
    {
        std::cout << "error with pthread_create()";
        exit(ERROR_CREATE_THREAD);
    }
    if(pthread_join(keyGenThread, (void**)&key) != 0)
    {
        std::cout << "error with pthread_join()";
        exit(ERROR_JOIN_THREAD);
    }

    status = pthread_barrier_init(&barrier, NULL, NUM_THREAD);

    if(status != 0)
    {
        std::cout << "error with pthread_barrier_init()";
        exit(ERROR_BARRIER);
    }

    for(int i = 0; i < 10; i++)
    {
        cryptParam* cryptParametrs = new cryptParam;

        cryptParametrs->key = key;
        cryptParametrs->size = inputSize;
        cryptParametrs->outputText = outputText;
        cryptParametrs->msg = (unsigned char*)input.c_str();

        if(i == 0)
        {
            cryptParametrs->downIndex = 0;
        }
        else
        {
            cryptParametrs->downIndex = inputSize / 10 * i;
        }

        if(i == 9)
        {
            cryptParametrs->topIndex = inputSize;
        }
        else
        {
            cryptParametrs->topIndex = inputSize / 10 * (i + 1);
        }

        pthread_create(&cryptThread[i], NULL, crypt, cryptParametrs);
    }

    status = pthread_barrier_wait(&barrier);
    if(status == PTHREAD_BARRIER_SERIAL_THREAD)
        pthread_barrier_destroy(&barrier);
    else if(status != 0)
    {
        std::cout << "promblem with pthread_barrier_destroy";
        exit(ERROR_BARRIER_DESTROY);
    }

    std::ofstream output("/home/alex/Desktop/verman/verman/output.txt");
    output << outputText;
    output.close();

    std::cout << "mes";

    return SUCCESS;
}
