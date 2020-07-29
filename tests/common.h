
void fail(const char* msg);
int open_ctrlfile(const char* srvfs, const char* name);
int open_localfile(const char* fn);
int assign_fd(const char* srvfs, const char* ctrlname, int local_fd);
