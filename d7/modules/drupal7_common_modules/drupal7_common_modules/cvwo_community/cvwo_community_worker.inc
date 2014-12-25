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
 * @file
 * This represents a Worker. Workers might be a volunteer or a full-time employee.
 * Ths reason this is necessary is just because we need to categorise the abstract
 * CVWO person into workers and clients, so this handles the workers bit.
 */

namespace CVWO\Community {
	use \CVWO\Base\Database as db;

	/**
	 * Searches for a person using either an incomplete NRIC or partial name.
	 *
	 * @param string $text The name or NRIC fragment to search for.
	 * @param function $modifier A function callback to use to modify the query
	 *                           before execute is called.
	 */
	function worker_search_callback($text, $modifier = null)
	{
		$modifier2 = function(&$query) use (&$modifier)
		{
			if (isset($modifier) && is_callable($modifier))
			{
				$modifier($query);
			}
			$query->addJoin('INNER', WORKER_TABLE, 's', 'p.person_id=s.person_id');
			$query->condition('s.is_deleted', 0);
		};

		return \CVWO\Base\person_search_callback($text, $modifier2);
	}

	/**
	 * Class extension for the Abstract CVWO Person. No new behaviours here.
	 */
	class Worker extends \CVWO\Base\Person
	{
		/**
		 * Creates a new Worker person from their database ID.
		 *
		 * @param integer $person_id
		 */
		public static function from_id($person_id)
		{
			if (!self::exists($person_id))
			{
				throw new \InvalidArgumentException('The person ID provided is '
					. 'an invalid Community Worker ID');
			}

			return new Worker($person_id);
		}

		/**
		 * Checks if the person with the given ID exists.
		 *
		 * @param integer $person_id
		 * @return boolean
		 */
		public static function exists($person_id)
		{
			$count = intval(db\select(WORKER_TABLE)
				->condition('person_id', $person_id)
				->countQuery()
				->execute()
				->fetchField());

			return $count === 1;
		}
		
		/**
		 * Deletes the current person from the database.
		 * 
		 * This must be wrapped around a database transaction.
		 */
		public function delete()
		{
			db\update(WORKER_TABLE)
				->fields(array('is_deleted' => 1, 'user_modified' => $GLOBALS['user']->uid))
				->condition('person_id', $this->person_id)
				->execute();

			//Check that the person being deleted is not also a person, so that if
			//he is, we won't delete the base person.
			if (intval(db\select(PERSON_TABLE)
				->condition('person_id', $this->person_id)
				->condition('is_deleted', 0)
				->countQuery()
				->execute()
				->fetchField()) === 0)
			{
				parent::delete();
			}
		}
	}

	class WorkerEditForm extends \CVWO\Base\PersonEditForm
	{
		/**
		 * Stores whether we have to insert at the next process.
		 * @var boolean
		 */
		private $is_insertion;

		public function __construct($person_id = null)
		{
			//Check that we have such a worker.
			if (!empty($person_id))
			{
				$query = db\select(WORKER_TABLE)
					->condition('person_id', $person_id)
					->countQuery()
					->execute();
				if (intval($query->fetchField()) !== 1)
				{
					throw new \InvalidArgumentException();
				}
			}
			
			parent::__construct($person_id);
		}

		protected function do_build(&$form_state)
		{
			$this->is_insertion = is_null($this->get_id());
			parent::do_build($form_state);
		}

		protected function do_process(&$form_state)
		{
			$transaction = db\transaction();

			parent::do_process($form_state);
			
			//Insert a staff member if this is an insertion
			if ($this->is_insertion)
			{
				$this->add();
			}

			//This should not cause a transaction commit in release builds.
			//This class needs to be extended to be meaningful in release!
			$transaction->commit(
				PERSON_ADD_LOG_MESSAGE,
				PERSON_PERSONAL_PARTICULARS_TAG,
				DEBUG ? MODULE_NAME : null,
				array('%person' => $form_state['values']['name'])
			);
		}
		
		private function add()
		{
			db\insert(WORKER_TABLE)
				->fields(array(
					'person_id' => $this->get_id(),
					'user_created' => $GLOBALS['user']->uid,
					'user_modified' => $GLOBALS['user']->uid,
					'date_created' => null
				))
				->execute();
		}
	}
}
