

int g_nfail, g_lasterr;
void front_save_failed(int err) { g_nfail++; g_lasterr = err; }

int g_t_lang = -1;
int dm_custom_lang_explicit(void) { return g_t_lang; }
int g_t_voice = -1;
int dm_custom_voice_explicit(void) { return g_t_voice; }
