#ifndef PAYMENT_H
#define PAYMENT_H

#include <time.h>
#include "types.h"

// Creates a new Payment structure.
Payment payment_new(int id, int memberid, double amount, const char *type, time_t txtime, int fineid);

// Adds a new payment record to the file.
int payment_add_new(const Payment *payment);

// Finds all payment records for a specific member.
int payment_find_by_member(int memberid, const char *type, Payment *payments, int max_payments);

// Finds the last payment made by a member of a specific type.
Payment *payment_find_last_paid(int member_id, const char *type);

#endif