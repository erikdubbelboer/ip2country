
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "php.h"
#include "php_ini.h"
#include "ext/standard/info.h"
#include "php_ip2country.h"

#include <sys/shm.h>
#include <arpa/inet.h>

#include "db.h"


typedef struct ip2country_stat_s {
        unsigned long hits;
        unsigned long misses;
        unsigned long lastmisses[10];
} ip2country_stat_t;


/* exported functions */
zend_function_entry ip2country_functions[] = {
	PHP_FE(ip2country,	NULL)
	PHP_FE(code2country,	NULL)
	PHP_FE(ip2country_stat, NULL)
	{NULL, NULL, NULL}
};

zend_module_entry ip2country_module_entry = {
#if ZEND_MODULE_API_NO >= 20010901
	STANDARD_MODULE_HEADER,
#endif
	"ip2country",
	ip2country_functions,
	PHP_MINIT(ip2country),
	PHP_MSHUTDOWN(ip2country),
	NULL,
	NULL,
	NULL,
#if ZEND_MODULE_API_NO >= 20010901
	"1.0",
#endif
	STANDARD_MODULE_PROPERTIES
};

#ifdef COMPILE_DL_IP2COUNTRY
ZEND_GET_MODULE(ip2country)
#endif



/* array of all ranges */
static range_t* ranges = NULL;
static range_t* root   = NULL; /* the root of the tree (last node of the ranges array) */

/* array of all countries */
static country_t*   countries     = NULL;
static unsigned int num_countries = 0;



/* shared memory id, and pointer to the stat structure that's placed in it */
static int shmid;
static ip2country_stat_t* stats = NULL;


/* php.ini entries
 * PHP_INI_SYSTEM means they can only be changed in the .ini file
 */
PHP_INI_BEGIN()
PHP_INI_ENTRY("ip2country.db"  , "/var/www/geoip.db", PHP_INI_SYSTEM, NULL)
PHP_INI_ENTRY("ip2country.stat", "1"                , PHP_INI_SYSTEM, NULL)
PHP_INI_END()


/* simple O(n) lookup base on the country code. */
static country_t* country_get(char* code) {
        unsigned int i;

        for (i = 0; i < num_countries; ++i) {
                if (strcmp(countries[i].code, code) == 0) {
                        return &countries[i];
                }
        }

        return NULL;
}



/* return the left node if we didn't visit it already */
static range_t* tree_left(range_t* node) {
        if (node->left == UINT_MAX ) {
                return NULL;
        } else {
                return &ranges[node->left];
        }
}


/* return the right node if we didn't visit it already */
static range_t* tree_right(range_t* node) {
        if (node->right == UINT_MAX) {
                return NULL;
        } else {
                return &ranges[node->right];
        }
}


/* find the range in which val belongs */
static country_t* tree_find(unsigned long val) {
        range_t* node = root;
        range_t* newn;

        for (;;) {
                // most common case first
                if (node->start > val) {
                        // val = 2
                        //         3 <-- we are here
                        //        / \
                        //       2   4

                        newn = tree_left(node);
                } else {
                        // val = 2
                        //         1 <-- we are here
                        //        / \
                        //       0   3

                        if ((node->start <= val) &&
                            (node->end   >= val)) {

                                return &countries[node->country];
                        }

                        newn = tree_right(node);
                }

                if (newn == NULL) {
                        return NULL;
                }

                node = newn;
        }
}


/* this function will be called once when php is started */
PHP_MINIT_FUNCTION(ip2country) {
	REGISTER_INI_ENTRIES();

        FILE* fp = fopen(INI_STR("ip2country.db"), "r");

        if (fp != NULL) {
                // first 4 bytes is the number of countries
                fread(&num_countries, sizeof(unsigned int), 1, fp);

                countries = (country_t*)pemalloc(sizeof(country_t) * num_countries, 1);

                fread(countries, sizeof(country_t) * num_countries, 1, fp);


                // next 4 bytes is the number of ranges
                unsigned int num_ranges = 0;
                fread(&num_ranges, sizeof(unsigned int), 1, fp);

                ranges = (range_t*)pemalloc(sizeof(range_t) * num_ranges, 1);

                fread(ranges, sizeof(range_t) * num_ranges, 1, fp);

                // the root is the last range in the array cause of the reverse order they are written in
                root = &ranges[num_ranges - 1];

                fclose(fp);
        }


        if (INI_BOOL("ip2country.stat")) {
                shmid = shmget(IPC_PRIVATE, sizeof(ip2country_stat_t), 0777|IPC_CREAT);

                if (shmid != -1) {
                        stats = (ip2country_stat_t*)shmat(shmid, 0, 0);
                        memset(stats, 0, sizeof(ip2country_stat_t));
                }
        }

	return SUCCESS;
}


/* this function will get called when php is shutdown */
PHP_MSHUTDOWN_FUNCTION(ip2country) {
	pefree(countries, 1);
        pefree(ranges, 1);

        if (stats != NULL) {
                shmdt(stats);
                shmctl(shmid, IPC_RMID, 0);
        }

        UNREGISTER_INI_ENTRIES();

	return SUCCESS;
}


PHP_RINIT_FUNCTION(ip2country) {
	return SUCCESS;
}


PHP_RSHUTDOWN_FUNCTION(ip2country) {
	return SUCCESS;
}


PHP_MINFO_FUNCTION(ip2country) {
	php_info_print_table_start();
	php_info_print_table_header(2, "ip2country support", "enabled");
	php_info_print_table_end();

	DISPLAY_INI_ENTRIES();
}


/* this function can be called from php code.
 * return a country code and optionally name from an ip.
 * the ip can either be a string or a number.
 */
PHP_FUNCTION(ip2country) {
	zval*         zip;
        zend_bool     full = 0;
        country_t*    country;
        unsigned long ip;

        // this check will also print if arguments are missing or wrong
        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "z|b", &zip, &full) == FAILURE) {
                RETURN_NULL();
        }

        if (Z_TYPE_P(zip) == IS_LONG) {
                ip = Z_LVAL_P(zip);
        } else if (Z_TYPE_P(zip) == IS_STRING) {
                ip = ntohl(inet_addr(Z_STRVAL_P(zip))); // convert the string to a number
        } else {
                convert_to_long(zip);
                ip = Z_LVAL_P(zip);
        }

        if (ranges == NULL) {
                php_error(E_WARNING, "%s: No db loaded (could not read %s)", get_active_function_name(TSRMLS_C), INI_STR("ip2country.db"));
                RETURN_NULL();
        }

        country = tree_find(ip);

        if (country == NULL) {
                if (stats != NULL) {
                        stats->misses++;
                        stats->lastmisses[stats->misses % 10] = ip;
                }

                RETURN_NULL();
        }

        if (stats != NULL) {
                stats->hits++;
        }

        if (full) {
                array_init(return_value);

                add_assoc_string(return_value, "code", country->code, 1);
                add_assoc_string(return_value, "name", country->name, 1);
        } else {
                RETURN_STRING(country->code, 1);
        }
}


/* this function can be called from php code.
 * return a country name from a code.
 */
PHP_FUNCTION(code2country) {
	char*      code;
        int        code_len;
        country_t* country;

        if (zend_parse_parameters(ZEND_NUM_ARGS() TSRMLS_CC, "s", &code, &code_len) == FAILURE) {
                RETURN_NULL();
        }

        country = country_get(code);

        if (country == NULL) {
                RETURN_NULL();
        } else {
                RETURN_STRING(country->name, 1);
        }
}


/* this function can be called from php code.
 * return some statistics if enabled.
 */
PHP_FUNCTION(ip2country_stat) {
        if (stats == NULL) {
                RETURN_NULL();
        }

        array_init(return_value);

        add_assoc_long(return_value, "hits", stats->hits);
        add_assoc_long(return_value, "misses", stats->misses);

        zval *lastmisses;
        ALLOC_INIT_ZVAL(lastmisses);
        array_init(lastmisses);

        int i;
        for (i = 0; i < 10; i++) {
                add_index_long(lastmisses, i, stats->lastmisses[i]);
        }

        add_assoc_zval(return_value, "lastmisses", lastmisses);
}

