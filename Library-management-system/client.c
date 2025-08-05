#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "types.h"
#include "config.h"
#include "book.h"

// Function Prototypes
void main_menu_logged_out();
void main_menu_logged_in();

void handle_sign_in(int sock);
void handle_add_user(int sock);
void handle_add_book(int sock);
void handle_remove_book(int sock);
void handle_update_my_info(int sock);
void handle_update_book(int sock);
void handle_check_copies(int sock);
void handle_collect_payment(int sock);
void handle_view_users(int sock);
void handle_delete_user(int sock);
void handle_update_user_info(int sock);
void handle_borrow_book(int sock);
void handle_return_book(int sock);

void send_request(int sock, const char *command, const char *payload);
int receive_response(int sock, char *response);

int main()
{
    int sock = 0;
    struct sockaddr_in serv_addr;
    int choice;
    char response[2048];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Socket creation error");
        return -1;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    if (inet_pton(AF_INET, SERV_ADDR, &serv_addr.sin_addr) <= 0)
    {
        perror("Invalid address/ Address not supported");
        return -1;
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("Connection failed");
        return -1;
    }
    printf("Connected to server.\n");

    int signed_in = 0;
    while (1)
    {
        if (signed_in)
        {
            main_menu_logged_in();
            printf("Enter your choice: ");
            scanf("%d", &choice);
            switch (choice)
            {
            case 1:
                handle_add_book(sock);
                break;
            case 2:
                handle_remove_book(sock);
                break;
            case 3:
                handle_update_my_info(sock);
                break;
            case 4:
                handle_update_book(sock);
                break;
            case 5:
                handle_check_copies(sock);
                break;
            case 6:
                handle_collect_payment(sock);
                break;
            case 7:
                handle_view_users(sock);
                break;
            case 8:
                handle_update_user_info(sock);
                break;
            case 9:
                handle_delete_user(sock);
                break;
            case 10:
                handle_borrow_book(sock);
                break;
            case 11:
                handle_return_book(sock);
                break;
            case 12:
                send_request(sock, "LOGOUT", "");
                if (receive_response(sock, response) > 0)
                {
                    printf("Server response: %s\n", response);
                    signed_in = 0;
                }
                break;
            default:
                printf("Invalid choice.\n");
                break;
            }
        }
        else
        {
            main_menu_logged_out();
            printf("Enter your choice: ");
            scanf("%d", &choice);
            switch (choice)
            {
            case 1:
                handle_sign_in(sock);
                if (receive_response(sock, response) > 0)
                {
                    printf("Server response: %s\n", response);
                    if (strncmp(response, "Success", 7) == 0)
                    {
                        signed_in = 1;
                    }
                }
                break;
            case 2:
                handle_add_user(sock);
                break;
            case 3:
                printf("Exiting.\n");
                close(sock);
                return 0;
            default:
                printf("Invalid choice.\n");
                break;
            }
        }
    }
    close(sock);
    return 0;
}

void main_menu_logged_out()
{
    printf("\n--- Library Management System ---\n");
    printf("1. Sign In\n");
    printf("2. Add User\n");
    printf("3. Exit\n");
}

void main_menu_logged_in()
{
    printf("\n--- Librarian Menu ---\n");
    printf("1. Add a Book\n");
    printf("2. Remove a Book\n");
    printf("3. Update My Information\n");
    printf("4. Update Book Information\n");
    printf("5. Check Book Copies\n");
    printf("6. Collect Payment\n");
    printf("7. View All Users\n");
    printf("8. Update User Information\n");
    printf("9. Delete a User\n");
    printf("10. Borrow a Book\n");
    printf("11. Return a Book\n");
    printf("12. Logout\n");
}

void handle_sign_in(int sock)
{
    char email[MAX_EMAIL_LEN];
    char password[MAX_PASSWD_LEN];
    char payload[1024];
    printf("Enter email: ");
    scanf("%s", email);
    printf("Enter password: ");
    scanf("%s", password);
    snprintf(payload, sizeof(payload), "%s|%s", email, password);
    send_request(sock, "SIGN_IN", payload);
}

void handle_add_user(int sock)
{
    char name[MAX_NAME_LEN];
    char email[MAX_EMAIL_LEN];
    char phone[MAX_PHONE_LEN];
    char password[MAX_PASSWD_LEN];
    int payment;
    char payload[1024];
    char response[1024];

    printf("Enter name: ");
    scanf(" %[^\n]", name);
    printf("Enter email: ");
    scanf(" %[^\n]", email);
    printf("Enter phone: ");
    scanf(" %[^\n]", phone);
    printf("Enter password: ");
    scanf(" %[^\n]", password);
    printf("Enter monthly payment amount (0 for default user): ");
    scanf("%d", &payment);

    snprintf(payload, sizeof(payload), "%s|%s|%s|%s|%d", name, email, phone, password, payment);
    send_request(sock, "SIGN_UP", payload);
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void handle_add_book(int sock)
{
    char title[MAX_TITLE_LEN];
    char author[MAX_AUTHOR_LEN];
    char subject[MAX_SUBJECT_LEN];
    int price;
    int copies;
    char payload[1024];
    char response[1024];

    printf("Enter book title: ");
    scanf(" %[^\n]", title);
    printf("Enter book author: ");
    scanf(" %[^\n]", author);
    printf("Enter book subject: ");
    scanf(" %[^\n]", subject);
    printf("Enter book price: ");
    scanf("%d", &price);
    printf("Enter number of copies: ");
    scanf("%d", &copies);

    snprintf(payload, sizeof(payload), "%s|%s|%s|%d|%d", title, author, subject, price, copies);
    send_request(sock, "ADD_BOOK", payload);
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void handle_remove_book(int sock)
{
    char title[MAX_TITLE_LEN];
    char payload[1024];
    char response[1024];
    printf("Enter the title of the book to remove: ");
    scanf(" %[^\n]", title);
    snprintf(payload, sizeof(payload), "%s", title);
    send_request(sock, "REMOVE_BOOK", payload);
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void handle_update_my_info(int sock)
{
    char name[MAX_NAME_LEN];
    char email[MAX_EMAIL_LEN];
    char phone[MAX_PHONE_LEN];
    char password[MAX_PASSWD_LEN];
    char payload[1024];
    char response[1024];

    printf("Enter new name: ");
    scanf(" %[^\n]", name);
    printf("Enter new email: ");
    scanf(" %s", email);
    printf("Enter new phone: ");
    scanf(" %s", phone);
    printf("Enter new password: ");
    scanf(" %s", password);

    snprintf(payload, sizeof(payload), "%s|%s|%s|%s", name, email, phone, password);
    send_request(sock, "UPDATE_INFO", payload);
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void handle_update_book(int sock)
{
    char old_title[MAX_TITLE_LEN];
    char new_title[MAX_TITLE_LEN];
    char new_author[MAX_AUTHOR_LEN];
    char new_subject[MAX_SUBJECT_LEN];
    int new_price;
    int new_copies;
    char payload[1024];
    char response[1024];

    printf("Enter the title of the book to update: ");
    scanf(" %[^\n]", old_title);
    printf("Enter new title: ");
    scanf(" %[^\n]", new_title);
    printf("Enter new author: ");
    scanf(" %[^\n]", new_author);
    printf("Enter new subject: ");
    scanf(" %[^\n]", new_subject);
    printf("Enter new price: ");
    scanf("%d", &new_price);
    printf("Enter new number of copies: ");
    scanf("%d", &new_copies);

    snprintf(payload, sizeof(payload), "%s|%s|%s|%s|%d|%d", old_title, new_title, new_author, new_subject, new_price, new_copies);
    send_request(sock, "UPDATE_BOOK", payload);
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void handle_check_copies(int sock)
{
    char title[MAX_TITLE_LEN];
    char payload[1024];
    char response[1024];
    printf("Enter the title of the book to check: ");
    scanf(" %[^\n]", title);
    snprintf(payload, sizeof(payload), "%s", title);
    send_request(sock, "CHECK_COPIES", payload);
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void handle_collect_payment(int sock)
{
    char email[MAX_EMAIL_LEN];
    int amount;
    char payload[1024];
    char response[1024];

    printf("Enter user's email: ");
    scanf(" %s", email);
    printf("Enter payment amount: ");
    scanf("%d", &amount);

    snprintf(payload, sizeof(payload), "%s|%d", email, amount);
    send_request(sock, "COLLECT_PAYMENT", payload);
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void handle_view_users(int sock)
{
    char response[2048];
    send_request(sock, "VIEW_USERS", "");
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void handle_delete_user(int sock)
{
    char email[MAX_EMAIL_LEN];
    char payload[1024];
    char response[1024];
    printf("Enter the email of the user to delete: ");
    scanf(" %s", email);
    snprintf(payload, sizeof(payload), "%s", email);
    send_request(sock, "DELETE_USER", payload);
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void handle_update_user_info(int sock)
{
    char target_email[MAX_EMAIL_LEN];
    char name[MAX_NAME_LEN];
    char email[MAX_EMAIL_LEN];
    char phone[MAX_PHONE_LEN];
    char password[MAX_PASSWD_LEN];
    char payload[1024];
    char response[1024];

    printf("Enter the email of the user to update: ");
    scanf(" %s", target_email);
    printf("Enter new name for %s: ", target_email);
    scanf(" %[^\n]", name);
    printf("Enter new email for %s: ", target_email);
    scanf(" %s", email);
    printf("Enter new phone for %s: ", target_email);
    scanf(" %s", phone);
    printf("Enter new password for %s: ", target_email);
    scanf(" %s", password);

    snprintf(payload, sizeof(payload), "%s|%s|%s|%s|%s", target_email, name, email, phone, password);
    send_request(sock, "UPDATE_USER_INFO", payload);
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void handle_borrow_book(int sock)
{
    char title[MAX_TITLE_LEN];
    char email[MAX_EMAIL_LEN];
    char payload[1024];
    char response[1024];

    printf("Enter user's email: ");
    scanf(" %s", email);
    printf("Enter the title of the book to borrow: ");
    scanf(" %[^\n]", title);

    snprintf(payload, sizeof(payload), "%s|%s", email, title);
    send_request(sock, "BORROW_BOOK", payload);
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void handle_return_book(int sock)
{
    char title[MAX_TITLE_LEN];
    char email[MAX_EMAIL_LEN];
    char payload[1024];
    char response[1024];

    printf("Enter user's email: ");
    scanf(" %s", email);
    printf("Enter the title of the book to return: ");
    scanf(" %[^\n]", title);

    snprintf(payload, sizeof(payload), "%s|%s", email, title);
    send_request(sock, "RETURN_BOOK", payload);
    if (receive_response(sock, response) > 0)
    {
        printf("Server response: %s\n", response);
    }
}

void send_request(int sock, const char *command, const char *payload)
{
    char request[1024];
    snprintf(request, sizeof(request), "%s|%s", command, payload);
    send(sock, request, strlen(request), 0);
}

int receive_response(int sock, char *response)
{
    int bytes_received = recv(sock, response, 1024 - 1, 0);
    if (bytes_received > 0)
    {
        response[bytes_received] = '\0';
    }
    return bytes_received;
}