#ifndef INPUTS_H
# define INPUTS_H

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

char *get_line(void);

#endif /* !INPUTS_H */
