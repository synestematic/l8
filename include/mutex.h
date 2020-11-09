// so FAR members of this struct do NOT need to be exposed...
struct ExclusiveData;

struct ExclusiveData * exclusive_data_create(char * data, ssize_t size);

struct ExclusiveData * append_to_exclusive_data(struct ExclusiveData * data, char * new_data, ssize_t size);

long exclusive_data_copy(char * dest, struct ExclusiveData * data);

long exclusive_data_size(struct ExclusiveData * data);

void exclusive_data_flush(struct ExclusiveData * data);
