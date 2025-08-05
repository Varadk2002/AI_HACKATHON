#ifndef BOOK_H
#define BOOK_H

#define BOOK_FILE "books.txt"
#define MAX_TITLE_LEN 100
#define MAX_AUTHOR_LEN 50
#define MAX_SUBJECT_LEN 50
#define PRICE

typedef struct
{
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];
    char subject[MAX_SUBJECT_LEN];
    int price;
    int copies;
} book_t;

#endif // BOOK_H