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

namespace CVWO\Community {
	use CVWO\Base\Database as db;

	/**
	 * Searches for a person using either an incomplete NRIC or partial name.
	 *
	 * @param string $text The name or NRIC fragment to search for.
	 * @param function $modifier A function callback to use to modify the query
	 *                           before execute is called.
	 */
	function person_search_callback($text, $modifier = null)
	{
		$modifier2 = function(&$query) use (&$modifier)
		{
			if (isset($modifier) && is_callable($modifier))
			{
				$modifier($query);
			}
			$query->innerJoin(PERSON_TABLE, 'cp', 'p.person_id=cp.person_id');
			$query->condition('cp.is_deleted', 0);
		};

		return \CVWO\Base\person_search_callback($text, $modifier2);
	}

	/**
	 * Builds the person edit form.
	 *
	 * @param array $form The form to build into.
	 * @param array $form_state The current state of the form.
	 * @param integer $id The ID of the person to edit.
	 * @return array The compiled Drupal form.
	 */
	function person_edit_form($form, &$form_state, $id = null)
	{
		try
		{
			$instance = new PersonEditForm($id);
			return $instance->build($form_state);
		}
		catch (\InvalidArgumentException $e)
		{
			drupal_not_found();
			drupal_exit();
		}
	}

	/**
	 * Validates the person edit form.
	 *
	 * @param array $form The Drupal form
	 * @param array $form_state The Drupal state of the form.
	 */
	function person_edit_form_validate(&$form, &$form_state)
	{
		$instance = $form['#instance'];
		$instance->validate($form, $form_state);
	}

	/**
	 * Handles submissions of the person edit form.
	 *
	 * @param array $form The form built.
	 * @param array $form_state The state of the form submittal.
	 */
	function person_edit_form_submit($form, &$form_state)
	{
		$instance = $form['#instance'];
		$instance->process($form_state);
	}

	function person_nok_form($form, &$form_state, $person_id, $person_nok_id = null)
	{
		return \CVWO\Base\person_nok_form($form, $form_state, $person_id, $person_nok_id);
	}

	function person_nok_form_validate($form, &$form_state)
	{
		return \CVWO\Base\person_nok_form_validate($form, $form_state);
	}

	function person_nok_form_submit($form, &$form_state)
	{
		return \CVWO\Base\person_nok_form_submit($form, $form_state);
	}

	class PersonEditForm extends \CVWO\Base\PersonEditForm
	{
		public function __construct($person_id = null)
		{
			//Check that we have such a person.
			if (!empty($person_id))
			{
				$query = db\select(PERSON_TABLE)
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
			parent::do_build($form_state);

			//Remove the submit button.
			$submit = &$this->entries['submit'];
			unset($this->entries['submit']);

			//Add our fields.
			$values								 = empty($form_state['values']) ?
				array() : $form_state['values'];
			$this->entries['personal_situation'] = array(
				'#type'			 => 'fieldset',
				'#title'		 => t('Personal Situation'),
				'#collapsible'	 => true
			);
			
			$personal_situation_form = new PersonSituationForm();
			$this->add_child($personal_situation_form);
			$this->entries['personal_situation'] =
				array_merge_recursive($this->entries['personal_situation'],
					$personal_situation_form->build($form_state));

			//Restore the submit button
			$this->entries['submit'] = &$submit;
			unset($submit);
		}

		protected function do_process(&$form_state)
		{
			$transaction = db\transaction();

			parent::do_process($form_state);

			//This should not cause a transaction commit in release builds.
			//This class needs to be extended to be meaningful in release!
			$transaction->commit(
				PERSON_ADD_LOG_MESSAGE,
				PERSON_PERSONAL_PARTICULARS_TAG,
				DEBUG ? MODULE_NAME : null,
				array('%person' => $form_state['values']['name'])
			);
		}
	}

	class PersonSituationForm extends \CVWO\Base\Form
	{
		/**
		 * Stores whether we have to insert at the next process.
		 * @var boolean
		 */
		private $is_insertion;

		/**
		 * Whether the current form is disabled.
		 * 
		 * @var boolean
		 */
		private $disabled;

		public function __construct($disabled = false)
		{
			parent::__construct();
			$this->disabled = $disabled;
		}

		protected function do_build(&$form_state)
		{
			$values = &$form_state['values'];
			$this->is_insertion = is_null($this->get_parent()->get_id());
			if (empty($form_state['input']))
			{
				$this->load($this->get_parent()->get_id(), $form_state);
			}

			$form['living_pattern']	 = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Living Pattern'),
				'#required'		 => true,
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSON_LIVING_PATTERN_QUESTION,
				'#default_value' => isset($values['living_pattern']) ?
					$values['living_pattern'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['movement_ability']	 = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('Movement Ability'),
				'#required'		 => true,
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSON_MOVEMENT_ABILITY_QUESTION,
				'#default_value' => isset($values['movement_ability']) ?
					$values['movement_ability'] : null,
				'#disabled'		 => $this->disabled
			);

			$form['at_risk'] = array(
				'#type'			 => 'cvwocodem2_select',
				'#title'		 => t('At risk'),
				'#required'		 => true,
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSON_AT_RISK_QUESTION,
				'#default_value' => isset($values['at_risk']) ?
					$values['at_risk'] : null,
				'#disabled'		 => $this->disabled
			);

			$options = cvwocodem2_getoptions2(MODULE_NAME,
				PERSON_FINANCIAL_SITUATION_QUESTION,0,false);
			$form['financial_situation'] = array(
				'#type'			 => 'checkboxes',
				'#title'		 => t('Financial Situation'),
				'#default_value' => isset($values['financial_situation']) ?
					$values['financial_situation'] : array(),
				'#options'		 => $options,
				'#disabled'		 => $this->disabled
			);

			$options = cvwocodem2_getoptions2(MODULE_NAME,
				PERSON_OTHER_HEALTH_CONDITION_QUESTION,0,false);
			$options					 = $options;
			$form['health_condition']	 = array(
				'#type'			 => 'checkboxes',
				'#title'		 => t('Health Condition'),
				'#default_value' => isset($values['health_condition']) ?
					$values['health_condition'] : array(),
				'#options'		 => $options,
				'#disabled'		 => $this->disabled
			);

			$this->entries = $form;
		}

		protected function do_process(&$form_state)
		{
			if ($this->is_insertion)
			{
				$this->add($form_state);
			}
			else
			{
				$this->update($this->get_parent()->get_id(), $form_state);
			}
		}

		/**
		 * Fills the form from the database.
		 */
		private function load($person_id, &$form_state)
		{
			$values = &$form_state['values'];
			$fields = array('person_id', 'living_pattern', 'movement_ability');

			$query = db\select(PERSON_TABLE)
				->condition('person_id', $person_id);
			$query->fields(PERSON_TABLE, $fields);
			$query = $query->execute();
			$row = $query->fetchAssoc();
			foreach ($fields as $field)
			{
				$values[$field] = $row[$field];
			}

			$values['at_risk'] = db\select(PERSON_AT_RISK_TABLE)
				->condition('person_id', $person_id)
				->orderBy('effective', 'DESC')
				->range(0, 1)
				->fields(PERSON_AT_RISK_TABLE, array('at_risk'))
				->execute()
				->fetchField();

			//Get the financial situation set
			$adapter = new FinancialSituationFormAdapter($person_id);
			$values['financial_situation'] = parent::load_one_to_many($adapter);

			//Get the other health condition set
			$adapter = new HealthConditionFormAdapter($person_id);
			$values['health_condition'] = parent::load_one_to_many($adapter);
		}

		private function add(&$form_state)
		{
			$values = &$form_state['values'];

			//Copy the directly-mapped fields first.
			$fields = drupal_map_assoc(
				array('living_pattern', 'movement_ability'),
				function($key) use (&$values) {
					return $values[$key];
				});

			//Insert the person ID
			$fields['person_id'] = $this->get_parent()->get_id();
			
			//And the timestamps
			$fields['user_modified'] = $fields['user_created'] =
				$GLOBALS['user']->uid;
			$fields['date_created'] = null;

			db\insert(PERSON_TABLE)
				->fields($fields)
				->execute();

			$now = new \DateTime();
			db\insert(PERSON_AT_RISK_TABLE)
				->fields(array(
					'person_id' => $this->get_parent()->get_id(),
					'at_risk' => $values['at_risk'],
					'effective' => $now->format(\CVWO\Base\DATE_FORMAT_DATABASE)))
				->execute();

			parent::save_one_to_many(array_filter(
				$form_state['values']['financial_situation']),
				new FinancialSituationFormAdapter($this->get_parent()->get_id()));
			parent::save_one_to_many(array_filter(
				$form_state['values']['health_condition']),
				new HealthConditionFormAdapter($this->get_parent()->get_id()));
		}

		/**
		 * Updates the current user in the database.
		 *
		 * @param integer $id The ID of the person to update.
		 * @param array $form_state The Drupal form state.
		 * @throws \Exception Error during profile photo handling.
		 */
		private function update($id, &$form_state)
		{
			$values = &$form_state['values'];

			//Copy the directly-mapped fields first.
			$fields		 = drupal_map_assoc(
				array('living_pattern', 'movement_ability'),
				function($key) use (&$values) {
					return $values[$key];
				});
				
			$fields['user_modified'] = $GLOBALS['user']->uid;

			db\update(PERSON_TABLE)
				->fields($fields)
				->condition('person_id', $id)
				->execute();

			//Update the at risk status
			if (intval(db\select(PERSON_AT_RISK_TABLE)
				->condition('person_id', $id)
				->orderBy('effective', 'DESC')
				->range(0, 1)
				->fields(PERSON_AT_RISK_TABLE, array('at_risk'))
				->execute()
				->fetchField()) !== intval($values['at_risk']))
			{
				$now = new \DateTime();
				db\insert(PERSON_AT_RISK_TABLE)
					->fields(array(
						'person_id' => $id,
						'at_risk' => $values['at_risk'],
						'effective' => $now->format(\CVWO\Base\DATE_FORMAT_DATABASE)))
					->execute();
			}

			parent::save_one_to_many(array_filter(
				$form_state['values']['financial_situation']),
				new FinancialSituationFormAdapter($id));
			parent::save_one_to_many(array_filter(
				$form_state['values']['health_condition']),
				new HealthConditionFormAdapter($id));
		}
	}

	class FinancialSituationFormAdapter implements \CVWO\Base\FormMultipleInputAdapter
	{
		/**
		 * The person to look up financial situation information for.
		 * 
		 * @var integer
		 */
		private $person_id;

		public function __construct($person_id)
		{
			$this->person_id = $person_id;
		}

		public function get_current_entry_ids()
		{
			$query = db\select(PERSON_FINANCIAL_SITUATION_TABLE)
				->condition('person_id', $this->person_id)
				->fields(PERSON_FINANCIAL_SITUATION_TABLE,
					array('person_financial_situation_id'))
				->execute();

			return $query->fetchCol(0);
		}

		public function get_first_entry(&$handle)
		{
			$query = db\select(PERSON_FINANCIAL_SITUATION_TABLE)
				->condition('person_id', $this->person_id)
				->fields(PERSON_FINANCIAL_SITUATION_TABLE,
					array('status'))
				->execute();

			$handle = $query;
			$row = $query->fetchAssoc();
			if ($row === false)
			{
				return false;
			}
			
			return $row['status'];
		}

		public function get_next_entry(&$handle)
		{
			$row = $handle->fetchAssoc();
			if ($row === false)
			{
				return false;
			}

			return $row['status'];
		}

		public function insert($value)
		{
			$entry = array(
				'person_id'	 => $this->person_id,
				'status'	 => $value
			);
			
			db\insert(PERSON_FINANCIAL_SITUATION_TABLE)
				->fields($entry)
				->execute();
		}

		public function update($id, $value)
		{
			$entry = array(
				'person_id'	 => $this->person_id,
				'status'	 => $value
			);
				
			db\update(PERSON_FINANCIAL_SITUATION_TABLE)
				->fields($entry)
				->condition('person_financial_situation_id', $id)
				->execute();
		}

		public function delete($id)
		{
			db\delete(PERSON_FINANCIAL_SITUATION_TABLE)
				->condition('person_financial_situation_id', $id)
				->execute();
		}
	}

	class HealthConditionFormAdapter implements \CVWO\Base\FormMultipleInputAdapter
	{
		/**
		 * The person to look up health condition information for.
		 *
		 * @var integer
		 */
		private $person_id;

		public function __construct($person_id)
		{
			$this->person_id = $person_id;
		}

		public function get_current_entry_ids()
		{
			$query = db\select(PERSON_OTHER_HEALTH_CONDITION_TABLE)
				->condition('person_id', $this->person_id)
				->fields(PERSON_OTHER_HEALTH_CONDITION_TABLE,
					array('person_health_condition_id'))
				->execute();

			return $query->fetchCol(0);
		}

		public function get_first_entry(&$handle)
		{
			$query = db\select(PERSON_OTHER_HEALTH_CONDITION_TABLE)
				->condition('person_id', $this->person_id)
				->fields(PERSON_OTHER_HEALTH_CONDITION_TABLE,
					array('status'))
				->execute();

			$handle = $query;
			$row = $query->fetchAssoc();
			if ($row === false)
			{
				return false;
			}

			return $row['status'];
		}

		public function get_next_entry(&$handle)
		{
			$row = $handle->fetchAssoc();
			if ($row === false)
			{
				return false;
			}

			return $row['status'];
		}

		public function insert($value)
		{
			$entry = array(
				'person_id'	 => $this->person_id,
				'status'	 => $value
			);

			db\insert(PERSON_OTHER_HEALTH_CONDITION_TABLE)
				->fields($entry)
				->execute();
		}

		public function update($id, $value)
		{
			$entry = array(
				'person_id'	 => $this->person_id,
				'status'	 => $value
			);

			db\update(PERSON_OTHER_HEALTH_CONDITION_TABLE)
				->fields($entry)
				->condition('person_health_condition_id', $id)
				->execute();
		}

		public function delete($id)
		{
			db\delete(PERSON_OTHER_HEALTH_CONDITION_TABLE)
				->condition('person_health_condition_id', $id)
				->execute();
		}
	}

	/**
	 * Class extension for the Abstract CVWO Person. No new behaviours here.
	 */
	class Person extends \CVWO\Base\Person
	{
		/**
		 * Creates a new Community person from their database ID.
		 *
		 * @param integer $person_id
		 */
		public static function from_id($person_id)
		{
			if (!self::exists($person_id))
			{
				throw new \InvalidArgumentException('The person ID provided is '
					. 'an invalid Community Person ID');
			}

			return new Person($person_id);
		}

		/**
		 * Checks if the person with the given ID exists.
		 *
		 * @param integer $person_id
		 * @return boolean
		 */
		public static function exists($person_id)
		{
			$count = intval(db\select(PERSON_TABLE)
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
			db\update(PERSON_TABLE)
				->fields(array('is_deleted' => 1, 'user_modified' => $GLOBALS['user']->uid))
				->condition('person_id', $this->person_id)
				->execute();

			//Check that the person being deleted is not also a worker, so that if
			//he is, we won't delete the base person.
			if (intval(db\select(WORKER_TABLE)
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
}
