#include "payment.h"
#include "fileutil.h"
#include "config.h"
#include <string.h>
#include <time.h>
#include <stdio.h>

Payment payment_new(int id, int memberid, double amount, const char *type, time_t txtime, int fineid)
{
    Payment payment = {0};
    payment.id = id;
    payment.memberid = memberid;
    payment.amount = amount;
    strncpy(payment.type, type ? type : "", MAX_TYPE_LEN - 1);
    payment.txtime = txtime;
    payment.fineid = fineid;
    return payment;
}

int payment_add_new(const Payment *payment)
{
    Payment new_payment = *payment;
    new_payment.id = get_next_id(PAYMENTS_FILE, sizeof(Payment));
    return write_record(PAYMENTS_FILE, &new_payment, sizeof(Payment));
}

static int match_by_member(const void *record, const void *criteria)
{
    const Payment *payment = (const Payment *)record;
    const int *memberid = (const int *)criteria;
    return payment->memberid == *memberid;
}

int payment_find_by_member(int memberid, const char *type, Payment *payments, int max_payments)
{
    // This function assumes `type` is ignored for now and finds all payments for the member.
    // To filter by type, you would need a more complex matching function.
    return find_records(PAYMENTS_FILE, payments, max_payments, sizeof(Payment),
                        match_by_member, &memberid);
}

Payment *payment_find_last_paid(int member_id, const char *type)
{
    // Placeholder function. The implementation would need to iterate through
    // payments for the member and find the most recent one of the specified type.
    return NULL;
}