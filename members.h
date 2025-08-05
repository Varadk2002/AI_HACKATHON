#ifndef MEMBERS_H
#define MEMBERS_H

#include "types.h"

// Function to create a new Member structure.
member_t member_new(int id, const char *name, const char *email,
                    const char *phone, const char *passwd, const char *role);

// Function to sign a member in. Returns a pointer to the Member if successful,
// otherwise returns NULL.
member_t *member_sign_in(const char *email, const char *passwd);

// Function to sign up a new member. Returns 0 on success, -1 on failure.
int member_sign_up(const member_t *new_member);

// Function to find all members.
int member_find_all(member_t *members, int max_members);

// Function to edit a member's profile. Returns 0 on success, -1 on failure.
int member_edit_profile(member_t *user, const char *new_email, const char *new_phone);

// Function to change a member's password. Returns 0 on success, -1 on failure.
int member_change_password(member_t *user, const char *new_passwd);

// Function to check if a member's fee is paid. Returns 1 if paid, 0 otherwise.
int member_is_paid(int member_id);

// Function to find a member by ID. Returns a pointer to the Member if found,
// otherwise returns NULL.
member_t *member_find_by_id(int member_id);

#endif