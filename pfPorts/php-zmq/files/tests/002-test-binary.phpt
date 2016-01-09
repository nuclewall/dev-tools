--TEST--
Test send / recv binary
--SKIPIF--
<?php require_once(dirname(__FILE__) . '/skipif.inc'); ?>
--FILE--
<?php

include dirname(__FILE__) . '/zeromq_test_helper.inc';

$rose = file_get_contents(dirname(__FILE__) . '/rose.jpg');

$server = create_server();
$client = create_client();

$client->send($rose);

$message = $server->recv();
var_dump(strlen($message));

$server->send($message);

$message = $client->recv();
var_dump(strlen($message));

var_dump($message === $rose);

--EXPECT--
int(2051)
int(2051)
bool(true)