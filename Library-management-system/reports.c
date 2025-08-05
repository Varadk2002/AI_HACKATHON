// reports.c
#include "reports.h"
#include "fileutil.h"
#include "config.h"
#include "books.h"
#include "payment.h"
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int subjectwise_copies_report(SubjectReport *reports, int max_reports)
{
    int fd = open(BOOKS_FILE, O_RDONLY);
    if (fd == -1)
        return 0;

    Book book;
    int report_count = 0;

    while (read(fd, &book, sizeof(Book)) == sizeof(Book))
    {
        int found = -1;
        for (int i = 0; i < report_count; i++)
        {
            if (strcmp(reports[i].subject, book.subject) == 0)
            {
                found = i;
                break;
            }
        }

        if (found >= 0)
        {
            int fd_copies = open(COPIES_FILE, O_RDONLY);
            if (fd_copies != -1)
            {
                BookCopy copy;
                while (read(fd_copies, &copy, sizeof(BookCopy)) == sizeof(BookCopy))
                {
                    if (copy.bookid == book.id)
                    {
                        reports[found].count++;
                    }
                }
                close(fd_copies);
            }
        }
        else if (report_count < max_reports)
        {
            strcpy(reports[report_count].subject, book.subject);
            reports[report_count].count = 0;

            int fd_copies = open(COPIES_FILE, O_RDONLY);
            if (fd_copies != -1)
            {
                BookCopy copy;
                while (read(fd_copies, &copy, sizeof(BookCopy)) == sizeof(BookCopy))
                {
                    if (copy.bookid == book.id)
                    {
                        reports[report_count].count++;
                    }
                }
                close(fd_copies);
            }
            report_count++;
        }
    }

    close(fd);
    return report_count;
}

int bookwise_copies_report(BookwiseReport *reports, int max_reports)
{
    int fd = open(BOOKS_FILE, O_RDONLY);
    if (fd == -1)
        return 0;

    Book book;
    int report_count = 0;

    while (read(fd, &book, sizeof(Book)) == sizeof(Book) && report_count < max_reports)
    {
        reports[report_count].id = book.id;
        strcpy(reports[report_count].name, book.name);
        reports[report_count].available = 0;
        reports[report_count].issued = 0;
        reports[report_count].total_count = 0;

        int fd_copies = open(COPIES_FILE, O_RDONLY);
        if (fd_copies != -1)
        {
            BookCopy copy;
            while (read(fd_copies, &copy, sizeof(BookCopy)) == sizeof(BookCopy))
            {
                if (copy.bookid == book.id)
                {
                    reports[report_count].total_count++;
                    if (strcmp(copy.status, "available") == 0)
                    {
                        reports[report_count].available++;
                    }
                    else if (strcmp(copy.status, "issued") == 0)
                    {
                        reports[report_count].issued++;
                    }
                }
            }
            close(fd_copies);
        }
        report_count++;
    }

    close(fd);
    return report_count;
}

int daterange_fees_fine_collection(time_t start, time_t end, CollectionReport *reports, int max_reports)
{
    int fd = open(PAYMENTS_FILE, O_RDONLY);
    if (fd == -1)
        return 0;

    Payment payment;
    int report_count = 0;
    double fee_total = 0.0, fine_total = 0.0;

    while (read(fd, &payment, sizeof(Payment)) == sizeof(Payment))
    {
        if (payment.txtime >= start && payment.txtime <= end)
        {
            if (strcmp(payment.type, "fee") == 0)
            {
                fee_total += payment.amount;
            }
            else if (strcmp(payment.type, "fine") == 0)
            {
                fine_total += payment.amount;
            }
        }
    }

    close(fd);

    if (fee_total > 0 && report_count < max_reports)
    {
        strcpy(reports[report_count].type, "fee");
        reports[report_count].amount = fee_total;
        report_count++;
    }

    if (fine_total > 0 && report_count < max_reports)
    {
        strcpy(reports[report_count].type, "fine");
        reports[report_count].amount = fine_total;
        report_count++;
    }

    return report_count;
}

void print_subjectwise_report(void)
{
    SubjectReport reports[50];
    int count = subjectwise_copies_report(reports, 50);

    printf("\n=== Subjectwise Copies Report ===\n");
    printf("%-30s %s\n", "Subject", "Copies Count");
    printf("%-30s %s\n", "--------", "------------");

    for (int i = 0; i < count; i++)
    {
        printf("%-30s %d\n", reports[i].subject, reports[i].count);
    }
}

void print_bookwise_report(void)
{
    BookwiseReport reports[100];
    int count = bookwise_copies_report(reports, 100);

    printf("\n=== Bookwise Copies Report ===\n");
    printf("%-5s %-40s %-10s %-8s %-8s\n", "ID", "Book Name", "Available", "Issued", "Total");
    printf("%-5s %-40s %-10s %-8s %-8s\n", "---", "---------", "---------", "------", "-----");

    for (int i = 0; i < count; i++)
    {
        printf("%-5d %-40.40s %-10d %-8d %-8d\n",
               reports[i].id, reports[i].name,
               reports[i].available, reports[i].issued, reports[i].total_count);
    }
}

void print_collection_report(time_t start, time_t end)
{
    CollectionReport reports[10];
    int count = daterange_fees_fine_collection(start, end, reports, 10);

    printf("\n=== Collection Report ===\n");
    printf("%-15s %s\n", "Type", "Amount");
    printf("%-15s %s\n", "----", "------");

    double total = 0.0;
    for (int i = 0; i < count; i++)
    {
        printf("%-15s %.2f\n", reports[i].type, reports[i].amount);
        total += reports[i].amount;
    }
    printf("%-15s %.2f\n", "TOTAL", total);
}