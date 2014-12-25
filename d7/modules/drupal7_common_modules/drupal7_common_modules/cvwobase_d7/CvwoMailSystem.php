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
 * CVWO extension of DefaultMailSystem.
 */

require_once drupal_get_path('module', CVWOBASE_MODULE).'/cvwobase_d7_mail_functions.php';

/**
 * The CVWO mail backend using PEAR's smtp mail function.
 */
class CvwoMailSystem extends DefaultMailSystem {
  /**
   * Send an e-mail message, using Drupal variables and default settings.
   *
   * @see http://pear.php.net/package/Mail/docs
   *
   * @param $message
   *   A message array, as described in hook_mail_alter().
   * @return
   *   TRUE if the mail was successfully accepted, otherwise FALSE.
   */
  public function mail(array $message) {
		$php_binary = variable_get(CVWOBASE_D7_PHP_BINARY, FALSE);
		// Send asynchronously, and PHP binary path known
		if ($php_binary && variable_get(CVWOBASE_D7_MAIL_SEND_ASYNC, CVWOBASE_D7_MAIL_SEND_ASYNC_DEFAULT)) {
			DrupalQueue::get(CVWOBASE_MAIL_QUEUE)->createItem($message);
			$execstring = $php_binary.' '.DRUPAL_ROOT.'/'.drupal_get_path('module', CVWOBASE_MODULE).'/cvwobase_d7_email_process.php '.DRUPAL_ROOT;
			cvwobase_exec($execstring);
			
			return TRUE;
		// Send inline
		} else {
			$error_queue = new MemoryQueue(CVWOBASE_MAIL_ERROR_QUEUE); // name irrelevant
			_cvwobase_d7_send_mail($message, $error_queue);
			if ($error_queue->numberOfItems() > 0) {
				while ($error = $error_queue->claimItem()) {
					drupal_set_message(t('Unable to send message: PEAR Mail reports error "%error"', array('%error' => $error->data)), 'error');
					$error_queue->deleteItem($item);
				}
				return FALSE;
			}
			return TRUE;
		}
  }
}