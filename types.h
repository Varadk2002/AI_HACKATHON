#ifndef TYPES_H
#define TYPES_H

#include <time.h> // Include this for time_t

#define MAX_NAME_LEN 50
#define MAX_EMAIL_LEN 50
#define MAX_PHONE_LEN 15
#define MAX_PASSWD_LEN 50
#define MAX_ACCOUNTS 200

// Book data structure definitions
#define MAX_TITLE_LEN 100
#define MAX_AUTHOR_LEN 50
#define MAX_SUBJECT_LEN 50

// typedef struct {
//     char title[MAX_TITLE_LEN];
//     char author[MAX_AUTHOR_LEN];
//     char subject[MAX_SUBJECT_LEN];
//     int price;
//     int copies;
// } book_t;

// Member and User data structures
typedef struct
{
    char name[MAX_NAME_LEN];
    char email[MAX_EMAIL_LEN];
    char phone[MAX_PHONE_LEN];
    char password[MAX_PASSWD_LEN];
} member_t;

typedef struct
{
    char name[MAX_NAME_LEN];
    char email[MAX_EMAIL_LEN];
    char phone[MAX_PHONE_LEN];
    char password[MAX_PASSWD_LEN];
    int payment_due;
    int fines_due;
} user_t;

typedef struct
{
    int type; // 0 for member, 1 for user
    union
    {
        member_t member;
        user_t user;
    } data;
} account_t;

// Borrowing data structure
typedef struct
{
    char user_email[MAX_EMAIL_LEN];
    char book_title[MAX_TITLE_LEN];
    time_t due_date_timestamp; // Changed to time_t
} borrowing_t;

#endif // TYPES_H