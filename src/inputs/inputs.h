#ifndef INPUTS_H
# define INPUTS_H

# define WHITESPACES_DELIM " \t\r\n"
# define ARG_REALLOC 4

/*
** \fn char *strip_whitespace(char *str)
** \param str String to strip
**
** \brief Move a pointer to first none whitespace character
** and set the character after the last none whitespace
** character to '\0'.
**
** \return Return a pointer inside str.
*/
char *strip_whitespace(char *str);

/**
** \fn void init_interaction(void)
** \brief Load the history and init the completion.
*/
void init_interaction(void);

/**
** \param text String to cut with WHITESPACE_DELIM
**
** \brief Allocate and fill a null terminated array containing
** the command and its argument given by the user.
**
** \return Returns the newly allocated array of string
*/
char **build_cmd(char *text);

/**
** \param args Null terminated array to duplicate
**
** \brief Duplicate \p args
**
** \return Returns the newly allocated array of allocated
** strings
*/
char **dup_args(char *args[]);

/**
** \param args Null terminated array to destroy
**
** \brief Free all the allocated memory inside \p args. Including args
*/
void destroy_args(char **args);

char *get_line(void);

#endif /* !INPUTS_H */
