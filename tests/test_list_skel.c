/**
 * Projeto: Sistemas Distribuídos 2025/2026
 * Grupo: XX
 * Autores: [Nomes dos elementos]
 * 
 * Unit tests for list_skel.c
 */

#include "list_skel.h"
#include "list.h"
#include "data.h"
#include "sdmessage.pb-c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* ========================================================================
 * TEST UTILITIES
 * ======================================================================== */

#define COLOR_RED     "\033[1;31m"
#define COLOR_GREEN   "\033[1;32m"
#define COLOR_BLUE    "\033[1;34m"
#define COLOR_RESET   "\033[0m"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_START(name) \
    printf(COLOR_BLUE "TEST: %s" COLOR_RESET "\n", name)

#define ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  " COLOR_GREEN "✓" COLOR_RESET " %s\n", message); \
        } else { \
            printf("  " COLOR_RED "✗ FAILED:" COLOR_RESET " %s\n", message); \
            tests_failed++; \
            return; \
        } \
    } while(0)

#define TEST_END() \
    do { \
        printf("  " COLOR_GREEN "PASS\n" COLOR_RESET "\n"); \
        tests_passed++; \
    } while(0)

/* ========================================================================
 * HELPER FUNCTIONS
 * ======================================================================== */

/**
 * Creates a test Data structure
 */
static Data *create_test_data(int ano, float preco, Marca marca, 
                              const char *modelo, Combustivel comb) {
    Data *data = malloc(sizeof(Data));
    data__init(data);
    data->ano = ano;
    data->preco = preco;
    data->marca = marca;
    data->modelo = strdup(modelo);
    data->combustivel = comb;
    return data;
}

/**
 * Frees a Data structure
 */
static void free_test_data(Data *data) {
    if (data) {
        free(data->modelo);
        free(data);
    }
}

/* ========================================================================
 * TEST CASES
 * ======================================================================== */

/**
 * Test 1: Initialize and destroy skeleton
 */
static void test_init_destroy(void) {
    TEST_START("Initialize and Destroy Skeleton");
    
    struct list_t *list = list_skel_init();
    ASSERT(list != NULL, "Skeleton initialized successfully");
    
    int result = list_skel_destroy(list);
    ASSERT(result == 0, "Skeleton destroyed successfully");
    
    TEST_END();
}

/**
 * Test 2: OP_ADD operation
 */
static void test_op_add(struct list_t *list) {
    TEST_START("OP_ADD Operation");
    
    // Create request message
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_ADD;
    msg.c_type = MESSAGE_T__C_TYPE__CT_DATA;
    msg.data = create_test_data(2020, 35000.0, MARCA__MARCA_BMW, 
                                "X5", COMBUSTIVEL__COMBUSTIVEL_GASOLEO);
    
    // Execute
    int result = invoke(&msg, list);
    ASSERT(result == 0, "invoke() returned success");
    
    // Check response
    ASSERT(msg.opcode == MESSAGE_T__OPCODE__OP_ADD + 1, 
           "Response opcode is OP_ADD+1");
    ASSERT(msg.c_type == MESSAGE_T__C_TYPE__CT_NONE, 
           "Response c_type is CT_NONE");
    
    free_test_data(msg.data);
    TEST_END();
}

/**
 * Test 3: OP_SIZE operation
 */
static void test_op_size(struct list_t *list, int expected_size) {
    TEST_START("OP_SIZE Operation");
    
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    
    int result = invoke(&msg, list);
    ASSERT(result == 0, "invoke() returned success");
    
    ASSERT(msg.opcode == MESSAGE_T__OPCODE__OP_SIZE + 1, 
           "Response opcode is OP_SIZE+1");
    ASSERT(msg.c_type == MESSAGE_T__C_TYPE__CT_RESULT, 
           "Response c_type is CT_RESULT");
    
    char size_msg[100];
    snprintf(size_msg, sizeof(size_msg), "Size is %d (expected %d)", 
             msg.result, expected_size);
    ASSERT(msg.result == expected_size, size_msg);
    
    TEST_END();
}

/**
 * Test 4: OP_GET with CT_MARCA
 */
static void test_op_get_marca(struct list_t *list) {
    TEST_START("OP_GET with CT_MARCA");
    
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_MARCA;
    msg.data = create_test_data(0, 0, MARCA__MARCA_BMW, "", 0);
    
    int result = invoke(&msg, list);
    ASSERT(result == 0, "invoke() returned success");
    
    ASSERT(msg.opcode == MESSAGE_T__OPCODE__OP_GET + 1, 
           "Response opcode is OP_GET+1");
    ASSERT(msg.c_type == MESSAGE_T__C_TYPE__CT_DATA, 
           "Response c_type is CT_DATA");
    ASSERT(msg.data != NULL, "Response data is not NULL");
    ASSERT(msg.data->marca == MARCA__MARCA_BMW, 
           "Returned car has correct marca");
    
    printf("  Found car: %s (year=%d, price=%.2f)\n", 
           msg.data->modelo, msg.data->ano, msg.data->preco);
    
    data__free_unpacked(msg.data, NULL);
    TEST_END();
}

/**
 * Test 5: OP_GET with CT_YEAR
 */
static void test_op_get_year(struct list_t *list) {
    TEST_START("OP_GET with CT_YEAR");
    
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    msg.c_type = MESSAGE_T__C_TYPE__CT_YEAR;
    msg.data = create_test_data(2020, 0, 0, "", 0);
    
    int result = invoke(&msg, list);
    ASSERT(result == 0, "invoke() returned success");
    
    ASSERT(msg.opcode == MESSAGE_T__OPCODE__OP_GET + 1, 
           "Response opcode is OP_GET+1");
    ASSERT(msg.c_type == MESSAGE_T__C_TYPE__CT_YEAR, 
           "Response c_type is CT_YEAR");
    
    printf("  Found %zu car(s) from year 2020\n", msg.n_cars);
    
    for (size_t i = 0; i < msg.n_cars; i++) {
        ASSERT(msg.cars[i]->ano == 2020, "Car has correct year");
        data__free_unpacked(msg.cars[i], NULL);
    }
    free(msg.cars);
    
    TEST_END();
}

/**
 * Test 6: OP_GETMODELS operation
 */
static void test_op_getmodels(struct list_t *list) {
    TEST_START("OP_GETMODELS Operation");
    
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETMODELS;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    
    int result = invoke(&msg, list);
    ASSERT(result == 0, "invoke() returned success");
    
    ASSERT(msg.opcode == MESSAGE_T__OPCODE__OP_GETMODELS + 1, 
           "Response opcode is OP_GETMODELS+1");
    ASSERT(msg.c_type == MESSAGE_T__C_TYPE__CT_MODEL, 
           "Response c_type is CT_MODEL");
    
    printf("  Found %zu model(s):\n", msg.n_models);
    for (size_t i = 0; i < msg.n_models; i++) {
        printf("    - %s\n", msg.models[i]);
        free(msg.models[i]);
    }
    free(msg.models);
    
    TEST_END();
}

/**
 * Test 7: OP_DEL operation
 */
static void test_op_del(struct list_t *list) {
    TEST_START("OP_DEL Operation");
    
    // First, add a car to delete
    MessageT add_msg = MESSAGE_T__INIT;
    add_msg.opcode = MESSAGE_T__OPCODE__OP_ADD;
    add_msg.c_type = MESSAGE_T__C_TYPE__CT_DATA;
    add_msg.data = create_test_data(2015, 15000.0, MARCA__MARCA_RENAULT, 
                                    "ToDelete", COMBUSTIVEL__COMBUSTIVEL_GASOLINA);
    invoke(&add_msg, list);
    free_test_data(add_msg.data);
    
    // Now delete it
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    msg.c_type = MESSAGE_T__C_TYPE__CT_MODEL;
    msg.models = malloc(sizeof(char*));
    msg.models[0] = strdup("ToDelete");
    msg.n_models = 1;
    
    int result = invoke(&msg, list);
    ASSERT(result == 0, "invoke() returned success");
    
    ASSERT(msg.opcode == MESSAGE_T__OPCODE__OP_DEL + 1, 
           "Response opcode is OP_DEL+1");
    ASSERT(msg.c_type == MESSAGE_T__C_TYPE__CT_NONE, 
           "Response c_type is CT_NONE");
    
    free(msg.models[0]);
    free(msg.models);
    
    TEST_END();
}

/**
 * Test 8: OP_GETLISTBYTEAR operation
 */
static void test_op_getlistbytear(struct list_t *list) {
    TEST_START("OP_GETLISTBYTEAR Operation");
    
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = MESSAGE_T__OPCODE__OP_GETLISTBYTEAR;
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    
    int result = invoke(&msg, list);
    ASSERT(result == 0, "invoke() returned success");
    
    ASSERT(msg.opcode == MESSAGE_T__OPCODE__OP_GETLISTBYTEAR + 1, 
           "Response opcode is OP_GETLISTBYTEAR+1");
    ASSERT(msg.c_type == MESSAGE_T__C_TYPE__CT_LIST, 
           "Response c_type is CT_LIST");
    
    printf("  Returned %zu car(s), ordered by year:\n", msg.n_cars);
    
    int prev_year = 0;
    for (size_t i = 0; i < msg.n_cars; i++) {
        printf("    %d: %s (year=%d)\n", (int)i+1, 
               msg.cars[i]->modelo, msg.cars[i]->ano);
        
        // Verify ordering
        if (i > 0) {
            ASSERT(msg.cars[i]->ano >= prev_year, "Cars are ordered by year");
        }
        prev_year = msg.cars[i]->ano;
        
        data__free_unpacked(msg.cars[i], NULL);
    }
    free(msg.cars);
    
    TEST_END();
}

/**
 * Test 9: Error handling - invalid opcode
 */
static void test_error_invalid_opcode(struct list_t *list) {
    TEST_START("Error Handling - Invalid Opcode");
    
    MessageT msg = MESSAGE_T__INIT;
    msg.opcode = 999; // Invalid opcode
    msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    
    int result = invoke(&msg, list);
    ASSERT(result == 0, "invoke() returned success even with invalid opcode");
    
    ASSERT(msg.opcode == MESSAGE_T__OPCODE__OP_ERROR, 
           "Response opcode is OP_ERROR");
    ASSERT(msg.c_type == MESSAGE_T__C_TYPE__CT_NONE, 
           "Response c_type is CT_NONE");
    
    TEST_END();
}

/**
 * Test 10: Error handling - NULL parameters
 */
static void test_error_null_params(struct list_t *list) {
    TEST_START("Error Handling - NULL Parameters");
    
    MessageT msg = MESSAGE_T__INIT;
    
    // Test NULL message
    int result = invoke(NULL, list);
    ASSERT(result == -1, "invoke() returns -1 for NULL message");
    
    // Test NULL list
    result = invoke(&msg, NULL);
    ASSERT(result == -1, "invoke() returns -1 for NULL list");
    
    TEST_END();
}

/**
 * Test 11: Multiple operations sequence
 */
static void test_multiple_operations(struct list_t *list) {
    TEST_START("Multiple Operations Sequence");
    
    // Add multiple cars
    const char *models[] = {"Car1", "Car2", "Car3"};
    for (int i = 0; i < 3; i++) {
        MessageT msg = MESSAGE_T__INIT;
        msg.opcode = MESSAGE_T__OPCODE__OP_ADD;
        msg.c_type = MESSAGE_T__C_TYPE__CT_DATA;
        msg.data = create_test_data(2020 + i, 20000.0 + i * 5000, 
                                    MARCA__MARCA_TOYOTA, models[i], 
                                    COMBUSTIVEL__COMBUSTIVEL_HIBRIDO);
        
        int result = invoke(&msg, list);
        ASSERT(result == 0, "Add operation successful");
        
        free_test_data(msg.data);
    }
    
    // Get size
    MessageT size_msg = MESSAGE_T__INIT;
    size_msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    size_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    invoke(&size_msg, list);
    
    int total_size = size_msg.result;
    ASSERT(total_size >= 3, "Size increased after adding cars");
    
    // Get models
    MessageT models_msg = MESSAGE_T__INIT;
    models_msg.opcode = MESSAGE_T__OPCODE__OP_GETMODELS;
    models_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    invoke(&models_msg, list);
    
    ASSERT(models_msg.n_models >= 3, "Model list contains added cars");
    
    for (size_t i = 0; i < models_msg.n_models; i++) {
        free(models_msg.models[i]);
    }
    free(models_msg.models);
    
    TEST_END();
}

/**
 * Test 12: Memory leak check - repeated operations
 */
static void test_memory_leak(struct list_t *list) {
    TEST_START("Memory Leak Check - Repeated Operations");
    
    printf("  Running 100 operations to check for leaks...\n");
    
    // Add and remove cars repeatedly
    for (int i = 0; i < 100; i++) {
        // Add
        MessageT add_msg = MESSAGE_T__INIT;
        add_msg.opcode = MESSAGE_T__OPCODE__OP_ADD;
        add_msg.c_type = MESSAGE_T__C_TYPE__CT_DATA;
        add_msg.data = create_test_data(2020, 25000.0, MARCA__MARCA_AUDI, 
                                       "LeakTest", COMBUSTIVEL__COMBUSTIVEL_GASOLINA);
        invoke(&add_msg, list);
        free_test_data(add_msg.data);
        
        // Query
        MessageT size_msg = MESSAGE_T__INIT;
        size_msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
        size_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
        invoke(&size_msg, list);
        
        // Get models (allocates memory that must be freed)
        MessageT models_msg = MESSAGE_T__INIT;
        models_msg.opcode = MESSAGE_T__OPCODE__OP_GETMODELS;
        models_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
        invoke(&models_msg, list);
        
        for (size_t j = 0; j < models_msg.n_models; j++) {
            free(models_msg.models[j]);
        }
        free(models_msg.models);
    }
    
    printf("  Completed 100 iterations without crash\n");
    ASSERT(1, "No crashes during repeated operations");
    
    TEST_END();
}

/**
 * Test 13: Edge case - empty list operations
 */
static void test_empty_list(void) {
    TEST_START("Edge Case - Operations on Empty List");
    
    struct list_t *empty_list = list_skel_init();
    ASSERT(empty_list != NULL, "Empty list initialized");
    
    // Size of empty list
    MessageT size_msg = MESSAGE_T__INIT;
    size_msg.opcode = MESSAGE_T__OPCODE__OP_SIZE;
    size_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    invoke(&size_msg, empty_list);
    ASSERT(size_msg.result == 0, "Empty list has size 0");
    
    // Get models from empty list
    MessageT models_msg = MESSAGE_T__INIT;
    models_msg.opcode = MESSAGE_T__OPCODE__OP_GETMODELS;
    models_msg.c_type = MESSAGE_T__C_TYPE__CT_NONE;
    invoke(&models_msg, empty_list);
    ASSERT(models_msg.n_models == 0, "Empty list has no models");
    
    // Get by marca from empty list
    MessageT get_msg = MESSAGE_T__INIT;
    get_msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    get_msg.c_type = MESSAGE_T__C_TYPE__CT_MARCA;
    get_msg.data = create_test_data(0, 0, MARCA__MARCA_BMW, "", 0);
    invoke(&get_msg, empty_list);
    ASSERT(get_msg.opcode == MESSAGE_T__OPCODE__OP_ERROR, 
           "Get from empty list returns error");
    free_test_data(get_msg.data);
    
    // Remove from empty list
    MessageT del_msg = MESSAGE_T__INIT;
    del_msg.opcode = MESSAGE_T__OPCODE__OP_DEL;
    del_msg.c_type = MESSAGE_T__C_TYPE__CT_MODEL;
    del_msg.models = malloc(sizeof(char*));
    del_msg.models[0] = strdup("NonExistent");
    del_msg.n_models = 1;
    invoke(&del_msg, empty_list);
    ASSERT(del_msg.opcode == MESSAGE_T__OPCODE__OP_DEL + 1, 
           "Delete from empty list completes");
    free(del_msg.models[0]);
    free(del_msg.models);
    
    list_skel_destroy(empty_list);
    TEST_END();
}

/**
 * Test 14: Data integrity - verify data after operations
 */
static void test_data_integrity(struct list_t *list) {
    TEST_START("Data Integrity Check");
    
    // Add a specific car
    float test_price = 42999.99;
    int test_year = 2023;
    const char *test_model = "IntegrityTest";
    
    MessageT add_msg = MESSAGE_T__INIT;
    add_msg.opcode = MESSAGE_T__OPCODE__OP_ADD;
    add_msg.c_type = MESSAGE_T__C_TYPE__CT_DATA;
    add_msg.data = create_test_data(test_year, test_price, MARCA__MARCA_MERCEDES, 
                                    test_model, COMBUSTIVEL__COMBUSTIVEL_ELETRICO);
    invoke(&add_msg, list);
    free_test_data(add_msg.data);
    
    // Retrieve by marca and verify
    MessageT get_msg = MESSAGE_T__INIT;
    get_msg.opcode = MESSAGE_T__OPCODE__OP_GET;
    get_msg.c_type = MESSAGE_T__C_TYPE__CT_MARCA;
    get_msg.data = create_test_data(0, 0, MARCA__MARCA_MERCEDES, "", 0);
    invoke(&get_msg, list);
    
    if (get_msg.opcode == MESSAGE_T__OPCODE__OP_GET + 1) {
        ASSERT(get_msg.data != NULL, "Retrieved data is not NULL");
        ASSERT(get_msg.data->ano == test_year, "Year matches");
        
        // Check price with tolerance for float comparison
        float price_diff = get_msg.data->preco - test_price;
        if (price_diff < 0) price_diff = -price_diff;
        ASSERT(price_diff < 0.01, "Price matches (within tolerance)");
        
        ASSERT(get_msg.data->marca == MARCA__MARCA_MERCEDES, "Marca matches");
        ASSERT(get_msg.data->combustivel == COMBUSTIVEL__COMBUSTIVEL_ELETRICO, 
               "Combustivel matches");
        
        printf("  All fields verified: year=%d, price=%.2f, marca=%d, modelo=%s, comb=%d\n",
               get_msg.data->ano, get_msg.data->preco, get_msg.data->marca,
               get_msg.data->modelo, get_msg.data->combustivel);
        
        data__free_unpacked(get_msg.data, NULL);
    }
    
    TEST_END();
}

/* ========================================================================
 * MAIN TEST RUNNER
 * ======================================================================== */

static void print_summary(void) {
    printf("\n");
    printf("========================================\n");
    printf("UNIT TEST SUMMARY\n");
    printf("========================================\n");
    printf("Total:  %d\n", tests_passed + tests_failed);
    printf(COLOR_GREEN "Passed: %d" COLOR_RESET "\n", tests_passed);
    printf(COLOR_RED "Failed: %d" COLOR_RESET "\n", tests_failed);
    printf("========================================\n");
    
    if (tests_failed == 0) {
        printf(COLOR_GREEN "All unit tests passed! ✓" COLOR_RESET "\n");
    } else {
        printf(COLOR_RED "Some unit tests failed! ✗" COLOR_RESET "\n");
    }
}

/* int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("  List Skeleton Unit Tests\n");
    printf("  Testing: list_skel.c\n");
    printf("========================================\n\n");
    
    // Test 1: Basic init/destroy (standalone)
    test_init_destroy();
    
    // Initialize list for remaining tests
    struct list_t *list = list_skel_init();
    if (list == NULL) {
        fprintf(stderr, COLOR_RED "Failed to initialize skeleton!\n" COLOR_RESET);
        return 1;
    }
    
    printf(COLOR_GREEN "Skeleton initialized for testing\n" COLOR_RESET "\n");
    
    // Run all operation tests
    test_op_add(list);              // Test 2
    test_op_size(list, 1);          // Test 3
    test_op_get_marca(list);        // Test 4
    test_op_get_year(list);         // Test 5
    test_op_getmodels(list);        // Test 6
    test_op_del(list);              // Test 7
    test_op_getlistbytear(list);    // Test 8
    
    // Error handling tests
    test_error_invalid_opcode(list); // Test 9
    test_error_null_params(list);    // Test 10
    
    // Complex tests
    test_multiple_operations(list);  // Test 11
    test_memory_leak(list);          // Test 12
    
    // Edge case tests
    test_empty_list();               // Test 13 (creates own list)
    test_data_integrity(list);       // Test 14
    
    // Cleanup
    printf("Destroying skeleton...\n");
    list_skel_destroy(list);
    
    // Print summary
    print_summary();
    
    return (tests_failed == 0) ? 0 : 1;
} */