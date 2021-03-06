#define LIGHT_ARENA_IMPLEMENT
#include <stdio.h>
#include "lexer.h"
#include "parser.h"
#include "utils/os.h"
#include "utils/utils.h"
#include "global_tables.h"
#include "top_typecheck.h"
#include "bytecode.h"
#include "backend/c/toplevel.h"
#include <light_array.h>

int main(int argc, char** argv) {
    double start = os_time_us();

    light_set_global_tables(argv[0]);

    if(argc < 2) {
        fprintf(stderr, "usage: %s filename\n", argv[0]);
        return 1;
    }

    size_t compiler_path_size = 0;
    const char* compiler_path = light_path_from_filename(argv[0], &compiler_path_size);

    size_t real_path_size = 0;
    const char* main_file_directory = light_path_from_filename(argv[1], &real_path_size);

    Light_Lexer  lexer = {0};
    Light_Parser parser = {0};
    Light_Scope  global_scope = {0};

    initialize_global_identifiers_table();

    u32 parser_error = 0;
    parse_init(&parser, &lexer, &global_scope, compiler_path, compiler_path_size, argv[1]);

    Light_Ast** ast = 0;

    double parse_start = os_time_us();
    double lexing_elapsed = 0.0;
    // Parse all other files included in the main file
    while(array_length(parser.parse_queue_array) > 0) {
        Light_Lexer lexer = {0};
        double lexer_start = os_time_us();
        Light_Token* tokens = lexer_file(&lexer, parser.parse_queue_array[0].data, 0);
        lexing_elapsed += (os_time_us() - lexer_start) / 1000.0;

        if(tokens == 0) {
            // File does not exist
            return 1;
        }

        ast = parse_top_level(&parser, &lexer, &global_scope, &parser_error);

        if(parser_error & PARSER_ERROR_FATAL)
            return 1;

        array_remove(parser.parse_queue_array, 0);
    }
    double parse_elapsed = (os_time_us() - parse_start) / 1000.0;
    
    // Type checking
    double tcheck_start = os_time_us();
    Light_Type_Error type_error = top_typecheck(ast, &global_scope);
    if(type_error & TYPE_ERROR) {
        return 1;
    }
    double tcheck_elapsed = (os_time_us() - tcheck_start) / 1000.0;
    
#if 0
    ast_print(ast, LIGHT_AST_PRINT_STDOUT|LIGHT_AST_PRINT_EXPR_TYPES, 0);
    //ast_print(ast, LIGHT_AST_PRINT_STDOUT, 0);
    type_table_print();
#endif

#if 1
    const char* outfile = light_extensionless_filename(light_filename_from_path(argv[1]));

    double generate_start = os_time_us();
    backend_c_generate_top_level(ast, global_type_table, &global_scope, main_file_directory, outfile, compiler_path);
    double generate_elapsed = (os_time_us() - generate_start) / 1000.0;

    double total_elapsed = (os_time_us() - start) / 1000.0;

    double gcc_start = os_time_us();
    backend_c_compile_with_gcc(ast, outfile, main_file_directory);
    double gcc_elapsed = (os_time_us() - gcc_start) / 1000.0;

    printf("- elapsed time:\n\n");
    printf("  lexing:          %.2f ms\n", lexing_elapsed);
    printf("  parse:           %.2f ms\n", parse_elapsed);
    printf("  type check:      %.2f ms\n", tcheck_elapsed);
    printf("  code generation: %.2f ms\n", generate_elapsed);
    printf("  total:           %.2f ms\n", total_elapsed);
    printf("\n");
    printf("  gcc backend:     %.2f ms\n", gcc_elapsed);
#endif

#if 0
    Bytecode_State state = bytecode_gen_ast(ast);

    light_vm_debug_dump_code(stdout, state.vmstate);

    light_vm_execute(state.vmstate, 0, 1);
    light_vm_debug_dump_registers(stdout, state.vmstate, LVM_PRINT_FLOATING_POINT_REGISTERS|LVM_PRINT_DECIMAL);
#endif

    return 0;
}