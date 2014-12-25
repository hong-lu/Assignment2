<?php
/*
 * The MIT License
 *
 * Copyright 2013 Computing for Volunteer Welfare Organizations (CVWO),
 * National University of Singapore.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file CVWO Drupal 7 Database API Reloaded
 * @author Joel Low <joel.low@nus.edu.sg>
 *
 * I'm going to try to resolve the problems with the original Database API here.
 * I can't get rid of the old API just yet, because plenty of old code uses the
 * old API and changing the behaviour there will break existing code (in a
 * different way)
 *
 * 1. Database transactions: they MUST default to rollback. Committing an
 *    operation must be explicit on the part of the code writer, because
 *    exceptions, when thrown, MUST roll back the transaction. Defaulting to
 *    commit makes your transaction pass if you don't catch the exception,
 *    which leads to broken code.
 * 2. Conditions: You can specify an empty array to use an 'IN' clause, right?
 *    WRONG. http://drupal.org/node/1976666. The QueryConditionInterface
 *    overrides here will check such conditions and automatically make the
 *    operation apply to nothing (IN <empty-set>) or apply to everything
 *    (NOT IN <empty-set>)
 */

namespace CVWO\Base\Database {
	/**
	 * Represents a Database Transaction. See MySQL's documentation on where
	 * transactions can and cannot be used and what they are for.
	 *
	 * Note that this class differs from Drupal's implementation of
	 * transactions. In Drupal, by default, transactions COMMIT to the database
	 * upon completion. Here, by default, transactions ROLL BACK. This is
	 * because if an error happens within your code that you did not expect
	 * (e.g. exception thrown), your partially completed transaction is saved to
	 * database otherwise, when you really should not let partial data in
	 * because your database would then be inconsistent.
	 *
	 * Because of MySQL's limitations, if one uses nested transactions, one must
	 * be careful that in a nested transaction, there is no full transaction
	 * support. In that situation, if @see rollback is called, the entire
	 * transaction right from the start is rolled back, not just the innermost
	 * transaction. As such, even when not using MySQL, this requirement is
	 * enforced: if rollback is called from any transaction in the stack except
	 * the bottommost transaction, all subsequent transaction must be rolled
	 * back, or else @see commit will throw an exception.
	 */
	class Transaction
	{
		/**
		 * This counts whether we are within a database transaction. Database
		 * transactions can only be committed together with an audit log, so
		 * there should not be any need to use a cvwo_special_query-type
		 * function; instead, wrap it around with an active transaction, and
		 * commit your transaction.
		 *
		 * @var integer The current nesting level of the database transaction.
		 */
		private static $transaction_count = 0;

		/**
		 * Stores whether any of the current transaction stack has been rolled
		 * back.
		 * 
		 * @var boolean Whether any of the transactions in our current stack
		 *              has been rolled back. See the class comments for
		 *              information on how nested transactions (do not) work.
		 */
		private static $rolled_back = false;

		/**
		 * Stores whether this current transaction (one of many in the stack)
		 * should be rolled back.
		 *
		 * @var boolean Whether this transaction should be rolled back.
		 */
		private $roll_back = true;

		/**
		 * The database transaction object that we're wrapping. We cannot
		 * extend the interface because PDO objects cannot be
		 * reinterpret_cast'ed.
		 * 
		 * @var \DatabaseTransaction
		 */
		private $transaction;

		/**
		 * Constructor. You should never need to call this; use the
		 * @see transaction wrapper.
		 */
		public function __construct($name = null, array $options = array())
		{
			$this->transaction = db_transaction($name, $options);

			//Keep track of our transaction depth, as well as set that we have
			//not been rolled back when we are the outermost transaction.
			if (self::$transaction_count === 0)
			{
				self::$rolled_back = false;
			}
			++self::$transaction_count;
		}

		public function __destruct()
		{
			if ($this->roll_back)
			{
				$this->rollback();
			}

			--self::$transaction_count;
			unset($this->transaction);
		}

		/**
		 * Gets whether the current database connection state is that it is
		 * within a transaction.
		 *
		 * @return boolean
		 */
		public static function is_in_transaction()
		{
			return self::$transaction_count !== 0;
		}

		/**
		 * Stores the current transaction to the database.
		 *
		 * If this is a nested transaction, this will not do anything because
		 * MySQL's nested transaction support is broken.
		 *
		 * @param string $message The message to add to the audit log.
		 * @param string $tag The tag to attach to the audit log.
		 * @param string $module The name of the module to attach to the audit
		 *               log.
		 * @param array $variables An array of variables to store with the audit
		 *                         log.
		 */
		public function commit($message, $tag, $module, array $variables = array())
		{
			if (self::$transaction_count === 0)
			{
				//This really shouldn't be possible.
				throw new \Exception('Cannot commit or rollback transaction when '
					. 'none are active.');
			}
			else if (self::$transaction_count === 1)
			{
				if (self::$rolled_back)
				{
					//This also is not possible. As documented in the class
					//comments above, because MySQL's nested transactions
					//doesn't work, we can't allow nested transactions to
					//partially roll back.
					throw new \Exception('Cannot commit transaction when nested'
						. ' transaction was rolled back');
				}
				//We will commit the transaction upon destruction of our object.
				$this->roll_back = false;
				\cvwobase_add_audit($message, $tag, $module, $variables);
			}
			else
			{
				//To work around MySQL's broken nested transactions, we won't do
				//anything here.
				$this->roll_back = false;
			}
		}
		
		/**
		 * Retrieves the name of the transaction or savepoint.
		 */
		public function name()
		{
			return $this->transaction->name;
		}

		/**
		 * Rolls back the current transaction.
		 */
		public function rollback()
		{
			self::$rolled_back = true;
			
			//Don't double roll-back
			$this->roll_back = false;

			$this->transaction->rollback();
		}
	}

	/**
	 * Returns a new transaction object for the active database. If the
	 * transaction is not rolled back, the transaction will be rolled back.
	 *
	 * This is a modification of db_transaction().
	 *
	 * @param string $name Optional name of the transaction.
	 * @param array $options An array of options to control how the transaction
	 *                       operates:
	 *                       - target: The database target name.
	 * @return \CVWO\Base\Database\Transaction A new Transaction object for the
	 *                                         connection.
	 */
	function transaction($name = null, array $options = array())
	{
		return new Transaction($name, $options);
	}

	/**
	 * Extension of SelectQuery: Allow condition to take empty arrays.
	 */
	class SelectQuery extends \SelectQuery
	{
		public function condition($field, $value = null, $operator = null)
		{
			Internal\condition_hook($field, $value, $operator);
			return parent::condition($field, $value, $operator);
		}
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
	 * @return \CVWO\Base\Database\SelectQuery
	 *   A new SelectQuery object for this connection.
	 *
	 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_select/7
	 * @see http://api.drupal.org/api/drupal/includes--database--select.inc/class/SelectQuery/7
	 */
	function select($table, $alias = null, array $options = array())
	{
		return Internal\reinterpret_cast(__NAMESPACE__ . '\SelectQuery',
			db_select($table, $alias, $options));
	}

	/**
	 * Extension of InsertQuery: Makes all execute by default require audit log
	 * information, EXCEPT when a transaction is active (because to commit then
	 * you need to give a log statement)
	 */
	class InsertQuery extends \InsertQuery
	{
		public function execute()
		{
			//Get arguments and fill in with default values.
			$arguments = func_get_args();
			for ($i = count($arguments); $i < 8; ++$i)
			{
				$arguments[$i] = $i === 3 ? array() : null;
			}

			return $this->execute_logged($arguments[0], $arguments[1], $arguments[2],
				$arguments[3], $arguments[4], $arguments[5], $arguments[6], $arguments[7]);
		}

		public function execute_logged($message, $tag, $module, array $variables = array(),
			$uid = 0, $referrer = '', $host = '', $timestamp = 0)
		{
			$transaction = Transaction::is_in_transaction() ?
				null : \db_transaction();
			try
			{
				if (!is_null($retval = parent::execute()) &&
					!Transaction::is_in_transaction())
				{
					\cvwobase_add_audit($message, $tag, $module, $variables, $uid,
						$referrer, $host, $timestamp);
				}

				return $retval;
			}
			catch (Exception $e)
			{
				if (!empty($transaction))
				{
					$transaction->rollback();
				}
				throw $e;
			}
		}

		/**
		 * Executes the query without audit logging.
		 * 
		 * @return
		 *   The last insert ID of the query, if one exists. If the query
		 *   was given multiple sets of values to insert, the return value is
		 *   undefined. If no fields are specified, this method will do nothing and
		 *   return NULL. That makes it safe to use in multi-insert loops.
		 */
		public function execute_special()
		{
			return parent::execute();
		}

		public function condition($field, $value = null, $operator = null)
		{
			Internal\condition_hook($field, $value, $operator);
			return parent::condition($field, $value, $operator);
		}
	}

	/**
	 * Returns a new InsertQuery object for the active database.
	 *
	 * This is a modification of db_insert().
	 *
	 * @param $table
	 *   The table into which to insert.
	 * @param $options
	 *   An array of options to control how the query operates.
	 *
	 * @return \CVWO\Base\Database\InsertQuery
	 *   A new InsertQuery object for this connection.
	 *
	 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_insert/7
	 * @see http://api.drupal.org/api/drupal/includes--database--query.inc/class/InsertQuery/7
	 */
	function insert($table, array $options = array())
	{
		return Internal\reinterpret_cast(__NAMESPACE__ . '\InsertQuery',
			db_insert($table, $options));
	}

	/**
	 * Extension of UpdateQuery: Makes all execute by default require audit log
	 * information, EXCEPT when a transaction is active (because to commit then
	 * you need to give a log statement)
	 */
	class UpdateQuery extends \UpdateQuery
	{
		public function execute()
		{
			//Get arguments and fill in with default values.
			$arguments = func_get_args();
			for ($i = count($arguments); $i < 8; ++$i)
			{
				$arguments[$i] = $i === 3 ? array() : null;
			}

			return $this->execute_logged($arguments[0], $arguments[1], $arguments[2],
				$arguments[3], $arguments[4], $arguments[5], $arguments[6], $arguments[7]);
		}

		public function execute_logged($message, $tag, $module, array $variables = array(),
			$uid = 0, $referrer = '', $host = '', $timestamp = 0)
		{
			$transaction = Transaction::is_in_transaction() ?
				null : \db_transaction();
			try
			{
				if (($retval = parent::execute()) !== 0 &&
					!Transaction::is_in_transaction())
				{
					\cvwobase_add_audit($message, $tag, $module, $variables, $uid,
						$referrer, $host, $timestamp);
				}

				return $retval;
			}
			catch (Exception $e)
			{
				if (!empty($transaction))
				{
					$transaction->rollback();
				}
				throw $e;
			}
		}

		/**
		 * Executes the query without audit logging.
		 * @return
		 *   The number of rows affected by the update.
		 */
		public function execute_special()
		{
			return parent::execute();
		}
		
		public function condition($field, $value = null, $operator = null)
		{
			Internal\condition_hook($field, $value, $operator);
			return parent::condition($field, $value, $operator);
		}
	}

	/**
	 * Returns a new UpdateQuery object for the active database.
	 *
	 * This is a modification of db_update().
	 *
	 * @param $table
	 *   The table to update.
	 * @param $options
	 *   An array of options to control how the query operates.
	 *
	 * @return \CVWO\Base\Database\UpdateQuery
	 *   A new UpdateQuery object for this connection.
	 *
	 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_update/7
	 * @see http://api.drupal.org/api/drupal/includes--database--query.inc/class/UpdateQuery/7
	 */
	function update($table, array $options = array())
	{
		return Internal\reinterpret_cast(__NAMESPACE__ . '\UpdateQuery',
			db_update($table, $options));
	}

	/**
	 * Extension of MergeQuery: Makes all execute by default require audit log
	 * information, EXCEPT when a transaction is active (because to commit then
	 * you need to give a log statement)
	 */
	class MergeQuery extends \MergeQuery
	{
		public function execute()
		{
			//Get arguments and fill in with default values.
			$arguments = func_get_args();
			for ($i = count($arguments); $i < 8; ++$i)
			{
				$arguments[$i] = $i === 3 ? array() : null;
			}

			return $this->execute_logged($arguments[0], $arguments[1], $arguments[2],
				$arguments[3], $arguments[4], $arguments[5], $arguments[6], $arguments[7]);
		}

		public function execute_logged($message, $tag, $module, array $variables = array(),
			$uid = 0, $referrer = '', $host = '', $timestamp = 0)
		{
			$transaction = Transaction::is_in_transaction() ?
				null : \db_transaction();
			try
			{
				if (($retval = parent::execute()) !== 0 &&
					!Transaction::is_in_transaction())
				{
					\cvwobase_add_audit($message, $tag, $module, $variables, $uid,
						$referrer, $host, $timestamp);
				}

				return $retval;
			}
			catch (Exception $e)
			{
				if (!empty($transaction))
				{
					$transaction->rollback();
				}
				throw $e;
			}
		}

		/**
		 * Executes the query without audit logging.
		 * @return
		 *   The number of rows affected by the update.
		 */
		public function execute_special()
		{
			return parent::execute();
		}
		
		public function condition($field, $value = null, $operator = null)
		{
			Internal\condition_hook($field, $value, $operator);
			return parent::condition($field, $value, $operator);
		}
	}

	/**
	 * Returns a new MergeQuery object for the active database.
	 *
	 * This is a modification of db_merge().
	 *
	 * @param $table
	 *   The table to update.
	 * @param $options
	 *   An array of options to control how the query operates.
	 *
	 * @return \CVWO\Base\Database\MergeQuery
	 *   A new MergeQuery object for this connection.
	 *
	 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_merge/7
	 * @see http://api.drupal.org/api/drupal/includes--database--query.inc/class/MergeQuery/7
	 */
	function merge($table, array $options = array())
	{
		return Internal\reinterpret_cast(__NAMESPACE__ . '\MergeQuery',
			db_merge($table, $options));
	}

	/**
	 * Extension of DeleteQuery: Makes all execute by default require audit log
	 * information, EXCEPT when a transaction is active (because to commit then
	 * you need to give a log statement)
	 */
	class DeleteQuery extends \DeleteQuery
	{
		public function execute()
		{
			//Get arguments and fill in with default values.
			$arguments = func_get_args();
			for ($i = count($arguments); $i < 8; ++$i)
			{
				$arguments[$i] = $i === 3 ? array() : null;
			}

			return $this->execute_logged($arguments[0], $arguments[1], $arguments[2],
				$arguments[3], $arguments[4], $arguments[5], $arguments[6], $arguments[7]);
		}
		
		public function execute_logged($message, $tag, $module, array $variables = array(),
			$uid = 0, $referrer = '', $host = '', $timestamp = 0)
		{
			$transaction = Transaction::is_in_transaction() ?
				null : \db_transaction();
			try
			{
				$retval = parent::execute();
				if (!Transaction::is_in_transaction())
				{
					\cvwobase_add_audit($message, $tag, $module, $variables, $uid,
						$referrer, $host, $timestamp);
				}
			}
			catch (Exception $e)
			{
				if (!empty($transaction))
				{
					$transaction->rollback();
				}
				throw $e;
			}

			return $retval;
		}

		/**
		 * Executes the query without audit logging.
		 *
		 * @return
		 *   The return value is dependent on the database connection.
		 */
		public function execute_special()
		{
			return parent::execute();
		}

		public function condition($field, $value = null, $operator = null)
		{
			Internal\condition_hook($field, $value, $operator);
			return parent::condition($field, $value, $operator);
		}
	}

	/**
	 * Returns a new DeleteQuery object for the active database.
	 *
	 * This is a modification of db_delete().
	 *
	 * @param $table
	 *   The table from which to delete.
	 * @param $options
	 *   An array of options to control how the query operates.
	 *
	 * @return \CVWO\Base\Database\DeleteQuery
	 *   A new DeleteQuery object for this connection.
	 *
	 * @see http://api.drupal.org/api/drupal/includes--database--database.inc/function/db_delete/7
	 * @see http://api.drupal.org/api/drupal/includes--database--query.inc/class/DeleteQuery/7
	 */
	function delete($table, array $options = array())
	{
		return Internal\reinterpret_cast(__NAMESPACE__ . '\DeleteQuery',
			db_delete($table, $options));
	}
	
	/**
	 * Extension of DatabaseCondition: Handle condition() clauses with empty
	 * arrays without generating invalid SQL.
	 */
	class DatabaseCondition extends \DatabaseCondition
	{
		public function condition($field, $value = null, $operator = null)
		{
			Internal\condition_hook($field, $value, $operator);
			return parent::condition($field, $value, $operator);
		}
	}
	
	/**
	 * Returns a new DatabaseCondition, set to "OR" all conditions together.
	 *
	 * @return DatabaseCondition
	 */
	function or_()
	{
		return new DatabaseCondition('OR');
	}

	/**
	 * Returns a new DatabaseCondition, set to "AND" all conditions together.
	 *
	 * @return DatabaseCondition
	 */
	function and_()
	{
		return new DatabaseCondition('AND');
	}

	/**
	 * Returns a new DatabaseCondition, set to "XOR" all conditions together.
	 *
	 * @return DatabaseCondition
	 */
	function xor_()
	{
		return new DatabaseCondition('XOR');
	}

	/**
	 * Returns a new DatabaseCondition, set to the specified conjunction.
	 *
	 * Internal API function call.  The db_and(), db_or(), and db_xor()
	 * functions are preferred.
	 *
	 * @param $conjunction
	 *   The conjunction to use for query conditions (AND, OR or XOR).
	 * @return DatabaseCondition
	 */
	function condition($conjunction)
	{
		return new DatabaseCondition($conjunction);
	}

	/**
	 * Declare that the provided schema table will record the user modified, user
	 * created, and modified and created timestamps.
	 *
	 * For tables using this, remember to set the date_created field to NULL
	 * when the record is first inserted.
	 *
	 * @param array $table A table in the Drupal hook_schema callback.
	 */
	function table_include_timestamps(&$table)
	{
		$table['fields'] = array_merge(
			isset($table['fields']) ? $table['fields'] : array(),
			array(
				'date_modified'	 => array(
					'type'			 => 'timestamp',
					'mysql_type'	 => 'timestamp',
					'not null'		 => true,
					'description'	 => t('The date that this record is modified in the database. This uses a MySQL behaviour where the first timestamp will be set to the time the field was last updated.')
				),
				'user_modified'	 => array(
					'type'			 => 'int',
					'unsigned'		 => true,
					'not null'		 => true,
					'description'	 => t('The uid of the person who modified the record.')
				),
				'date_created'	 => array(
					'type'			 => 'timestamp',
					'mysql_type'	 => 'timestamp',
					'not null'		 => true,
					'description'	 => t('The date that this record is created to the database. This uses a MySQL behaviour where inserting a NULL into a timestamp uses the current timestamp. Remember to insert this column with NULLs')
				),
				'user_created'	 => array(
					'type'			 => 'int',
					'unsigned'		 => true,
					'not null'		 => true,
					'description'	 => t('The uid of the person who created the record.')
				)
			)
		);

		$table['foreign keys'] = array_merge(
			isset($table['foreign keys']) ? $table['foreign keys'] : array(),
			array(
				'user_modified_relation' => array(
					'table'			 => \CVWO\Base\PERSON_TABLE,
					'user_modified'	 => 'person_id'
				),
				'user_created_relation' => array(
					'table'			 => \CVWO\Base\PERSON_TABLE,
					'user_created'	 => 'person_id'
				),
			)
		);
	}

	/**
	 * Declare that the provided schema table will not see deletions and instead
	 * will be deleted by setting a flag.
	 *
	 * @param array $table A table in the Drupal hook_schema callback.
	 */
	function table_include_soft_delete(&$table)
	{
		$table['fields'] = array_merge(
			isset($table['fields']) ? $table['fields'] : array(),
			array(
				'is_deleted' => array(
					'type' => 'int',
					'size' => 'tiny',
					'not null' => true,
					'default' => 0,
					'description' => t('0 for undeleted entries, nonzero for deleted entries.'),
				)
			)
		);
	}

	/**
	 * Declare that the provided schema table will store a value that changes
	 * over time.
	 *
	 * @param array $table A table in the Drupal hook_schema callback.
	 * @param array $keys The primary keys from the original table, this will
	 *        be paired with the timestamp so that there is no chance of having
	 *        two records for the same time.
	 */
	function table_make_incremental(&$table, array $keys)
	{
		$table['fields'] = array_merge(
			isset($table['fields']) ? $table['fields'] : array(),
			array(
				'effective'			 => array(
					'type'			 => 'timestamp',
					'mysql_type'	 => 'timestamp',
					'not null'		 => true,
					'description'	 => t('The effective date/time which this record takes effect. This status will be superseded when another entry with a later effective time is inserted.')
				)
			)
		);
		
		$table['unique keys'] = array_merge(
			isset($table['unique keys']) ? $table['unique keys'] : array(),
			array(
				'unique_effective'	 => array_merge(
					$keys,
					array('effective')
				)
			)
		);
	}
}

namespace CVWO\Base\Database\Internal {
	/**
	 * Just like the C++ counterpart. Very unsafe. But seems like it's necessary
	 * once in a while.
	 */
	function reinterpret_cast($class, $object)
	{
		return unserialize(preg_replace('/^O:\d+:"[^"]++"/', 'O:' . strlen($class) . ':"' . $class . '"', serialize($object)));
	}

	/**
	 * Hooks the condition function in the QueryConditionInterface to check for
	 * empty sets in an IN or NOT IN statement.
	 * 
	 * @param string $field The field the condition refers to.
	 * @param mixed $value The value to compare against.
	 * @param string $operation The operation to apply to the value.
	 */
	function condition_hook(&$field, &$value, &$operation)
	{
		//We only care about IN or NOT IN statements.
		if ((($operation === null && is_array($value)) ||
			strtoupper($operation) === 'IN' ||
			strtoupper($operation) === 'NOT IN') &&
			
			//And if the value is the empty set.
			empty($value))
		{
			//Nothing is in the empty set.
			if (is_null($operation) || strtoupper($operation) === 'IN')
			{
				$value = null;
				$field = db_and()->where('FALSE');
			}

			//Everything is not in the empty set.
			else if (strtoupper($operation) === 'NOT IN')
			{
				$value = null;
				$field = db_and()->where('TRUE');
			}
		}
	}
}
