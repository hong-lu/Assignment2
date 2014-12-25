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
 * Audit API for the cvwobase_d7 module.
 */

 
/**
 * Retrieves module names that have entries in the {cvwoaudit} table
 * @return
 *  Array of module names
 */
function cvwobase_get_audit_modules() {
  return db_query('SELECT DISTINCT module_name FROM {'.CVWO_AUDIT_TABLE.'} ORDER BY module_name ASC')->fetchCol();
}

/**
 * Retrieves tags that exist in the {cvwoaudit} table
 * @return
 *  Array of tags
 */
function cvwobase_get_audit_tags() {
  return db_query('SELECT DISTINCT tag FROM {'.CVWO_AUDIT_TABLE.'} ORDER BY tag ASC')->fetchCol();
}

/**
 * Add a log message to the CVWO audit trail
 *
 * @param string $message
 *  A string containing your message. Variables in the
 *  message should be added by using placeholder strings alongside
 *  the variables argument to declare the value of the placeholders.
 *  See t() for documentation on how $message and $variables interact.
 *
 * @param string $tag
 *  A varchar containing a tag for the message. This tag can be arbitrarily
 *  defined, but modules are recommended to utilise a limited number of tags.
 *  Tags can be think of as the "type" of audit message.
 *  N.B. Tags should not contain the character '/'.
 *
 * @param string $module
 *  Your module name (the one which you use for all hooks, etc.) The function
 *  will throw an exception if you use an invalid module name not registered
 *  with drupal.
 *
 * @param $variables
 *  Array of variables to replace in the message on display or
 *  NULL if message is already translated or not possible to
 *  translate.
 *
 * @param int $uid (optional)
 *  User id for which the log message applies to (or who triggered the log
 *  message). Defaults to the current logged in user.
 *
 * @param string $referrer (optional)
 *  Referer URL string. Defaults to what the system detects but you can manually
 *  set it.
 *
 * @param string $host (optional)
 *  Hostname string. Defaults to what the system detects but you can manually
 *  overwrite.
 *
 * @param int $timestamp (optional)
 *  Unix timestamp. Defaults to what the system detects but you can manually
 *  overwrite.
 *
 * @throws Exception
 *  Throws Exception when $module isn't a Drupal module
 *
 * @return bool $success
 */
function cvwobase_add_audit($message, $tag, $module, $variables = array(), $uid = 0, $referrer = '', $host = '', $timestamp = 0) {
  if (!module_exists($module))
    throw new Exception('Module name is not valid');

	global $user;
  $record = array(
    'uid' => empty($uid) ? $user->uid : $uid,
    'tag' => $tag,
    'module_name' => $module,
    'message' => $message,
    'variables' => $variables,
    'host_name' => empty($host) ? ip_address() : $host,
    'referrer' => substr(empty($referrer) ? $_SERVER['HTTP_REFERER'] : $referrer, 0, 127),
    'timestamp' => empty($timestamp) ? REQUEST_TIME : $timestamp
  );
  return (bool) drupal_write_record(CVWO_AUDIT_TABLE, $record);
}