typedef struct {
    const char *kw;
    const char *enc;
} Keyword;

Keyword keywords[] = {
    {"PRINT", "\xF1\x20"},
    {NULL, NULL}
};

const char* get_kw(char* buf, size_t* kw_len, size_t* kwenc_len) {
    for (size_t i = 0; keywords[i].kw != NULL; ++i) {
        Keyword *kw = &keywords[i];
        *kwenc_len = strlen(kw->enc);
        if (!strncmp(buf, kw->enc, *kwenc_len)) {
            *kw_len = strlen(kw->kw);
            return kw->kw;
        }
    }

    return NULL;
}
