#!/usr/local/bin/php -f
<?php

/*
     This script will chroot via the builder system to test
     the local php setup.  If we can perform a series of
     small tests to ensure the php environment is sane.
*/

require_once("/etc/inc/globals.inc");
require_once("/etc/inc/util.inc");
require_once("/etc/inc/xmlparse.inc");
/* NOTE: Helpful for 1.2.3 builds which do not have this file. */
if (file_exists("/etc/inc/config.lib.inc"))
	require_once("/etc/inc/config.lib.inc");
else
	require_once("/etc/inc/config.inc");
require_once("/etc/inc/functions.inc");

$config = parse_config(true);

$passed_tests = true;

// Test config.inc
if($config['system']['hostname'] == "") {
	$passed_tests = false;
}

// Test for php-fcgi
$php_cgi = trim(`php -v | grep cgi-fcgi`);
if(stristr($php_cgi, "cgi-fcgi")) {
	echo "FCGI-PASSED ";
} else {
	echo "FCGI-FAILED ";
	exit(1);
}

if($passed_tests) {
	echo "PASSED";
	exit(0);
}

// Tests failed.
exit(1);

?>
