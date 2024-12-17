//
// Created by Lucca Haddad on 14/12/24.
//

#ifndef HASH_H
#define HASH_H

#define SIZE 23

struct DataItem {
    char* key;
    int data;
};

void hashInit();
void hashInsert(char* key, int data);
int hashSearch(char* key);
void hashDelete(char* key);

#endif // HASH_H