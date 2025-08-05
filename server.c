#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>

#include "types.h"
#include "config.h"
#include "book.h"

#define MEMBER_FILE "members.txt"
#define USERS_FILE "users.txt"
#define PAYMENTS_FILE "payments.txt"
#define FINES_FILE "fines.txt"
#define BORROWINGS_FILE "borrowings.txt"
#define MAX_ACCOUNTS 200
#define MAX_BOOKS 100

account_t accounts[MAX_ACCOUNTS];
int account_count = 0;
book_t books[MAX_BOOKS];
int book_count = 0;

void load_accounts_from_file();
void save_user_to_file(const user_t *new_user);
void save_member_to_file(const member_t *new_member);
void save_payment_to_file(const char *email, int amount);
void save_fine_to_file(const char *email, int amount);
void load_books_from_file();

void handle_client_session(int client_sock);
void handle_sign_in(const char *payload, char *response, int *logged_in_type, char *logged_in_email);
void handle_sign_up(const char *payload, char *response);
void handle_add_book(const char *payload, char *response);
void handle_remove_book(const char *payload, char *response);
void handle_update_my_info(const char *payload, char *response, const char *logged_in_email);
void handle_update_book(const char *payload, char *response);
void handle_check_copies(const char *payload, char *response);
void handle_collect_payment(const char *payload, char *response);
void handle_collect_fine(const char *payload, char *response);
void handle_view_users(char *response);
void handle_delete_user(const char *payload, char *response);
void handle_update_user_info(const char *payload, char *response);
void handle_borrow_book(const char *payload, char *response, const char *logged_in_email);
void handle_return_book(const char *payload, char *response);

int find_account_by_email(const char *email, int *account_type)
{
    for (int i = 0; i < account_count; i++)
    {
        if (accounts[i].type == 0)
        {
            if (strcmp(accounts[i].data.member.email, email) == 0)
            {
                *account_type = 0;
                return i;
            }
        }
        else
        {
            if (strcmp(accounts[i].data.user.email, email) == 0)
            {
                *account_type = 1;
                return i;
            }
        }
    }
    *account_type = -1;
    return -1;
}

int find_book_by_title(const char *title)
{
    for (int i = 0; i < book_count; i++)
    {
        if (strcmp(books[i].title, title) == 0)
        {
            return i;
        }
    }
    return -1;
}

void load_accounts_from_file()
{
    FILE *member_file = fopen(MEMBER_FILE, "r");
    if (member_file != NULL)
    {
        char line[512];
        while (fgets(line, sizeof(line), member_file) && account_count < MAX_ACCOUNTS)
        {
            accounts[account_count].type = 0;
            sscanf(line, "%[^|]|%[^|]|%[^|]|%s",
                   accounts[account_count].data.member.name,
                   accounts[account_count].data.member.email,
                   accounts[account_count].data.member.phone,
                   accounts[account_count].data.member.password);
            account_count++;
        }
        fclose(member_file);
    }

    FILE *user_file = fopen(USERS_FILE, "r");
    if (user_file != NULL)
    {
        char line[512];
        while (fgets(line, sizeof(line), user_file) && account_count < MAX_ACCOUNTS)
        {
            accounts[account_count].type = 1;
            sscanf(line, "%[^|]|%[^|]|%[^|]|%[^|]|%d|%d",
                   accounts[account_count].data.user.name,
                   accounts[account_count].data.user.email,
                   accounts[account_count].data.user.phone,
                   accounts[account_count].data.user.password,
                   &accounts[account_count].data.user.payment_due,
                   &accounts[account_count].data.user.fines_due);
            account_count++;
        }
        fclose(user_file);
    }
    printf("Loaded %d accounts from file.\n", account_count);
}

void load_books_from_file()
{
    FILE *file = fopen(BOOK_FILE, "r");
    if (file == NULL)
    {
        printf("Book file not found. Starting with an empty book list.\n");
        return;
    }
    char line[512];
    while (fgets(line, sizeof(line), file) && book_count < MAX_BOOKS)
    {
        sscanf(line, "%[^|]|%[^|]|%[^|]|%d|%d",
               books[book_count].title,
               books[book_count].author,
               books[book_count].subject,
               &books[book_count].price,
               &books[book_count].copies);
        book_count++;
    }
    fclose(file);
    printf("Loaded %d books from file.\n", book_count);
}

void save_user_to_file(const user_t *new_user)
{
    FILE *file = fopen(USERS_FILE, "a");
    if (file == NULL)
    {
        perror("Error opening user file for writing");
        return;
    }
    fprintf(file, "%s|%s|%s|%s|%d|%d\n",
            new_user->name,
            new_user->email,
            new_user->phone,
            new_user->password,
            new_user->payment_due,
            new_user->fines_due);
    fclose(file);
}

void save_book_to_file(const book_t *new_book)
{
    FILE *file = fopen(BOOK_FILE, "a");
    if (file == NULL)
    {
        perror("Error opening book file for writing");
        return;
    }
    fprintf(file, "%s|%s|%s|%d|%d\n",
            new_book->title,
            new_book->author,
            new_book->subject,
            new_book->price,
            new_book->copies);
    fclose(file);
}

void save_borrowing_record(const borrowing_t *record)
{
    FILE *file = fopen(BORROWINGS_FILE, "a");
    if (file == NULL)
    {
        perror("Error opening borrowings file for writing");
        return;
    }
    fprintf(file, "%s|%s|%lld\n", record->user_email, record->book_title, (long long)record->due_date_timestamp);
    fclose(file);
}

void save_payment_to_file(const char *email, int amount)
{
    FILE *file = fopen(PAYMENTS_FILE, "a");
    if (file == NULL)
    {
        perror("Error opening payments file for writing");
        return;
    }
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date_str[11];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm);
    fprintf(file, "%s|%d|%s\n", email, amount, date_str);
    fclose(file);
    printf("Payment recorded for user: %s\n", email);
}

void save_fine_to_file(const char *email, int amount)
{
    FILE *file = fopen(FINES_FILE, "a");
    if (file == NULL)
    {
        perror("Error opening fines file for writing");
        return;
    }
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date_str[11];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm);
    fprintf(file, "%s|%d|%s\n", email, amount, date_str);
    fclose(file);
    printf("Fine recorded for user: %s\n", email);
}

void handle_sign_up(const char *payload, char *response)
{
    if (account_count >= MAX_ACCOUNTS)
    {
        strcpy(response, "Error: Maximum accounts reached.");
        return;
    }
    char name[MAX_NAME_LEN];
    char email[MAX_EMAIL_LEN];
    char phone[MAX_PHONE_LEN];
    char password[MAX_PASSWD_LEN];
    int payment;
    if (sscanf(payload, "%[^|]|%[^|]|%[^|]|%[^|]|%d", name, email, phone, password, &payment) != 5)
    {
        strcpy(response, "Error: Invalid sign-up format.");
        return;
    }
    int existing_account_type;
    if (find_account_by_email(email, &existing_account_type) != -1)
    {
        strcpy(response, "Error: Email already exists.");
        return;
    }
    user_t new_user;
    strcpy(new_user.name, name);
    strcpy(new_user.email, email);
    strcpy(new_user.phone, phone);
    strcpy(new_user.password, password);
    new_user.payment_due = (payment > 0) ? 0 : 1;
    new_user.fines_due = 0;
    accounts[account_count].type = 1;
    accounts[account_count].data.user = new_user;
    account_count++;
    save_user_to_file(&new_user);
    if (payment > 0)
    {
        save_payment_to_file(new_user.email, payment);
    }
    printf("New user signed up: %s, %s\n", name, email);
    strcpy(response, "Success: Sign-up successful.");
}

void handle_sign_in(const char *payload, char *response, int *logged_in_type, char *logged_in_email)
{
    char email[MAX_EMAIL_LEN];
    char password[MAX_PASSWD_LEN];
    if (sscanf(payload, "%[^|]|%s", email, password) != 2)
    {
        strcpy(response, "Error: Invalid sign-in format.");
        return;
    }
    int account_type;
    int account_index = find_account_by_email(email, &account_type);
    if (account_index != -1)
    {
        if (account_type == 0) // Member
        {
            if (strcmp(accounts[account_index].data.member.password, password) == 0)
            {
                printf("Member signed in: %s\n", email);
                strcpy(logged_in_email, email);
                *logged_in_type = 0;
                strcpy(response, "Success: Sign-in successful.");
                return;
            }
        }
        else // User
        {
            if (strcmp(accounts[account_index].data.user.password, password) == 0)
            {
                printf("User signed in: %s\n", email);
                strcpy(logged_in_email, email);
                *logged_in_type = 1;
                strcpy(response, "Success: Sign-in successful.");
                return;
            }
        }
    }
    printf("Failed sign-in attempt for email: %s\n", email);
    strcpy(response, "Error: Invalid credentials.");
}

void handle_add_book(const char *payload, char *response)
{
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];
    char subject[MAX_SUBJECT_LEN];
    int price;
    int copies_to_add;

    if (sscanf(payload, "%[^|]|%[^|]|%[^|]|%d|%d", title, author, subject, &price, &copies_to_add) != 5)
    {
        strcpy(response, "Error: Invalid book format.");
        return;
    }

    FILE *original_file = fopen(BOOK_FILE, "r");
    if (original_file != NULL)
    {
        FILE *temp_file = fopen("temp.txt", "w");
        if (temp_file == NULL)
        {
            perror("Error creating temporary file");
            fclose(original_file);
            strcpy(response, "Error: Server failed to process request.");
            return;
        }

        char line[MAX_TITLE_LEN + MAX_AUTHOR_LEN + MAX_SUBJECT_LEN + 25];
        bool book_found = false;

        while (fgets(line, sizeof(line), original_file))
        {
            char current_title[MAX_TITLE_LEN];
            char current_author[MAX_AUTHOR_LEN];
            char current_subject[MAX_SUBJECT_LEN];
            int current_price;
            int current_copies;
            if (sscanf(line, "%[^|]|%[^|]|%[^|]|%d|%d", current_title, current_author, current_subject, &current_price, &current_copies) == 5)
            {
                if (strcmp(current_title, title) == 0)
                {
                    current_copies += copies_to_add;
                    fprintf(temp_file, "%s|%s|%s|%d|%d\n", current_title, current_author, current_subject, current_price, current_copies);
                    book_found = true;
                    printf("Updated copies for book '%s'. New count: %d\n", current_title, current_copies);
                }
                else
                {
                    fprintf(temp_file, "%s", line);
                }
            }
            else
            {
                fprintf(temp_file, "%s", line);
            }
        }
        fclose(original_file);
        fclose(temp_file);

        if (book_found)
        {
            remove(BOOK_FILE);
            rename("temp.txt", BOOK_FILE);
            strcpy(response, "Success: Book copies updated successfully.");
        }
        else
        {
            FILE *file = fopen(BOOK_FILE, "a");
            if (file == NULL)
            {
                perror("Error opening book file for writing");
                strcpy(response, "Error: Could not open book file.");
                return;
            }
            fprintf(file, "%s|%s|%s|%d|%d\n", title, author, subject, price, copies_to_add);
            fclose(file);
            printf("New book added: '%s' by %s\n", title, author);
            strcpy(response, "Success: Book added successfully.");
            remove("temp.txt");
        }
    }
    else
    {
        FILE *file = fopen(BOOK_FILE, "a");
        if (file == NULL)
        {
            perror("Error opening book file for writing");
            strcpy(response, "Error: Could not open book file.");
            return;
        }
        fprintf(file, "%s|%s|%s|%d|%d\n", title, author, subject, price, copies_to_add);
        fclose(file);
        printf("New book added: '%s' by %s\n", title, author);
        strcpy(response, "Success: Book added successfully.");
    }
}

void handle_remove_book(const char *payload, char *response)
{
    FILE *original_file = fopen(BOOK_FILE, "r");
    if (original_file == NULL)
    {
        strcpy(response, "Error: Book file not found.");
        return;
    }
    FILE *temp_file = fopen("temp.txt", "w");
    if (temp_file == NULL)
    {
        perror("Error creating temporary file");
        fclose(original_file);
        strcpy(response, "Error: Server failed to process request.");
        return;
    }
    char line[MAX_TITLE_LEN + MAX_AUTHOR_LEN + MAX_SUBJECT_LEN + 25];
    bool book_found = false;
    char book_to_remove[MAX_TITLE_LEN + 1];
    strcpy(book_to_remove, payload);
    char *newline_pos = strchr(book_to_remove, '\n');
    if (newline_pos)
        *newline_pos = '\0';

    while (fgets(line, sizeof(line), original_file))
    {
        char current_title[MAX_TITLE_LEN];
        char current_author[MAX_AUTHOR_LEN];
        char current_subject[MAX_SUBJECT_LEN];
        int current_price;
        int current_copies;
        if (sscanf(line, "%[^|]|%[^|]|%[^|]|%d|%d", current_title, current_author, current_subject, &current_price, &current_copies) == 5)
        {
            if (strcmp(current_title, book_to_remove) == 0)
            {
                book_found = true;
                if (current_copies > 1)
                {
                    current_copies--;
                    fprintf(temp_file, "%s|%s|%s|%d|%d\n", current_title, current_author, current_subject, current_price, current_copies);
                    printf("Decreased copies for book '%s'. New count: %d\n", current_title, current_copies);
                    strcpy(response, "Success: Book copy removed successfully.");
                }
                else
                {
                    printf("Removed last copy of book: '%s'\n", current_title);
                    strcpy(response, "Success: Book removed successfully.");
                }
            }
            else
            {
                fprintf(temp_file, "%s", line);
            }
        }
        else
        {
            fprintf(temp_file, "%s", line);
        }
    }
    fclose(original_file);
    fclose(temp_file);
    if (book_found)
    {
        remove(BOOK_FILE);
        rename("temp.txt", BOOK_FILE);
    }
    else
    {
        remove("temp.txt");
        strcpy(response, "Error: Book not found.");
    }
}

void handle_update_my_info(const char *payload, char *response, const char *logged_in_email)
{
    FILE *original_file = fopen(USERS_FILE, "r");
    if (original_file == NULL)
    {
        strcpy(response, "Error: User file not found.");
        return;
    }
    FILE *temp_file = fopen("temp_users.txt", "w");
    if (temp_file == NULL)
    {
        perror("Error creating temporary file");
        fclose(original_file);
        strcpy(response, "Error: Server failed to process request.");
        return;
    }
    char name[MAX_NAME_LEN];
    char new_email[MAX_EMAIL_LEN];
    char phone[MAX_PHONE_LEN];
    char password[MAX_PASSWD_LEN];
    if (sscanf(payload, "%[^|]|%[^|]|%[^|]|%s", name, new_email, phone, password) != 4)
    {
        strcpy(response, "Error: Invalid update format.");
        fclose(original_file);
        fclose(temp_file);
        remove("temp_users.txt");
        return;
    }
    char line[MAX_NAME_LEN + MAX_EMAIL_LEN + MAX_PHONE_LEN + MAX_PASSWD_LEN + 10];
    bool user_updated = false;
    while (fgets(line, sizeof(line), original_file))
    {
        char current_name[MAX_NAME_LEN];
        char current_email[MAX_EMAIL_LEN];
        char current_phone[MAX_PHONE_LEN];
        char current_password[MAX_PASSWD_LEN];
        int payment_due;
        int fines_due;
        if (sscanf(line, "%[^|]|%[^|]|%[^|]|%[^|]|%d|%d", current_name, current_email, current_phone, current_password, &payment_due, &fines_due) == 6)
        {
            if (strcmp(current_email, logged_in_email) == 0)
            {
                fprintf(temp_file, "%s|%s|%s|%s|%d|%d\n", name, new_email, phone, password, payment_due, fines_due);
                user_updated = true;
                printf("Updated user info for: %s\n", logged_in_email);
            }
            else
            {
                fprintf(temp_file, "%s", line);
            }
        }
        else
        {
            fprintf(temp_file, "%s", line);
        }
    }
    fclose(original_file);
    fclose(temp_file);
    if (user_updated)
    {
        remove(USERS_FILE);
        rename("temp_users.txt", USERS_FILE);
        strcpy(response, "Success: Information updated successfully.");
    }
    else
    {
        remove("temp_users.txt");
        strcpy(response, "Error: Could not update information.");
    }
}

void handle_update_book(const char *payload, char *response)
{
    FILE *original_file = fopen(BOOK_FILE, "r");
    if (original_file == NULL)
    {
        strcpy(response, "Error: Book file not found.");
        return;
    }
    FILE *temp_file = fopen("temp_books.txt", "w");
    if (temp_file == NULL)
    {
        perror("Error creating temporary file");
        fclose(original_file);
        strcpy(response, "Error: Server failed to process request.");
        return;
    }

    char old_title[MAX_TITLE_LEN];
    char new_title[MAX_TITLE_LEN];
    char new_author[MAX_AUTHOR_LEN];
    char new_subject[MAX_SUBJECT_LEN];
    int new_price;
    int new_copies;

    if (sscanf(payload, "%[^|]|%[^|]|%[^|]|%[^|]|%d|%d", old_title, new_title, new_author, new_subject, &new_price, &new_copies) != 6)
    {
        strcpy(response, "Error: Invalid update format.");
        fclose(original_file);
        fclose(temp_file);
        remove("temp_books.txt");
        return;
    }

    char line[MAX_TITLE_LEN + MAX_AUTHOR_LEN + MAX_SUBJECT_LEN + 25];
    bool book_updated = false;

    while (fgets(line, sizeof(line), original_file))
    {
        char current_title[MAX_TITLE_LEN];
        char current_author[MAX_AUTHOR_LEN];
        char current_subject[MAX_SUBJECT_LEN];
        int current_price;
        int current_copies;
        if (sscanf(line, "%[^|]|%[^|]|%[^|]|%d|%d", current_title, current_author, current_subject, &current_price, &current_copies) == 5)
        {
            if (strcmp(current_title, old_title) == 0)
            {
                fprintf(temp_file, "%s|%s|%s|%d|%d\n", new_title, new_author, new_subject, new_price, new_copies);
                book_updated = true;
                printf("Updated book info for: %s\n", old_title);
            }
            else
            {
                fprintf(temp_file, "%s", line);
            }
        }
        else
        {
            fprintf(temp_file, "%s", line);
        }
    }
    fclose(original_file);
    fclose(temp_file);
    if (book_updated)
    {
        remove(BOOK_FILE);
        rename("temp_books.txt", BOOK_FILE);
        strcpy(response, "Success: Book updated successfully.");
    }
    else
    {
        remove("temp_books.txt");
        strcpy(response, "Error: Book not found.");
    }
}

void handle_check_copies(const char *payload, char *response)
{
    FILE *file = fopen(BOOK_FILE, "r");
    if (file == NULL)
    {
        strcpy(response, "Error: Book file not found.");
        return;
    }

    char title_to_find[MAX_TITLE_LEN];
    strcpy(title_to_find, payload);
    char *newline_pos = strchr(title_to_find, '\n');
    if (newline_pos)
        *newline_pos = '\0';

    char line[MAX_TITLE_LEN + MAX_AUTHOR_LEN + MAX_SUBJECT_LEN + 25];
    bool book_found = false;

    while (fgets(line, sizeof(line), file))
    {
        char current_title[MAX_TITLE_LEN];
        char current_author[MAX_AUTHOR_LEN];
        char current_subject[MAX_SUBJECT_LEN];
        int current_price;
        int current_copies;
        if (sscanf(line, "%[^|]|%[^|]|%[^|]|%d|%d", current_title, current_author, current_subject, &current_price, &current_copies) == 5)
        {
            if (strcmp(current_title, title_to_find) == 0)
            {
                snprintf(response, 1024, "Success: '%s' has %d copies.", current_title, current_copies);
                book_found = true;
                break;
            }
        }
    }
    fclose(file);

    if (!book_found)
    {
        strcpy(response, "Error: Book not found.");
    }
}

void handle_view_users(char *response)
{
    FILE *file = fopen(USERS_FILE, "r");
    if (file == NULL)
    {
        strcpy(response, "Error: User file not found.");
        return;
    }

    char line[512];
    char user_list[2048] = "";
    strcpy(user_list, "Success: List of Users\n");

    while (fgets(line, sizeof(line), file))
    {
        strcat(user_list, line);
    }

    fclose(file);
    strcpy(response, user_list);
}

void handle_delete_user(const char *payload, char *response)
{
    strcpy(response, "Error: User accounts cannot be deleted.");
    printf("Attempt to delete user failed: Deletion is not allowed.\n");
}

void handle_update_user_info(const char *payload, char *response)
{
    FILE *original_file = fopen(USERS_FILE, "r");
    if (original_file == NULL)
    {
        strcpy(response, "Error: User file not found.");
        return;
    }
    FILE *temp_file = fopen("temp_users.txt", "w");
    if (temp_file == NULL)
    {
        perror("Error creating temporary file");
        fclose(original_file);
        strcpy(response, "Error: Server failed to process request.");
        return;
    }

    char target_email[MAX_EMAIL_LEN];
    char new_name[MAX_NAME_LEN];
    char new_email[MAX_EMAIL_LEN];
    char new_phone[MAX_PHONE_LEN];
    char new_password[MAX_PASSWD_LEN];

    if (sscanf(payload, "%[^|]|%[^|]|%[^|]|%[^|]|%s", target_email, new_name, new_email, new_phone, new_password) != 5)
    {
        strcpy(response, "Error: Invalid update format.");
        fclose(original_file);
        fclose(temp_file);
        remove("temp_users.txt");
        return;
    }

    char line[512];
    bool user_updated = false;

    while (fgets(line, sizeof(line), original_file))
    {
        char current_name[MAX_NAME_LEN];
        char current_email[MAX_EMAIL_LEN];
        char current_phone[MAX_PHONE_LEN];
        char current_password[MAX_PASSWD_LEN];
        int current_payment_due;
        int current_fines_due;

        if (sscanf(line, "%[^|]|%[^|]|%[^|]|%[^|]|%d|%d", current_name, current_email, current_phone, current_password, &current_payment_due, &current_fines_due) == 6)
        {
            if (strcmp(current_email, target_email) == 0)
            {
                fprintf(temp_file, "%s|%s|%s|%s|%d|%d\n", new_name, new_email, new_phone, new_password, current_payment_due, current_fines_due);
                user_updated = true;
                printf("Updated info for user: %s\n", target_email);
            }
            else
            {
                fprintf(temp_file, "%s", line);
            }
        }
        else
        {
            fprintf(temp_file, "%s", line);
        }
    }

    fclose(original_file);
    fclose(temp_file);

    if (user_updated)
    {
        remove(USERS_FILE);
        rename("temp_users.txt", USERS_FILE);
        strcpy(response, "Success: User updated successfully.");
    }
    else
    {
        remove("temp_users.txt");
        strcpy(response, "Error: User not found.");
    }
}

void handle_collect_payment(const char *payload, char *response)
{
    FILE *original_file = fopen(USERS_FILE, "r");
    if (original_file == NULL)
    {
        strcpy(response, "Error: User file not found.");
        return;
    }
    FILE *temp_file = fopen("temp_users.txt", "w");
    if (temp_file == NULL)
    {
        perror("Error creating temporary file");
        fclose(original_file);
        strcpy(response, "Error: Server failed to process request.");
        return;
    }

    char email_to_find[MAX_EMAIL_LEN];
    int payment_amount;
    if (sscanf(payload, "%[^|]|%d", email_to_find, &payment_amount) != 2)
    {
        strcpy(response, "Error: Invalid payment format.");
        fclose(original_file);
        fclose(temp_file);
        remove("temp_users.txt");
        return;
    }

    char line[512];
    bool user_found = false;

    while (fgets(line, sizeof(line), original_file))
    {
        char current_name[MAX_NAME_LEN];
        char current_email[MAX_EMAIL_LEN];
        char current_phone[MAX_PHONE_LEN];
        char current_password[MAX_PASSWD_LEN];
        int current_payment_due;
        int current_fines_due;

        if (sscanf(line, "%[^|]|%[^|]|%[^|]|%[^|]|%d|%d", current_name, current_email, current_phone, current_password, &current_payment_due, &current_fines_due) == 6)
        {
            if (strcmp(current_email, email_to_find) == 0)
            {
                if (current_payment_due == 0)
                {
                    strcpy(response, "Error: User has no outstanding payment.");
                    user_found = true;
                    fprintf(temp_file, "%s", line);
                }
                else
                {
                    current_payment_due = 0;
                    fprintf(temp_file, "%s|%s|%s|%s|%d|%d\n", current_name, current_email, current_phone, current_password, current_payment_due, current_fines_due);
                    save_payment_to_file(current_email, payment_amount);
                    strcpy(response, "Success: Payment collected successfully.");
                    user_found = true;
                }
            }
            else
            {
                fprintf(temp_file, "%s", line);
            }
        }
        else
        {
            fprintf(temp_file, "%s", line);
        }
    }

    fclose(original_file);
    fclose(temp_file);

    if (user_found)
    {
        remove(USERS_FILE);
        rename("temp_users.txt", USERS_FILE);
    }
    else
    {
        remove("temp_users.txt");
        strcpy(response, "Error: User not found.");
    }
}

void handle_collect_fine(const char *payload, char *response)
{
    FILE *original_file = fopen(USERS_FILE, "r");
    if (original_file == NULL)
    {
        strcpy(response, "Error: User file not found.");
        return;
    }
    FILE *temp_file = fopen("temp_users.txt", "w");
    if (temp_file == NULL)
    {
        perror("Error creating temporary file");
        fclose(original_file);
        strcpy(response, "Error: Server failed to process request.");
        return;
    }

    char email_to_find[MAX_EMAIL_LEN];
    int fine_amount_collected;
    if (sscanf(payload, "%[^|]|%d", email_to_find, &fine_amount_collected) != 2)
    {
        strcpy(response, "Error: Invalid fine format.");
        fclose(original_file);
        fclose(temp_file);
        remove("temp_users.txt");
        return;
    }

    char line[512];
    bool user_found = false;

    while (fgets(line, sizeof(line), original_file))
    {
        char current_name[MAX_NAME_LEN];
        char current_email[MAX_EMAIL_LEN];
        char current_phone[MAX_PHONE_LEN];
        char current_password[MAX_PASSWD_LEN];
        int current_payment_due;
        int current_fines_due;

        if (sscanf(line, "%[^|]|%[^|]|%[^|]|%[^|]|%d|%d", current_name, current_email, current_phone, current_password, &current_payment_due, &current_fines_due) == 6)
        {
            if (strcmp(current_email, email_to_find) == 0)
            {
                user_found = true;
                if (current_fines_due <= 0)
                {
                    strcpy(response, "Error: User has no outstanding fines.");
                    fprintf(temp_file, "%s", line);
                }
                else
                {
                    current_fines_due -= fine_amount_collected;
                    if (current_fines_due < 0)
                        current_fines_due = 0;
                    fprintf(temp_file, "%s|%s|%s|%s|%d|%d\n", current_name, current_email, current_phone, current_password, current_payment_due, current_fines_due);
                    save_fine_to_file(current_email, fine_amount_collected);
                    snprintf(response, 1024, "Success: Fine of %d collected. Remaining fine: %d.", fine_amount_collected, current_fines_due);
                }
            }
            else
            {
                fprintf(temp_file, "%s", line);
            }
        }
        else
        {
            fprintf(temp_file, "%s", line);
        }
    }

    fclose(original_file);
    fclose(temp_file);

    if (user_found)
    {
        remove(USERS_FILE);
        rename("temp_users.txt", USERS_FILE);
    }
    else
    {
        remove("temp_users.txt");
        strcpy(response, "Error: User not found.");
    }
}

void handle_borrow_book(const char *payload, char *response, const char *logged_in_email)
{
    char title[MAX_TITLE_LEN];
    char email[MAX_EMAIL_LEN];

    if (sscanf(payload, "%[^|]|%s", email, title) != 2)
    {
        strcpy(response, "Error: Invalid borrowing format.");
        return;
    }

    int book_index = find_book_by_title(title);
    int user_type;
    int user_index = find_account_by_email(email, &user_type);

    if (user_index == -1)
    {
        strcpy(response, "Error: User not found.");
        return;
    }
    if (book_index == -1)
    {
        strcpy(response, "Error: Book not found.");
        return;
    }
    if (books[book_index].copies <= 0)
    {
        strcpy(response, "Error: No copies of this book are available.");
        return;
    }
    if (accounts[user_index].data.user.fines_due > 0)
    {
        strcpy(response, "Error: User has outstanding fines and cannot borrow a book.");
        return;
    }

    books[book_index].copies--;
    borrowing_t new_borrowing;
    strcpy(new_borrowing.user_email, email);
    strcpy(new_borrowing.book_title, title);
    time_t current_time = time(NULL);
    new_borrowing.due_date_timestamp = current_time + (7 * 24 * 60 * 60);

    save_borrowing_record(&new_borrowing);
    printf("Book '%s' borrowed by '%s'. Due date: %s", title, email, ctime(&new_borrowing.due_date_timestamp));

    FILE *original_file = fopen(BOOK_FILE, "r");
    FILE *temp_file = fopen("temp_books.txt", "w");
    char line[512];
    while (fgets(line, sizeof(line), original_file))
    {
        char current_title[MAX_TITLE_LEN];
        sscanf(line, "%[^|]|", current_title);
        if (strcmp(current_title, title) == 0)
        {
            fprintf(temp_file, "%s|%s|%s|%d|%d\n", books[book_index].title, books[book_index].author, books[book_index].subject, books[book_index].price, books[book_index].copies);
        }
        else
        {
            fprintf(temp_file, "%s", line);
        }
    }
    fclose(original_file);
    fclose(temp_file);
    remove(BOOK_FILE);
    rename("temp_books.txt", BOOK_FILE);

    strcpy(response, "Success: Book borrowed successfully.");
}

void handle_return_book(const char *payload, char *response)
{
    char user_email[MAX_EMAIL_LEN];
    char book_title[MAX_TITLE_LEN];

    if (sscanf(payload, "%[^|]|%s", user_email, book_title) != 2)
    {
        strcpy(response, "Error: Invalid return format.");
        return;
    }

    FILE *original_file = fopen(BORROWINGS_FILE, "r");
    if (original_file == NULL)
    {
        strcpy(response, "Error: No books are currently borrowed.");
        return;
    }

    FILE *temp_file = fopen("temp_borrowings.txt", "w");
    if (temp_file == NULL)
    {
        perror("Error creating temporary file");
        fclose(original_file);
        strcpy(response, "Error: Server failed to process request.");
        return;
    }

    char line[512];
    bool borrowing_found = false;

    while (fgets(line, sizeof(line), original_file))
    {
        borrowing_t current_borrowing;
        if (sscanf(line, "%[^|]|%[^|]|%ld", current_borrowing.user_email, current_borrowing.book_title, (long int *)&current_borrowing.due_date_timestamp) == 3)
        {
            if (strcmp(current_borrowing.user_email, user_email) == 0 && strcmp(current_borrowing.book_title, book_title) == 0)
            {
                borrowing_found = true;
                time_t current_time = time(NULL);
                if (current_time > current_borrowing.due_date_timestamp)
                {
                    long long fine_in_seconds = current_time - current_borrowing.due_date_timestamp;
                    int fine_in_days = fine_in_seconds / (24 * 60 * 60);
                    int fine_amount = fine_in_days * 5;
                    printf("User '%s' is late returning book '%s'. Fine due: Rs. %d\n", user_email, book_title, fine_amount);

                    int user_type;
                    int user_index = find_account_by_email(user_email, &user_type);
                    if (user_index != -1 && user_type == 1)
                    {
                        accounts[user_index].data.user.fines_due += fine_amount;
                    }
                    snprintf(response, 1024, "Success: Book returned. Fine of Rs. %d due.", fine_amount);
                }
                else
                {
                    strcpy(response, "Success: Book returned on time. No fine.");
                }

                int book_index = find_book_by_title(book_title);
                if (book_index != -1)
                {
                    books[book_index].copies++;
                }
            }
            else
            {
                fprintf(temp_file, "%s", line);
            }
        }
    }

    fclose(original_file);
    fclose(temp_file);

    if (borrowing_found)
    {
        remove(BORROWINGS_FILE);
        rename("temp_borrowings.txt", BORROWINGS_FILE);
    }
    else
    {
        remove("temp_borrowings.txt");
        strcpy(response, "Error: User has not borrowed this book.");
    }
}

void handle_client_session(int client_sock)
{
    char buffer[1024];
    char logged_in_email[MAX_EMAIL_LEN] = "";
    int signed_in = 0;

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_sock, buffer, 1024, 0);
        if (bytes_received <= 0)
        {
            printf("Client disconnected.\n");
            break;
        }
        buffer[bytes_received] = '\0';
        printf("Client request: %s\n", buffer);

        char command[32];
        char payload[1024];
        char response[2048];
        memset(response, 0, sizeof(response));

        char *pipe_pos = strchr(buffer, '|');
        if (pipe_pos)
        {
            *pipe_pos = '\0';
            strcpy(command, buffer);
            strcpy(payload, pipe_pos + 1);
        }
        else
        {
            strcpy(command, buffer);
            strcpy(payload, "");
        }

        if (strcmp(command, "SIGN_UP") == 0)
        {
            handle_sign_up(payload, response);
        }
        else if (strcmp(command, "SIGN_IN") == 0)
        {
            int logged_in_type;
            handle_sign_in(payload, response, &logged_in_type, logged_in_email);
            if (strncmp(response, "Success", 7) == 0)
            {
                signed_in = 1;
            }
        }
        else if (strcmp(command, "LOGOUT") == 0)
        {
            if (signed_in)
            {
                printf("User logged out: %s\n", logged_in_email);
                signed_in = 0;
                strcpy(logged_in_email, "");
                strcpy(response, "Success: Logged out.");
            }
            else
            {
                strcpy(response, "Error: Not signed in.");
            }
        }
        else if (signed_in)
        {
            if (strcmp(command, "ADD_BOOK") == 0)
            {
                handle_add_book(payload, response);
            }
            else if (strcmp(command, "REMOVE_BOOK") == 0)
            {
                handle_remove_book(payload, response);
            }
            else if (strcmp(command, "UPDATE_INFO") == 0)
            {
                handle_update_my_info(payload, response, logged_in_email);
            }
            else if (strcmp(command, "UPDATE_BOOK") == 0)
            {
                handle_update_book(payload, response);
            }
            else if (strcmp(command, "CHECK_COPIES") == 0)
            {
                handle_check_copies(payload, response);
            }
            else if (strcmp(command, "COLLECT_PAYMENT") == 0)
            {
                handle_collect_payment(payload, response);
            }
            else if (strcmp(command, "COLLECT_FINE") == 0)
            {
                handle_collect_fine(payload, response);
            }
            else if (strcmp(command, "BORROW_BOOK") == 0)
            {
                handle_borrow_book(payload, response, logged_in_email);
            }
            else if (strcmp(command, "RETURN_BOOK") == 0)
            {
                handle_return_book(payload, response);
            }
            else if (strcmp(command, "VIEW_USERS") == 0)
            {
                handle_view_users(response);
            }
            else if (strcmp(command, "DELETE_USER") == 0)
            {
                handle_delete_user(payload, response);
            }
            else if (strcmp(command, "UPDATE_USER_INFO") == 0)
            {
                handle_update_user_info(payload, response);
            }
            else
            {
                strcpy(response, "Error: Unknown command.");
            }
        }
        else
        {
            strcpy(response, "Error: Please sign in first.");
        }

        send(client_sock, response, strlen(response), 0);
        printf("Server response: %s\n\n", response);
    }
}

int main()
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    load_accounts_from_file();
    load_books_from_file();
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERV_PORT);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", SERV_PORT);
    while (1)
    {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        printf("Connection accepted.\n");
        handle_client_session(new_socket);
        close(new_socket);
    }
    return 0;
}