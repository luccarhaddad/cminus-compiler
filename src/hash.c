//
// Created by Lucca Haddad on 14/12/24.
//

#include "hash.h"
#include <stdlib.h>
#include <string.h>

#define SIZE 23

static struct DataItem* hashArray[SIZE];

static int hash(char* key) {
	int hashVal = 0;
	while (*key != '\0') hashVal = (hashVal << 5) + *key++;
	return hashVal % SIZE;
}

void hashInit() {
	for (int i = 0; i < SIZE; i++) hashArray[i] = NULL;
}

void hashInsert(char* key, int data) {
	struct DataItem* item = malloc(sizeof(struct DataItem));
	item->data            = data;
	item->key             = key;

	int hashIndex = hash(key);

	while (hashArray[hashIndex] != NULL) {
		++hashIndex;
		hashIndex %= SIZE;
	}

	hashArray[hashIndex] = item;
}

int hashSearch(char* key) {
	int hashIndex = hash(key);

	while (hashArray[hashIndex] != NULL) {
		if (strcmp(hashArray[hashIndex]->key, key) == 0) return hashArray[hashIndex]->data;

		++hashIndex;
		hashIndex %= SIZE;
	}

	return 1024;
}

void hashDelete(char* key) {
	int hashIndex = hash(key);

	while (hashArray[hashIndex] != NULL) {
		if (strcmp(hashArray[hashIndex]->key, key) == 0) {
			free(hashArray[hashIndex]);
			hashArray[hashIndex] = NULL;
			return;
		}

		++hashIndex;
		hashIndex %= SIZE;
	}
}