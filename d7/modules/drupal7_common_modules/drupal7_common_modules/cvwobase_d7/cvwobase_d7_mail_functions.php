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
 * Utility functions to send mails using a PEAR mail object.
 */

@require_once 'Mail.php';

/**
 * Sends the mail defined in the $message array.
 * If not called from within Drupal, DRUPAL_ROOT should have been defined,
 * and bootstrap.inc should have been included prior to calling this.
 *
 * @param array $message
 * @param object
 *	Either NULL or an instance of DrupalQueueInterface
 *	Will send PEAR Mail object errors into this queue.
 * @return boolean
 *	Success of sending mail
 */
function _cvwobase_d7_send_mail($message, $error_queue = NULL) {
	// Not drupal_static(), because does not depend on any run-time information.
	static $mail_obj = NULL;
	drupal_bootstrap(DRUPAL_BOOTSTRAP_VARIABLES);
	
	if (is_null($mail_obj))
		$mail_obj = Mail::factory('smtp', array(
			'host' => variable_get(CVWOBASE_D7_MAIL_HOST, CVWOBASE_D7_MAIL_HOST_DEFAULT),
			'port' => variable_get(CVWOBASE_D7_MAIL_PORT, CVWOBASE_D7_MAIL_PORT_DEFAULT),
			'auth' => TRUE,
			'username' => variable_get(CVWOBASE_D7_MAIL_USER, CVWOBASE_D7_MAIL_USER_DEFAULT),
			'password' => variable_get(CVWOBASE_D7_MAIL_PASS, CVWOBASE_D7_MAIL_PASS_DEFAULT)
		));
	
	$headers = array();
	if (!isset($message['headers']))
		$message['headers'] = array();
	$headers['From'] = $message['from'];
	$headers['To'] = $message['to'];
	$headers['Subject'] = $message['subject'];
	$headers += $message['headers'];
	if (!is_null($error_queue) && PEAR::isError($result = $mail_obj->send($message['to'], $headers, $message['body']))) {
		$error_queue->createItem($result->getMessage());
		return FALSE;
	}
	
	return TRUE;
}