AUTHOR: Patrick Tan
PERSON IN CHARGE: Patrick
TESTING: Name/Date,
Code Review: Name/Date, 

MODULE: CVWO Base (D7)

CHANGES TO NOTE DURING MIGRATION
- Removed tables:
  - File map: Drupal maintains it own since D6
  - Message Queue: Drupal maintains it own for D7
- Added tables:
  - Audit log: Merged with cvwoaudit

DESCRIPTION: CVWO Base contains common functions used across CVWO modules
 - overrides default mail system to use PEAR SMTP mail; drupal_mail will hence use PEAR
 - provides wrappers for database functions to enforce logging
 - provides an audit trail interface to track changes by users

LIBRARIES INCLUDED:
qTip - load via $element['#attached']['library'][] = array('cvwobase_d7', 'qTip');

DESCRIPTION OF INCLUDE FILES:
 - cvwobase_d7_api.php
   Common functions defined and used in cvwobase_d7
    - cvwobase_download_as_excel($header, $rows, [$filename])
      Outputs specified table as an excel file
    - exception_error_handler($errno, $errstr, $errfile, $errline)
      Converts PHP errors to exceptions
    - cvwobase_exec($cmd, $cwd)
      Executes a command in a separate process
    - findPHPbinary()
      Attempts to find the PHP binary in the environment PATH variable
 - cvwobase_d7_audit_api.php
   Functions used by the audit trail
    - cvwobase_get_audit_modules()
      Returns an indexed array of modules which have entries in {cvwoaudit}
    - cvwobase_get_audit_tags()
      Returns an indexed array of tags that exist in {cvwoaudit}.
    - cvwobase_add_audit($message, $tag, $module, [$uid], [$referrer], [$host], [$timestamp])
      Adds an audit log entry to the database
 - cvwobase_d7_db_api.php
   Logged Database Functions
    - cvwo_query($query, [$args], [$options])
      Wrapper for db_query. Only allows SELECT queries.
    - cvwo_query_range($query, $from, $count, [$args], [$options])
      Wrapper for db_query_range. Only allows SELECT queries.
    - cvwo_select($table, [$alias], [$options])
      Wrapper for db_select. Returns a SelectQuery object.
    - cvwo_insert($table, [$options])
      Modification of db_insert. Returns a CvwoInsertQuery object, which adds
      execute_logged($message, $tag, $module).
    - cvwo_update($table, [$options])
      Modification of db_update. Returns a CvwoUpdateQuery object, which adds
      execute_logged($message, $tag, $module).
    - cvwo_delete($table, [$options])
      Modification of db_delete. Returns a CvwoDeleteQuery object, which adds
      execute_logged($message, $tag, $module).
    - cvwo_merge($table, [$options])
      Modification of db_merge. Returns a CvwoMergeQuery object, which adds
      execute_logged($message, $tag, $module).
    - cvwo_transaction([$name], [$options])
      Wrapper for db_transaction. To use, save the return value of this function in a
      variable. When it goes out of scope (without you calling $transaction->rollback()),
      the transaction is committed.
    - cvwo_write_record($message, $tag, $module, $table, &$record, [$primary_keys])
      Wrapper for drupal_write_record. Will insert / update a record based on the Schema
      defined previously. $record will hold the full record after the function call,
      including default values and serials. To update, specify $primary_keys.
 - CvwoMailSystem.php
   Class definition for CvwoMailSystem. Mail will be sent asynchronously if it is enabled
   and the path to the PHP binary is known. Inline sending will be used otherwise.
 - cvwobase_d7_mail_functions.php
   Functions used by the mail system
 - cvwobase_d7_email_process.php
   Script called as a separate process to send email asynchronously.
 - cvwobase_d7_admin.inc
   Admin page include file
 - cvwobase_d7_audit.inc
   Audit page include file
 - cvwobase_d7.test
   Functional Test Driver for cvwobase_d7