#ifndef CHECKS_LAPTOP_H
#define CHECKS_LAPTOP_H

#include <stddef.h>

/* Returns 1 if the system appears to be a laptop, 0 otherwise */
int hw_is_laptop(void);

/* Writes laptop vendor + model string into buf */
void hw_laptop_model(char *buf, size_t size);

/* Returns human-readable laptop description (static buffer) */
const char *hw_laptop_description(void);

#endif
