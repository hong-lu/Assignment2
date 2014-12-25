<?php
/*
  Copyright (c) 2011-2012 Computing for Volunteer Welfare Organisations (CVWO)
  National University of Singapore
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
  files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
  modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:
   
  1. The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
  Software.
   
  2. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
// $Id$

/**
 * @file
 * Script to send the queued emails, called directly from the command prompt.
 * Parameter passed in (from command prompt) should be DRUPAL_ROOT.
 * Should also be in the same directory as cvwobase_d7_constants.php and cvwobase_d7_mail_functions.php
 */

require_once 'cvwobase_d7_constants.php';
require_once 'cvwobase_d7_mail_functions.php';
 
// check for command line arguments
if ($argc != 2 || defined(DRUPAL_ROOT)) {
	die('Invalid arguments');
}

// Load Drupal up to database and system variables
define('DRUPAL_ROOT', $argv[1]);
require_once DRUPAL_ROOT . '/includes/bootstrap.inc';
$_SERVER['REMOTE_ADDR'] = '127.0.0.1';
drupal_bootstrap(DRUPAL_BOOTSTRAP_DATABASE);

$mail_queue = DrupalQueue::get(CVWOBASE_MAIL_QUEUE);
$error_queue = DrupalQueue::get(CVWOBASE_MAIL_ERROR_QUEUE);

// iterate between email batches
while ($item = $mail_queue->claimItem()) {
	$message = $item->data;
	$mail_queue->deleteItem($item);
	_cvwobase_d7_send_mail($message, $error_queue);
}