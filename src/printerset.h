#ifndef PRINTERSET_H
#define PRINTERSET_H

// `Handler should return nonzero on success, zero on error.'
#define COPRIS_PARSE_FAILURE   0
#define COPRIS_PARSE_SUCCESS   1

#define SET_RAW_VALUE(value) \
        memccpy(raw_value, value, '\0', MAX_INIFILE_ELEMENT_LENGTH)

#define INSERT_TEXT(string)  \
        utstring_bincpy(text, string, (sizeof string) - 1)

#define INSERT_CODE(string)  \
        insert_code_helper(string, prset, text)

/*
 * Load printer set file `filename' into a hash table, passed by `prset'.
 * Return 0 on success.
 */
int load_printer_set_file(const char *filename, struct Inifile **prset);

/*
 * Print out all printer commands, recognised by COPRIS, in an INI-style format.
 */
int dump_printer_set_commands(struct Inifile **prset);

/*
 * Unload printer set hash table, loaded from `filename', passed by `prset'.
 */
void unload_printer_set_file(const char *filename, struct Inifile **prset);

/*
 * Take input text `copris_text' and convert the Markdown specifiers to printer's
 * escape codes, passed by `prset' hash table. Put converted text into `copris_text',
 * overwriting previous content.
 */
void convert_markdown(UT_string *copris_text, struct Inifile **prset);

#endif /* PRINTERSET_H */
