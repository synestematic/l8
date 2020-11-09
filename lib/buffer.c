#include <stdlib.h>     // NULL, malloc
#include <string.h>     // memcpy
#include <sys/types.h>  // ssize_t

#include "buffer.h"

struct LinkedBytes* bytes_create(ssize_t size, char * data)
{
    struct LinkedBytes * new_bytes = malloc(sizeof(struct LinkedBytes));
    new_bytes->size = size;
    new_bytes->data = malloc(size);
    memcpy(new_bytes->data, data, size);
    new_bytes->next = NULL;
    return new_bytes;
}


/*
    The following functions work on the ENTIRE LIST of linked bytes,
    NOT the individual LinkedBytes nodes
*/

struct LinkedBytes * append_to_bytelist(struct LinkedBytes* head, struct LinkedBytes* new)
{
    /*
        SCANS LLIST OF struct LinkedBytes DATA UNTIL LAST struct LinkedBytes IS FOUND
        APPENDS NEW struct LinkedBytes TO END OF LLIST
        RETURNS __ALWAYS__ FIRST struct LinkedBytes OF LLIST
    */

    struct LinkedBytes * this = head;
    struct LinkedBytes * last = NULL;

    // SCAN LLIST
    while (last == NULL) {
        if (this->next == NULL) last = this;
        else                    this = this->next;
    }

    last->next = new;  // APPEND NEW struct LinkedBytes

    // RETURN FIRST struct LinkedBytes
    return head;

}

struct LinkedBytes * bytelist_flush(struct LinkedBytes* head)
{
    /*
        starts from head, links to next->next, frees next
    */

    while (head->next != NULL) {
        struct LinkedBytes * gone = head->next;
        head->next = head->next->next;
        free(gone->data);
        free(gone);
    }

    // reset head but keep original ptr
    memcpy(head->data, "", 0);
    head->size = 0;
    return head;

}

long bytelist_size(struct LinkedBytes * head)
{
    long size = 0;

    struct LinkedBytes * this = head;
    struct LinkedBytes * next;

    do {
        next  = this->next;
        size += this->size;
        this  = this->next;
    } while (next != NULL);

    return size;
}

int bytelist_store_data(struct LinkedBytes * head, char * store, int size_check)
{
    long offset = 0;

    struct LinkedBytes * this = head;
    struct LinkedBytes * next;

    do {
        next  = this->next;
        memcpy(store + offset, this->data, this->size);
        offset += this->size;
        this  = this->next;
    } while (next != NULL);

    // CHECK AMOUNT OF BYTES COPIED
    if (offset != size_check) return -1;
    return 0;
}
