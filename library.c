#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_TITLE_LENGTH 100
#define MAX_AUTHOR_LENGTH 50
#define MAX_GENRE_LENGTH 30
#define MAX_ISBN_LENGTH 20
#define MAX_NAME_LENGTH 50
#define HASH_TABLE_SIZE 101
#define MAX_USERS 100 // Defined but not strictly enforced by linked list size
#define MAX_BOOKS 500 // Defined but not strictly enforced by hash table size
#define MAX_BORROWED 10

// Define structures

// Book structure
typedef struct Book {
    char isbn[MAX_ISBN_LENGTH];
    char title[MAX_TITLE_LENGTH];
    char author[MAX_AUTHOR_LENGTH];
    char genre[MAX_GENRE_LENGTH];
    int available;
    int borrow_count; // For tracking popularity
    struct Book *next; // For hash table collision handling via chaining
} Book;

// User structure
typedef struct User {
    int id;
    char name[MAX_NAME_LENGTH];
    char borrowed_books[MAX_BORROWED][MAX_ISBN_LENGTH]; // Queue implementation for borrowed books
    int borrowed_count;
    struct User *next; // For linked list implementation
} User;

// Binary Search Tree Node for efficient book lookup
typedef struct TreeNode {
    Book *book; // Pointer to a Book in the hash table
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

// Global variables
Book *hash_table[HASH_TABLE_SIZE] = {NULL}; // Hash table for books
User *user_list = NULL; // Linked list for users
TreeNode *title_bst_root = NULL; // BST for book lookup by title
int next_user_id = 1001; // Starting ID for users

// Function prototypes

// Hash table functions
unsigned int hash_function(char *isbn);
void insert_book(Book *new_book);
Book* search_book_by_isbn(char *isbn);
void remove_book(char *isbn); 

// User linked list functions
void add_user(char *name);
User* find_user(int id);
void remove_user(int id); 

// BST functions
void insert_into_bst(Book *book);
TreeNode* create_tree_node(Book *book);
TreeNode* search_by_title(TreeNode *root, char *title);
void inorder_traversal(TreeNode *root);

// Issue & Return functions
int issue_book(int user_id, char *isbn);
int return_book(int user_id, char *isbn);

// Report generation functions
void list_all_books();
void list_available_books();
void list_borrowed_books();
void list_most_borrowed_books();
void list_active_users();

// Menu functions
void display_menu();
void book_management_menu();
void user_management_menu();
void issue_return_menu();
void search_menu();
void report_menu();

// Helper functions
void read_string(char *buffer, int length);
void clear_input_buffer();

// File I/O functions for persistence
void save_books_to_file(const char *filename);
void load_books_from_file(const char *filename);
void save_users_to_file(const char *filename);
void load_users_from_file(const char *filename);

// Memory freeing functions
void free_all_books();
void free_bst_nodes(TreeNode *root); // Helper for freeing BST
void free_all_users();


// Main function
int main() {
    int choice;

    printf("\n===== Smart Library Management System =====\n");

    // Load data at startup
    load_books_from_file("books.dat");
    load_users_from_file("users.dat");

    do {
        display_menu();
        printf("Enter your choice: ");
        scanf("%d", &choice);
        clear_input_buffer();

        switch(choice) {
            case 1:
                book_management_menu();
                break;
            case 2:
                user_management_menu();
                break;
            case 3:
                issue_return_menu();
                break;
            case 4:
                search_menu();
                break;
            case 5:
                report_menu();
                break;
            case 0:
                printf("Exiting the system. Saving data...\n");
                save_books_to_file("books.dat");
                save_users_to_file("users.dat");
                printf("Data saved. Thank you!\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }

    } while(choice != 0);

    // Free allocated memory before exit
    free_all_books();
    free_all_users();

    return 0;
}

// --- Hash Table Functions ---

// Hash function implementation
unsigned int hash_function(char *isbn) {
    unsigned int hash = 0;
    while (*isbn) {
        hash = (hash * 31) + (*isbn++);
    }
    return hash % HASH_TABLE_SIZE;
}

// Insert a book into the hash table
void insert_book(Book *new_book) {
    unsigned int index = hash_function(new_book->isbn);

    // Check if book with the same ISBN already exists
    Book *current = hash_table[index];
    while (current != NULL) {
        if (strcmp(current->isbn, new_book->isbn) == 0) {
            printf("Book with ISBN %s already exists. Not adding duplicate.\n", new_book->isbn);
            free(new_book); // Free the newly allocated book if it's a duplicate
            return;
        }
        current = current->next;
    }

    // Add book to the beginning of the chain at this index
    new_book->next = hash_table[index];
    hash_table[index] = new_book;

    // Also add to BST for title-based searching
    insert_into_bst(new_book);

    printf("Book '%s' added successfully.\n", new_book->title);
}

// Search for a book by ISBN
Book* search_book_by_isbn(char *isbn) {
    unsigned int index = hash_function(isbn);
    Book *current = hash_table[index];

    while (current != NULL) {
        if (strcmp(current->isbn, isbn) == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL; // Book not found
}

// Remove a book by ISBN
void remove_book(char *isbn) {
    unsigned int index = hash_function(isbn);
    Book *current = hash_table[index];
    Book *prev = NULL;

    // Search for the book in the hash table chain
    while (current != NULL && strcmp(current->isbn, isbn) != 0) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("Book with ISBN %s not found.\n", isbn);
        return;
    }

    // Check if the book is currently borrowed
    if (!current->available) {
        printf("Cannot remove book '%s' (ISBN: %s) as it is currently borrowed.\n", current->title, isbn);
        return;
    }

    // Remove from hash table
    if (prev == NULL) { // Book is the head of the chain
        hash_table[index] = current->next;
    } else {
        prev->next = current->next;
    }

    // Remove from BST

    printf("Book '%s' (ISBN: %s) removed successfully.\n", current->title, current->isbn);
    free(current); // Free the memory allocated for the book
}


// --- BST Functions ---

// BST node creation
TreeNode* create_tree_node(Book *book) {
    TreeNode *new_node = (TreeNode*)malloc(sizeof(TreeNode));
    if (new_node == NULL) {
        printf("Memory allocation failed for tree node.\n");
        exit(1);
    }

    new_node->book = book;
    new_node->left = NULL;
    new_node->right = NULL;

    return new_node;
}

// Insert a book into the BST
void insert_into_bst(Book *book) {
    if (title_bst_root == NULL) {
        title_bst_root = create_tree_node(book);
        return;
    }

    TreeNode *current = title_bst_root;
    TreeNode *parent = NULL;
    int comparison;

    while (current != NULL) {
        parent = current;
        comparison = strcmp(book->title, current->book->title);

        if (comparison < 0) {
            current = current->left;
        } else if (comparison > 0) {
            current = current->right;
        } else {
            // Title already exists, decide how to handle.
            current = current->right;
        }
    }

    TreeNode *new_node = create_tree_node(book);

    if (comparison < 0) {
        parent->left = new_node;
    } else { // comparison > 0
        parent->right = new_node;
    }
}

// Search for a book by title in the BST
TreeNode* search_by_title(TreeNode *root, char *title) {
    if (root == NULL) {
        return NULL;
    }

    int comparison = strcmp(title, root->book->title);
    if (comparison == 0) {
        return root; // Found a book with the matching title
    } else if (comparison < 0) {
        return search_by_title(root->left, title);
    } else {
        return search_by_title(root->right, title);
    }
}

// Inorder traversal of BST (for listing books in alphabetical order by title)
void inorder_traversal(TreeNode *root) {
    if (root != NULL) {
        inorder_traversal(root->left);
        printf("Title: %-30s | Author: %-20s | ISBN: %-15s | Status: %s\n",
               root->book->title, root->book->author, root->book->isbn,
               root->book->available ? "Available" : "Borrowed");
        inorder_traversal(root->right);
    }
}

// --- User Linked List Functions ---

// Add new user to the linked list
void add_user(char *name) {
    User *new_user = (User*)malloc(sizeof(User));
    if (new_user == NULL) {
        printf("Memory allocation failed for user.\n");
        return;
    }

    new_user->id = next_user_id++;
    strcpy(new_user->name, name);
    new_user->borrowed_count = 0;
    new_user->next = NULL;

    // Add to the beginning of the linked list
    if (user_list == NULL) {
        user_list = new_user;
    } else {
        new_user->next = user_list;
        user_list = new_user;
    }

    printf("User '%s' added successfully with ID: %d\n", name, new_user->id);
}

// Find a user by ID
User* find_user(int id) {
    User *current = user_list;

    while (current != NULL) {
        if (current->id == id) {
            return current;
        }
        current = current->next;
    }

    return NULL; // User not found
}

// Remove a user by ID
void remove_user(int id) {
    User *current = user_list;
    User *prev = NULL;

    while (current != NULL && current->id != id) {
        prev = current;
        current = current->next;
    }

    if (current == NULL) {
        printf("User with ID %d not found.\n", id);
        return;
    }

    // Check if the user has any borrowed books
    if (current->borrowed_count > 0) {
        printf("Cannot remove user '%s' (ID: %d) as they still have borrowed books.\n", current->name, current->id);
        return;
    }

    // Remove from linked list
    if (prev == NULL) { // User is the head of the list
        user_list = current->next;
    } else {
        prev->next = current->next;
    }

    printf("User '%s' (ID: %d) removed successfully.\n", current->name, current->id);
    free(current); // Free the memory allocated for the user
}


// --- Issue & Return Functions ---

// Issue a book to a user
int issue_book(int user_id, char *isbn) {
    User *user = find_user(user_id);
    if (user == NULL) {
        printf("User ID %d not found.\n", user_id);
        return 0;
    }

    Book *book = search_book_by_isbn(isbn);
    if (book == NULL) {
        printf("Book with ISBN %s not found.\n", isbn);
        return 0;
    }

    if (!book->available) {
        printf("Book '%s' is not available for borrowing.\n", book->title);
        return 0;
    }

    if (user->borrowed_count >= MAX_BORROWED) {
        printf("User '%s' has reached the maximum number of books that can be borrowed (%d).\n", user->name, MAX_BORROWED);
        return 0;
    }

    // Add book to user's borrowed list
    strcpy(user->borrowed_books[user->borrowed_count++], isbn);

    // Update book availability
    book->available = 0;
    book->borrow_count++;

    printf("Book '%s' issued to user '%s' successfully.\n", book->title, user->name);
    return 1;
}

// Return a book
int return_book(int user_id, char *isbn) {
    User *user = find_user(user_id);
    if (user == NULL) {
        printf("User ID %d not found.\n", user_id);
        return 0;
    }

    Book *book = search_book_by_isbn(isbn);
    if (book == NULL) {
        printf("Book with ISBN %s not found.\n", isbn);
        return 0;
    }

    // Check if user has borrowed this book
    int found_idx = -1;
    for (int i = 0; i < user->borrowed_count; i++) {
        if (strcmp(user->borrowed_books[i], isbn) == 0) {
            found_idx = i;
            break;
        }
    }

    if (found_idx == -1) {
        printf("User '%s' has not borrowed book with ISBN %s.\n", user->name, isbn);
        return 0;
    }

    // Remove book from user's borrowed list by shifting elements
    for (int i = found_idx; i < user->borrowed_count - 1; i++) {
        strcpy(user->borrowed_books[i], user->borrowed_books[i + 1]);
    }
    user->borrowed_count--;

    // Update book availability
    book->available = 1;

    printf("Book '%s' returned by user '%s' successfully.\n", book->title, user->name);
    return 1;
}

// --- Report Generation Functions ---

// List all books
void list_all_books() {
    printf("\n===== All Books =====\n");
    printf("%-30s | %-20s | %-15s | %-10s\n", "Title", "Author", "ISBN", "Status");
    printf("-------------------------------------------------------------------------------------\n");

    if (title_bst_root == NULL) {
        printf("No books in the library.\n");
        return;
    }
    // Use BST inorder traversal for alphabetical listing
    inorder_traversal(title_bst_root);
}

// List available books
void list_available_books() {
    printf("\n===== Available Books =====\n");
    printf("%-30s | %-20s | %-15s\n", "Title", "Author", "ISBN");
    printf("--------------------------------------------------------------------\n");

    int count = 0;
    // Iterate through the hash table to find available books
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Book *current = hash_table[i];
        while (current != NULL) {
            if (current->available) {
                printf("%-30s | %-20s | %-15s\n",
                       current->title, current->author, current->isbn);
                count++;
            }
            current = current->next;
        }
    }

    if (count == 0) {
        printf("No available books in the library.\n");
    }
}

// List borrowed books
void list_borrowed_books() {
    printf("\n===== Currently Borrowed Books =====\n");
    printf("%-30s | %-20s | %-15s | %-20s\n", "Title", "Author", "ISBN", "Borrowed By");
    printf("-------------------------------------------------------------------------------------\n");

    int count = 0;
    User *user = user_list;

    while (user != NULL) {
        for (int i = 0; i < user->borrowed_count; i++) {
            Book *book = search_book_by_isbn(user->borrowed_books[i]);
            if (book != NULL) { // Should always be found if the ISBN is valid
                printf("%-30s | %-20s | %-15s | %-20s (ID: %d)\n",
                       book->title, book->author, book->isbn, user->name, user->id);
                count++;
            }
        }
        user = user->next;
    }

    if (count == 0) {
        printf("No books are currently borrowed.\n");
    }
}

// List most borrowed books (simple implementation using bubble sort)
void list_most_borrowed_books() {
    printf("\n===== Most Borrowed Books =====\n");
    printf("%-30s | %-20s | %-15s | %-10s\n", "Title", "Author", "ISBN", "Borrows");
    printf("-------------------------------------------------------------------------------------\n");

    // Create an array of pointers to books for sorting
    Book *books[MAX_BOOKS];
    int book_count = 0;

    // Collect all books from hash table
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Book *current = hash_table[i];
        while (current != NULL && book_count < MAX_BOOKS) {
            books[book_count++] = current;
            current = current->next;
        }
    }

    if (book_count == 0) {
        printf("No books in the library.\n");
        return;
    }

    // Simple bubble sort by borrow count (descending)
    for (int i = 0; i < book_count - 1; i++) {
        for (int j = 0; j < book_count - i - 1; j++) {
            if (books[j]->borrow_count < books[j + 1]->borrow_count) {
                Book *temp = books[j];
                books[j] = books[j + 1];
                books[j + 1] = temp;
            }
        }
    }

    // Display top books (at most 10)
    int display_limit = 10;
    if (book_count < display_limit) {
        display_limit = book_count;
    }

    int displayed_any = 0;
    for (int i = 0; i < display_limit; i++) {
        if (books[i]->borrow_count > 0) {
            printf("%-30s | %-20s | %-15s | %-10d\n",
                   books[i]->title, books[i]->author, books[i]->isbn, books[i]->borrow_count);
            displayed_any = 1;
        }
    }

    if (!displayed_any) {
        printf("No books have been borrowed yet.\n");
    }
}

// List active users
void list_active_users() {
    printf("\n===== Active Users =====\n");
    printf("%-5s | %-20s | %-15s\n", "ID", "Name", "Books Borrowed");
    printf("--------------------------------------------\n");

    User *current = user_list;
    int count = 0;

    // Create a temporary array of active users for sorting if needed,
    User *active_users[MAX_USERS]; // Max 100 users
    int active_user_count = 0;

    while (current != NULL && active_user_count < MAX_USERS) {
        if (current->borrowed_count > 0) {
            active_users[active_user_count++] = current;
        }
        current = current->next;
    }

    if (active_user_count == 0) {
        printf("No active users at the moment.\n");
        return;
    }

    // Simple bubble sort by borrowed_count (descending) for active users
    for (int i = 0; i < active_user_count - 1; i++) {
        for (int j = 0; j < active_user_count - i - 1; j++) {
            if (active_users[j]->borrowed_count < active_users[j + 1]->borrowed_count) {
                User *temp = active_users[j];
                active_users[j] = active_users[j + 1];
                active_users[j + 1] = temp;
            }
        }
    }

    // Display active users
    for (int i = 0; i < active_user_count; i++) {
        printf("%-5d | %-20s | %-15d\n",
               active_users[i]->id, active_users[i]->name, active_users[i]->borrowed_count);
        count++;
    }

    if (count == 0) { 
        printf("No active users at the moment.\n");
    }
}


// --- Menu Functions ---

void display_menu() {
    printf("\n===== Main Menu =====\n");
    printf("1. Book Management\n");
    printf("2. User Management\n");
    printf("3. Issue/Return Books\n");
    printf("4. Search\n");
    printf("5. Reports\n");
    printf("0. Exit\n");
}

void book_management_menu() {
    int choice;

    do {
        printf("\n===== Book Management =====\n");
        printf("1. Add New Book\n");
        printf("2. Remove Book\n");
        printf("3. List All Books\n");
        printf("0. Back to Main Menu\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        clear_input_buffer();

        switch(choice) {
            case 1: {
                Book *new_book = (Book*)malloc(sizeof(Book));
                if (new_book == NULL) {
                    printf("Memory allocation failed.\n");
                    break;
                }

                printf("Enter ISBN: ");
                read_string(new_book->isbn, MAX_ISBN_LENGTH);

                printf("Enter Title: ");
                read_string(new_book->title, MAX_TITLE_LENGTH);

                printf("Enter Author: ");
                read_string(new_book->author, MAX_AUTHOR_LENGTH);

                printf("Enter Genre: ");
                read_string(new_book->genre, MAX_GENRE_LENGTH);

                new_book->available = 1;
                new_book->borrow_count = 0;
                new_book->next = NULL;

                insert_book(new_book);
                break;
            }
            case 2: {
                char isbn[MAX_ISBN_LENGTH];
                printf("Enter ISBN of the book to remove: ");
                read_string(isbn, MAX_ISBN_LENGTH);

                remove_book(isbn);
                break;
            }
            case 3:
                list_all_books();
                break;
            case 0:
                printf("Returning to main menu.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while(choice != 0);
}

void user_management_menu() {
    int choice;

    do {
        printf("\n===== User Management =====\n");
        printf("1. Add New User\n");
        printf("2. Find User\n");
        printf("3. Remove User\n");
        printf("4. List All Users\n");
        printf("0. Back to Main Menu\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        clear_input_buffer();

        switch(choice) {
            case 1: {
                char name[MAX_NAME_LENGTH];
                printf("Enter user name: ");
                read_string(name, MAX_NAME_LENGTH);
                add_user(name);
                break;
            }
            case 2: {
                int id;
                printf("Enter user ID: ");
                scanf("%d", &id);
                clear_input_buffer();

                User *user = find_user(id);
                if (user != NULL) {
                    printf("\nUser Found:\n");
                    printf("ID: %d\n", user->id);
                    printf("Name: %s\n", user->name);
                    printf("Books borrowed: %d\n", user->borrowed_count);

                    if (user->borrowed_count > 0) {
                        printf("\nBorrowed Books:\n");
                        for (int i = 0; i < user->borrowed_count; i++) {
                            Book *book = search_book_by_isbn(user->borrowed_books[i]);
                            if (book != NULL) {
                                printf("%d. %s by %s (ISBN: %s)\n", i+1, book->title, book->author, book->isbn);
                            }
                        }
                    }
                } else {
                    printf("User with ID %d not found.\n", id);
                }
                break;
            }
            case 3: {
                int id;
                printf("Enter user ID to remove: ");
                scanf("%d", &id);
                clear_input_buffer();

                remove_user(id);
                break;
            }
            case 4: {
                printf("\n===== All Users =====\n");
                printf("%-5s | %-20s | %-15s\n", "ID", "Name", "Books Borrowed");
                printf("--------------------------------------------\n");

                User *current = user_list;
                int count = 0;

                if (user_list == NULL) {
                    printf("No users registered in the system.\n");
                    break;
                }

                while (current != NULL) {
                    printf("%-5d | %-20s | %-15d\n",
                           current->id, current->name, current->borrowed_count);
                    count++;
                    current = current->next;
                }
                break;
            }
            case 0:
                printf("Returning to main menu.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while(choice != 0);
}

void issue_return_menu() {
    int choice;

    do {
        printf("\n===== Issue/Return Books =====\n");
        printf("1. Issue Book\n");
        printf("2. Return Book\n");
        printf("0. Back to Main Menu\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        clear_input_buffer();

        switch(choice) {
            case 1: {
                int user_id;
                char isbn[MAX_ISBN_LENGTH];

                printf("Enter User ID: ");
                scanf("%d", &user_id);
                clear_input_buffer();

                printf("Enter ISBN of the book to issue: ");
                read_string(isbn, MAX_ISBN_LENGTH);

                issue_book(user_id, isbn);
                break;
            }
            case 2: {
                int user_id;
                char isbn[MAX_ISBN_LENGTH];

                printf("Enter User ID: ");
                scanf("%d", &user_id);
                clear_input_buffer();

                printf("Enter ISBN of the book to return: ");
                read_string(isbn, MAX_ISBN_LENGTH);

                return_book(user_id, isbn);
                break;
            }
            case 0:
                printf("Returning to main menu.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while(choice != 0);
}

void search_menu() {
    int choice;

    do {
        printf("\n===== Search =====\n");
        printf("1. Search by ISBN\n");
        printf("2. Search by Title\n");
        printf("3. Search by Author\n");
        printf("0. Back to Main Menu\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        clear_input_buffer();

        switch(choice) {
            case 1: {
                char isbn[MAX_ISBN_LENGTH];
                printf("Enter ISBN: ");
                read_string(isbn, MAX_ISBN_LENGTH);

                Book *book = search_book_by_isbn(isbn);
                if (book != NULL) {
                    printf("\nBook Found:\n");
                    printf("ISBN: %s\n", book->isbn);
                    printf("Title: %s\n", book->title);
                    printf("Author: %s\n", book->author);
                    printf("Genre: %s\n", book->genre);
                    printf("Status: %s\n", book->available ? "Available" : "Borrowed");
                    printf("Times borrowed: %d\n", book->borrow_count);
                } else {
                    printf("Book with ISBN %s not found.\n", isbn);
                }
                break;
            }
            case 2: {
                char title[MAX_TITLE_LENGTH];
                printf("Enter Title: ");
                read_string(title, MAX_TITLE_LENGTH);

                TreeNode *result_node = search_by_title(title_bst_root, title);
                if (result_node != NULL && result_node->book != NULL) {
                    printf("\nBook Found:\n");
                    printf("ISBN: %s\n", result_node->book->isbn);
                    printf("Title: %s\n", result_node->book->title);
                    printf("Author: %s\n", result_node->book->author);
                    printf("Genre: %s\n", result_node->book->genre);
                    printf("Status: %s\n", result_node->book->available ? "Available" : "Borrowed");
                    printf("Times borrowed: %d\n", result_node->book->borrow_count);
                } else {
                    printf("Book with title '%s' not found.\n", title);
                }
                break;
            }
            case 3: {
                char author[MAX_AUTHOR_LENGTH];
                printf("Enter Author: ");
                read_string(author, MAX_AUTHOR_LENGTH);

                printf("\nBooks by %s:\n", author);
                printf("%-30s | %-15s | %-10s\n", "Title", "ISBN", "Status");
                printf("------------------------------------------------------------\n");

                int found = 0;
                for (int i = 0; i < HASH_TABLE_SIZE; i++) {
                    Book *current = hash_table[i];
                    while (current != NULL) {
                        if (strcmp(current->author, author) == 0) {
                            printf("%-30s | %-15s | %-10s\n",
                                   current->title, current->isbn,
                                   current->available ? "Available" : "Borrowed");
                            found = 1;
                        }
                        current = current->next;
                    }
                }

                if (!found) {
                    printf("No books found by author '%s'.\n", author);
                }
                break;
            }
            case 0:
                printf("Returning to main menu.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while(choice != 0);
}

void report_menu() {
    int choice;

    do {
        printf("\n===== Reports =====\n");
        printf("1. List All Books\n");
        printf("2. List Available Books\n");
        printf("3. List Borrowed Books\n");
        printf("4. List Most Borrowed Books\n");
        printf("5. List Active Users\n");
        printf("0. Back to Main Menu\n");
        printf("Enter your choice: ");
        scanf("%d", &choice);
        clear_input_buffer();

        switch(choice) {
            case 1:
                list_all_books();
                break;
            case 2:
                list_available_books();
                break;
            case 3:
                list_borrowed_books();
                break;
            case 4:
                list_most_borrowed_books();
                break;
            case 5:
                list_active_users();
                break;
            case 0:
                printf("Returning to main menu.\n");
                break;
            default:
                printf("Invalid choice. Please try again.\n");
        }
    } while(choice != 0);
}

// --- Helper Functions ---

// Helper function to read a string with spaces
void read_string(char *buffer, int length) {
    fgets(buffer, length, stdin);

    // Remove trailing newline if present
    int len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') {
        buffer[len-1] = '\0';
    }
}

// Helper function to clear input buffer
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}


// --- File I/O Functions ---

// Function to save all books to a file
void save_books_to_file(const char *filename) {
    FILE *file = fopen(filename, "w"); // Open in write mode, overwriting existing file
    if (file == NULL) {
        perror("Error opening books file for writing");
        return;
    }

    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Book *current = hash_table[i];
        while (current != NULL) {
            // Write book details in a delimited format (e.g., pipe '|')
            fprintf(file, "%s|%s|%s|%s|%d|%d\n",
                    current->isbn,
                    current->title,
                    current->author,
                    current->genre,
                    current->available,
                    current->borrow_count);
            current = current->next;
        }
    }

    fclose(file);
    
}

// Function to load books from a file
void load_books_from_file(const char *filename) {
    FILE *file = fopen(filename, "r"); // Open in read mode
    if (file == NULL) {
        return;
    }

    char line[512]; // A buffer to read each line
    while (fgets(line, sizeof(line), file) != NULL) {
        // Remove trailing newline character
        line[strcspn(line, "\n")] = '\0';

        Book *new_book = (Book*)malloc(sizeof(Book));
        if (new_book == NULL) {
            printf("Memory allocation failed during book loading.\n");
            fclose(file);
            return;
        }

        // Parse the line using strtok
        char *token = strtok(line, "|");
        if (token != NULL) strcpy(new_book->isbn, token); else { free(new_book); continue; }
        token = strtok(NULL, "|");
        if (token != NULL) strcpy(new_book->title, token); else { free(new_book); continue; }
        token = strtok(NULL, "|");
        if (token != NULL) strcpy(new_book->author, token); else { free(new_book); continue; }
        token = strtok(NULL, "|");
        if (token != NULL) strcpy(new_book->genre, token); else { free(new_book); continue; }
        token = strtok(NULL, "|");
        if (token != NULL) new_book->available = atoi(token); else { free(new_book); continue; }
        token = strtok(NULL, "|");
        if (token != NULL) new_book->borrow_count = atoi(token); else { free(new_book); continue; }

        new_book->next = NULL; // Will be set correctly by insert_book

        // Insert the book into the hash table
        unsigned int index = hash_function(new_book->isbn);
        new_book->next = hash_table[index];
        hash_table[index] = new_book;

        // Also add to BST for title-based searching
        insert_into_bst(new_book);
    }

    fclose(file);
    
}

// Function to save all users to a file
void save_users_to_file(const char *filename) {
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Error opening users file for writing");
        return;
    }

    User *current = user_list;
    while (current != NULL) {
        // Write user details
        fprintf(file, "%d|%s|%d", current->id, current->name, current->borrowed_count);
        // Write borrowed books
        for (int i = 0; i < current->borrowed_count; i++) {
            fprintf(file, "|%s", current->borrowed_books[i]);
        }
        fprintf(file, "\n");
        current = current->next;
    }

    fclose(file);
    
}

// Function to load users from a file
void load_users_from_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return;
    }

    char line[1024]; // Larger buffer for user lines due to borrowed books
    int current_max_id = 1000; // Track max ID to correctly set next_user_id

    // Initialize user_list to NULL
    User *temp_user_list = NULL;

    while (fgets(line, sizeof(line), file) != NULL) {
        line[strcspn(line, "\n")] = '\0';

        User *new_user = (User*)malloc(sizeof(User));
        if (new_user == NULL) {
            printf("Memory allocation failed during user loading.\n");
            fclose(file);
            return;
        }
        new_user->next = NULL;

        char *token;
        char *rest_of_line = line; 

        token = strtok_r(rest_of_line, "|", &rest_of_line);
        if (token != NULL) new_user->id = atoi(token); else { free(new_user); continue; }

        token = strtok_r(rest_of_line, "|", &rest_of_line);
        if (token != NULL) strcpy(new_user->name, token); else { free(new_user); continue; }

        token = strtok_r(rest_of_line, "|", &rest_of_line);
        if (token != NULL) new_user->borrowed_count = atoi(token); else { free(new_user); continue; }

        for (int i = 0; i < new_user->borrowed_count; i++) {
            token = strtok_r(rest_of_line, "|", &rest_of_line);
            if (token != NULL) strcpy(new_user->borrowed_books[i], token); else { free(new_user); continue; }
        }

        // Add to the beginning of the temporary linked list
        new_user->next = temp_user_list;
        temp_user_list = new_user;

        if (new_user->id > current_max_id) {
            current_max_id = new_user->id;
        }
    }
    next_user_id = current_max_id + 1; // Set next_user_id correctly

    // Reverse the temporary list to maintain original order 
    user_list = NULL;
    User *current_temp = temp_user_list;
    while(current_temp != NULL) {
        User *node_to_move = current_temp;
        current_temp = current_temp->next;

        node_to_move->next = user_list;
        user_list = node_to_move;
    }

    fclose(file);
    
}


// --- Memory Freeing Functions ---

// Helper function to free BST nodes recursively
void free_bst_nodes(TreeNode *root) {
    if (root != NULL) {
        free_bst_nodes(root->left);
        free_bst_nodes(root->right);
        free(root); // Free the TreeNode itself
    }
}

// Function to free all books from the hash table and BST
void free_all_books() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        Book *current = hash_table[i];
        while (current != NULL) {
            Book *temp = current;
            current = current->next;
            free(temp); // Free the Book structure
        }
        hash_table[i] = NULL; // Reset the hash table entry
    }
    free_bst_nodes(title_bst_root); // Free BST nodes
    title_bst_root = NULL; // Reset BST root
    printf("All book data freed from memory.\n");
}

// Function to free all users from the linked list
void free_all_users() {
    User *current = user_list;
    while (current != NULL) {
        User *temp = current;
        current = current->next;
        free(temp); // Free the User structure
    }
    user_list = NULL; // Reset the user list head
    printf("All user data freed from memory.\n");
}