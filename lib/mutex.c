#include <stdlib.h>     // malloc
#include <pthread.h>    // pthread_mutex_t, pthread_mutex_init
#include <sys/types.h>  // ssize_t

#include "buffer.h"     // LinkedBytes

struct ExclusiveData {
    pthread_mutex_t lock;
    struct LinkedBytes * head;
};

struct ExclusiveData * exclusive_data_create(char * data, ssize_t size)
{
    struct ExclusiveData * new_data = malloc( sizeof(struct ExclusiveData) );
    pthread_mutex_init(&new_data->lock, NULL);
    new_data->head = bytes_create(size, data);
    return new_data;
}


struct ExclusiveData * append_to_exclusive_data(struct ExclusiveData * data, char * new_data, ssize_t size)
{
    if (size < 1) return data;
    pthread_mutex_lock(&data->lock);
    append_to_bytelist(data->head, bytes_create(size, new_data));
    pthread_mutex_unlock(&data->lock);
    return data;
}


long exclusive_data_read(struct ExclusiveData * data, char * store, int flush)
{
    /*
        safely blocks and reads ExclusiveData contents,
        also implements functionality for:
            storing contents of read
            getting contents size
            flushing contents

        PRIVATE function, used by high-level functions below
    */

    pthread_mutex_lock(&data->lock);

    long data_size = bytelist_size(data->head);

    if ( data_size && store != NULL ) {

        if ( bytelist_store_data(data->head, store, data_size) != 0) {

            data_size = -1;

        }

    }

    if (flush) bytelist_flush(data->head);

    pthread_mutex_unlock(&data->lock);

    return data_size;
}


long exclusive_data_copy(char * dest, struct ExclusiveData * data)
{
    /* stores: YES, size: YES, FLUSH: yes */
    return exclusive_data_read(data, dest, 1);
}


long exclusive_data_size(struct ExclusiveData * data)
{
    /* stores: NO , size: YES, FLUSH: NO */
    return exclusive_data_read(data, NULL, 0);
}


void exclusive_data_flush(struct ExclusiveData * data)
{
    /* stores: NO , size: NO , FLUSH: yes */
    exclusive_data_read(data, NULL, 1);
}
