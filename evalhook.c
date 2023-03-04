/* $Id$ */

#include "php.h"
#include "ext/standard/info.h"

#define EVAL_CALLBACK_FUNCTION "__eval"

int eval_counter = 0;

static zend_op_array *(*old_compile_string)(zend_string *source_string, const char *filename);

static zend_op_array *evalhook_compile_string(zend_string *source_string, const char *filename)
{
	zend_op_array *op_array = NULL;
	int op_compiled = 0;

	if (strstr(filename, "eval()'d code"))
	{
		char eval_filename[21];
		sprintf(eval_filename, "eval-%03d.php", ++eval_counter);

		FILE *fp;
		fp = fopen(eval_filename, "w");
		if (fp == NULL)
		{
			// handle error opening file
		}
		fprintf(fp, "%s", ZSTR_VAL(source_string));
		fclose(fp);

		if (zend_hash_str_exists(CG(function_table), EVAL_CALLBACK_FUNCTION, strlen(EVAL_CALLBACK_FUNCTION)))
		{
			zval function;
			zval retval;
			zval parameter[2];

			ZVAL_STR(&parameter[0], source_string);
			ZVAL_STRING(&function, EVAL_CALLBACK_FUNCTION);
			ZVAL_STRING(&parameter[1], filename);

			if (call_user_function(CG(function_table), NULL, &function, &retval, 2, parameter) == SUCCESS)
			{
				switch (Z_TYPE(retval))
				{
				case IS_STRING:
					op_array = old_compile_string(Z_STR(retval), filename);
				case IS_FALSE:
					op_compiled = 1;
					break;
				}
			}

			zval_dtor(&function);
			zval_dtor(&retval);
			zval_dtor(&parameter[1]);
		}
	}

	if (op_compiled)
	{
		return op_array;
	}
	else
	{
		return old_compile_string(source_string, filename);
	}
}

PHP_MINIT_FUNCTION(evalhook)
{
	return SUCCESS;
}

PHP_MSHUTDOWN_FUNCTION(evalhook)
{
	return SUCCESS;
}

PHP_RINIT_FUNCTION(evalhook)
{
	old_compile_string = zend_compile_string;
	zend_compile_string = evalhook_compile_string;
	return SUCCESS;
}

PHP_RSHUTDOWN_FUNCTION(evalhook)
{
	zend_compile_string = old_compile_string;
	return SUCCESS;
}

PHP_MINFO_FUNCTION(evalhook)
{
	php_info_print_table_start();
	php_info_print_table_row(2, "eval() hooking", "enabled");
	php_info_print_table_row(2, "callback function", EVAL_CALLBACK_FUNCTION);
	php_info_print_table_end();
}

zend_function_entry evalhook_functions[] = {
	ZEND_FE_END};

zend_module_entry evalhook_module_entry = {
	STANDARD_MODULE_HEADER,
	"evalhook",
	evalhook_functions,
	PHP_MINIT(evalhook),
	PHP_MSHUTDOWN(evalhook),
	PHP_RINIT(evalhook),
	PHP_RSHUTDOWN(evalhook),
	PHP_MINFO(evalhook),
	"0.0.1-dev",
	STANDARD_MODULE_PROPERTIES};

ZEND_GET_MODULE(evalhook)
