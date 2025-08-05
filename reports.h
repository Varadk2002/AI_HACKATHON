// reports.h
#ifndef REPORTS_H
#define REPORTS_H

#include "types.h"
#include <time.h>

// Report functions
int subjectwise_copies_report(SubjectReport *reports, int max_reports);
int bookwise_copies_report(BookwiseReport *reports, int max_reports);
int daterange_fees_fine_collection(time_t start, time_t end, CollectionReport *reports, int max_reports);
void print_subjectwise_report(void);
void print_bookwise_report(void);
void print_collection_report(time_t start, time_t end);

#endif // REPORTS_H