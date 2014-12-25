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
 * CVWO extensions of Drupal database query methods and classes
 */

require_once drupal_get_path('module', CVWOBASE_MODULE).'/cvwobase_d7_audit_api.php';
define('CVWO_DATABASE_NON_SELECT', 'CVWO query exception - Non-SELECT query');

/**
 * Extension of InsertQuery: adds execute_logged(), which should be used whenever possible, so as to log changes
 */
class CvwoInsertQuery extends InsertQuery {
  public function execute_logged($message, $tag, $module, $variables = array()) {
    if (!is_null($retval = parent::execute()))
      cvwobase_add_audit($message, $tag, $module, $variables);
    return $retval;
  }
}

/**
 * Extension of UpdateQuery: adds execute_logged(), which should be used whenever possible, so as to log changes
 */
class CvwoUpdateQuery extends UpdateQuery {
  public function execute_logged($message, $tag, $module, $variables = array()) {
    if (($retval = parent::execute()) !== 0)
      cvwobase_add_audit($message, $tag, $module, $variables);
    return $retval;
  }
}

/**
 * Extension of MergeQuery: adds execute_logged(), which should be used whenever possible, so as to log changes
 */
class CvwoMergeQuery extends MergeQuery {
  public function execute_logged($message, $tag, $module, $variables = array()) {
    if ($retval = parent::execute())
      cvwobase_add_audit($message, $tag, $module, $variables);
    return $retval;
  }
}

/**
 * Extension of DeleteQuery: adds execute_logged(), which should be used whenever possible, so as to log changes
 */
class CvwoDeleteQuery extends DeleteQuery {
  public function execute_logged($message, $tag, $module, $variables = array()) {
    $retval = parent::execute();
    cvwobase_add_audit($message, $tag, $module, $variables);
    return $retval;
  }
}

/**
 * Extension of DatabaseTransaction: logs just before committing transaction,
 * unless it was rolled back.
 * 
 * The default of this class is to commit transactions, which prevents proper
 * error handling. Use the new CVWO\Common\Database\Transaction API.
 *
 * @deprecated since version 2.0
 */
class CvwoTransaction {
  public $message;
  public $tag;
  public $module;
  public $variables;
  protected $rolledBack = FALSE;
  protected $txn;
  
  public function __construct($message, $tag, $module, $variables = array(), $name = NULL, array $options = array()) {
    $this->message = $message;
    $this->tag = $tag;
    $this->module = $module;
    $this->variables = $variables;
    $this->txn = db_transaction($name, $options);
  }
  
  public function rollback() {
    $this->rolledBack = TRUE;
    $this->txn->rollback();
  }
  
  public function __destruct() {
    if (!$this->rolledBack)
      cvwobase_add_audit($this->message, $this->tag, $this->module, $this->variables);
  }
  
  public function __call($name, $arguments) {
    return call_user_func_array(array($this->txn, $name), $arguments);
  }
}

/**
 * Executes an arbitrary query string against the active database.
 *
 * Use this function for SELECT queries if it is just a simple query string.
 * If the caller or other modules need to change the query, use cvwo_select()
 * instead.
 *
 * Do not use this function for INSERT, UPDATE, or DELETE queries. Those should
 * be handled via cvwo_insert(), cvwo_update() and cvwo_delete() respectively.
 *
 * This is a wrapper for db_query(), allowing only SELECT queries.
 *
 * @param $query
 *   The prepared statement query to run. Although it will accept both named and
 *   unnamed placeholders, named placeholders are strongly preferred as they are
 *   more self-documenting.
 * @param $args
 *   An array of values to substitute into the query. If the query uses named
 *   placeholders, this is an associative array in any order. If the query uses
 *   unnamed placeholders (?), this is an indexed array and the order must match
 *   the order of placeholders in the query string.
 * @param $options
 *   An array of options to control how the query operates.
 *
 * @return DatabaseStatementInterface
 *   A prepared statement object, already executed.
 *
 * @see DatabaseConnection::defaultOptions()
 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_query/7
 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/interface/DatabaseStatementInterface/7
 */
function cvwo_query($query, array $args = array(), array $options = array()) {
  if (strstr(strtoupper(trim($query)), ' ', TRUE) != 'SELECT')
    throw new Exception(CVWO_DATABASE_NON_SELECT);
  return db_query($query, $args, $options);
}

/**
 * Executes a query against the active database, restricted to a range.
 *
 * This is a wrapper for db_query_range(), allowing only SELECT queries.
 *
 * @param $query
 *   The prepared statement query to run. Although it will accept both named and
 *   unnamed placeholders, named placeholders are strongly preferred as they are
 *   more self-documenting.
 * @param $from
 *   The first record from the result set to return.
 * @param $count
 *   The number of records to return from the result set.
 * @param $args
 *   An array of values to substitute into the query. If the query uses named
 *   placeholders, this is an associative array in any order. If the query uses
 *   unnamed placeholders (?), this is an indexed array and the order must match
 *   the order of placeholders in the query string.
 * @param $options
 *   An array of options to control how the query operates.
 *
 * @return DatabaseStatementInterface
 *   A prepared statement object, already executed.
 *
 * @see DatabaseConnection::defaultOptions()
 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_query_range/7
 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/interface/DatabaseStatementInterface/7
 */
function cvwo_query_range($query, $from, $count, array $args = array(), array $options = array()) {
  if (strstr(strtoupper(trim($query)), ' ', TRUE) != 'SELECT')
    throw new Exception(CVWO_DATABASE_NON_SELECT);
  return db_query_range($query, $from, $count, $args, $options);
}

/**
 * Returns a new SelectQuery object for the active database.
 *
 * This is a wrapper for db_select().
 *
 * @param $table
 *   The base table for this query. May be a string or another SelectQuery
 *   object. If a query object is passed, it will be used as a subselect.
 * @param $alias
 *   The alias for the base table of this query.
 * @param $options
 *   An array of options to control how the query operates.
 *
 * @return SelectQuery
 *   A new SelectQuery object for this connection.
 *
 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_select/7
 * @see http://api.drupal.org/api/drupal/includes--database--select.inc/class/SelectQuery/7
 */
function cvwo_select($table, $alias = NULL, array $options = array()) {
  return db_select($table, $alias, $options);
}

/**
 * Returns a new CvwoInsertQuery object for the active database.
 *
 * This is a modification of db_insert().
 *
 * @param $table
 *   The table into which to insert.
 * @param $options
 *   An array of options to control how the query operates.
 *
 * @return CvwoInsertQuery
 *   A new CvwoInsertQuery object for this connection. Instead of
 *   InsertQuery::execute(), please use CvwoInsertQuery::execute_logged($message, $tag, $module).
 *
 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_insert/7
 * @see http://api.drupal.org/api/drupal/includes--database--query.inc/class/InsertQuery/7
 */
function cvwo_insert($table, array $options = array()) {
  return _casttoclass('CvwoInsertQuery', db_insert($table, $options));
}

/**
 * Returns a new CvwoUpdateQuery object for the active database.
 *
 * This is a modification of db_update().
 *
 * @param $table
 *   The table to update.
 * @param $options
 *   An array of options to control how the query operates.
 *
 * @return CvwoUpdateQuery
 *   A new CvwoUpdateQuery object for this connection. Instead of
 *   UpdateQuery::execute(), please use CvwoUpdateQuery::execute_logged($message, $tag, $module).
 *
 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_update/7
 * @see http://api.drupal.org/api/drupal/includes--database--query.inc/class/UpdateQuery/7
 */
function cvwo_update($table, array $options = array()) {
  return _casttoclass('CvwoUpdateQuery', db_update($table, $options));
}

/**
 * Returns a new CvwoMergeQuery object for the active database.
 *
 * This is a modification of db_merge().
 *
 * @param $table
 *   The table into which to merge.
 * @param $options
 *   An array of options to control how the query operates.
 *
 * @return MergeQuery
 *   A new MergeQuery object for this connection.
 */
function cvwo_merge($table, array $options = array()) {
  return _casttoclass('CvwoMergeQuery', db_merge($table, $options));
}

/**
 * Returns a new CvwoDeleteQuery object for the active database.
 *
 * This is a modification of db_delete().
 *
 * @param $table
 *   The table from which to delete.
 * @param $options
 *   An array of options to control how the query operates.
 *
 * @return CvwoDeleteQuery
 *   A new CvwoDeleteQuery object for this connection. Instead of
 *   DeleteQuery::execute(), please use CvwoDeleteQuery::execute_logged($message, $tag, $module).
 *
 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_delete/7
 * @see http://api.drupal.org/api/drupal/includes--database--query.inc/class/DeleteQuery/7
 */
function cvwo_delete($table, array $options = array()) {
  return _casttoclass('CvwoDeleteQuery', db_delete($table, $options));
}

/**
 * Returns a new transaction object for the active database. If the transaction is not rolled
 * back, an audit entry will be inserted just before committing the transaction.
 *
 * This is a modification of db_transaction().
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
 * @param string $name
 *   Optional name of the transaction.
 *
 * @param array $options
 *   An array of options to control how the transaction operates:
 *   - target: The database target name.
 *
 * @return DatabaseTransaction
 *   A new DatabaseTransaction object for this connection.
 *
 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_transaction/7
 * @deprecated since version 2.0
 */
function cvwo_transaction($message, $tag, $module, $variables = array(), $name = NULL, array $options = array()) {
  return new CvwoTransaction($message, $tag, $module, $variables, $name, $options);
}

/**
 * Saves (inserts or updates) a record to the database based upon the schema.
 *
 * This is a wrapper for drupal_write_record().
 *
 * @param string $message
 *  A string containing your message
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
 * @param $table
 *   The name of the table; this must be defined by a hook_schema()
 *   implementation.
 * @param $record
 *   An object or array representing the record to write, passed in by
 *   reference. If inserting a new record, values not provided in $record will
 *   be populated in $record and in the database with the default values from
 *   the schema, as well as a single serial (auto-increment) field (if present).
 *   If updating an existing record, only provided values are updated in the
 *   database, and $record is not modified.
 * @param $primary_keys
 *   To indicate that this is a new record to be inserted, omit this argument.
 *   If this is an update, this argument specifies the primary keys' field
 *   names. If there is only 1 field in the key, you may pass in a string; if
 *   there are multiple fields in the key, pass in an array.
 *
 * @return
 *   If the record insert or update failed, returns FALSE. If it succeeded,
 *   returns SAVED_NEW or SAVED_UPDATED, depending on the operation performed.
 */
function cvwo_write_record($message, $tag, $module, $variables = array(), $table, &$record, $primary_keys = array()) {
  if ($ret = drupal_write_record($table, $record, $primary_keys))
    cvwobase_add_audit($message, $tag, $module, $variables);
  return $ret;
}

/**
 * Cast an object to another class, keeping the properties, but changing the methods
 *
 * @param string $class  Class name
 * @param object $object
 * @return object
 */
function _casttoclass($class, $object) {
	return unserialize(preg_replace('/^O:\d+:"[^"]++"/', 'O:' . strlen($class) . ':"' . $class . '"', serialize($object)));
}