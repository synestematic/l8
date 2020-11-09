/*  LinkedBytes stores a chunk of data and a pointer to the next chunk,
    creating a linked bytelist of data
*/
struct LinkedBytes {
    ssize_t size;
    char * data;
    struct LinkedBytes * next;
};

struct LinkedBytes * bytes_create(ssize_t size, char * data);

struct LinkedBytes * append_to_bytelist(struct LinkedBytes * head, struct LinkedBytes * new);

struct LinkedBytes * bytelist_flush(struct LinkedBytes * head);

long bytelist_size(struct LinkedBytes * head);

int bytelist_store_data(struct LinkedBytes * head, char * store, int size_check);
