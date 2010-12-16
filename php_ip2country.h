
#ifndef PHP_IP2COUNTRY_H
#define PHP_IP2COUNTRY_H

extern zend_module_entry ip2country_module_entry;
#define phpext_ip2country_ptr &ip2country_module_entry

#ifdef PHP_WIN32
#define PHP_IP2COUNTRY_API __declspec(dllexport)
#else
#define PHP_IP2COUNTRY_API
#endif

#ifdef ZTS
#include "TSRM.h"
#endif

/* init and shutdown functions */
PHP_MINIT_FUNCTION(ip2country);
PHP_MSHUTDOWN_FUNCTION(ip2country);
PHP_RINIT_FUNCTION(ip2country);
PHP_RSHUTDOWN_FUNCTION(ip2country);
PHP_MINFO_FUNCTION(ip2country);

/* exported functions that can be used inside php */
PHP_FUNCTION(ip2country);
PHP_FUNCTION(code2country);
PHP_FUNCTION(ip2country_stat);


#ifdef ZTS
#define IP2COUNTRY_G(v) TSRMG(ip2country_globals_id, zend_ip2country_globals *, v)
#else
#define IP2COUNTRY_G(v) (ip2country_globals.v)
#endif

#endif	/* PHP_IP2COUNTRY_H */

