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
	require_once drupal_get_path('module', MODULE_NAME) . '/cvwo_community_person.inc';
	require_once drupal_get_path('module', MODULE_NAME) . '/cvwo_community_worker.inc';

	use \CVWO\Base\Database as db;

	abstract class HomeVisitEditForm extends \CVWO\Base\Form
	{
		/**
		 * Stores the home_visit ID to show. If we're adding, this would be null.
		 *
		 * @var integer
		 */
		private $home_visit_report_id;
		
		/**
		 * The Workers List
		 * 
		 * @var HomeVisitWorkersForm
		 */
		private $workers_list_form;

		public function __construct($home_visit_report_id = null)
		{
			parent::__construct();

			$this->home_visit_report_id = $home_visit_report_id;

			//Check if we are editing a home visit report. If we are, and the report
			//does not exist, throw an error.
			if (!empty($this->home_visit_report_id))
			{
				$query = db\select(HOME_VISIT_REPORT_TABLE)
					->condition('home_visit_report_id', $this->home_visit_report_id)
					->countQuery()
					->execute();
				if (intval($query->fetchField()) !== 1)
				{
					throw new \InvalidArgumentException(t('The report selected'
						. 'for editing does not exist.'));
				}
			}
		}

		protected function do_build(&$form_state)
		{
			$form = array(
				'#attached' => array(
					'js' => array(
						drupal_get_path('module', MODULE_NAME) . '/cvwo_community.js',
					),
					'css' => array(
						drupal_get_path('module', MODULE_NAME) . '/cvwo_community_home_visit.css',
					)
				),
				'#attributes' => array(
					'class' => array(
						namespace\MODULE_NAME . '_home_visit_edit_form',
					)
				)
			);

			$values = &$form_state['values'];
			if (empty($form_state['input']) && $this->home_visit_report_id !== null)
			{
				$this->load($values);
			}

			//Retrieve the person's name, contact information, address, 
			//attendant name and organisation name
			if (!empty($values['person_id']))
			{
				$person = Person::from_id($values['person_id']);

				// use the address_id to get address
				$values['address'] = $this->load_address_display_text($values['address_id']);
				$values['contact'] = $person->get_contacts();
				foreach ($values['contact'] as $key => &$value)
				{
					$value = array(
						$value['contact_type_display'],
						$value['contact_value']
					);
				}
			}

			$form['client'] = array(
				'#type' => 'fieldset',
				'#title' => t('Client Information'),
				'#collapsible' => true,
				'#attributes' => array(
					'class' => array(
						'client_fieldset'
					)
				)
			);

			//We use hook this to community person because at this level, we only
			//distinguish between a community person and a worker.
			$form['client']['person_id'] = array(
				'#type' => 'select_person',
				'#title' => t('Client'),
				'#autocomplete_path' => MODULE_NAME . '/person/search',
				'#class' => '\CVWO\Community\Person',
				'#default_value' => isset($values['person_id']) ?
					$values['person_id'] : null,
				'#required' => true
			);

			$form['client']['address'] = array(
				'#type' => 'textarea',
				'#id' => 'address_display',
				'#title' => t('Client\'s Address'),
				'#resizable' => false,
				'#rows' => 4,
				'#disabled' => true,
				'#default_value' => isset($values['address']) ?
					$values['address'] : null
			);

			$form['client']['address_id'] = array(
				'#type' => 'hidden',
				'#attributes' => array(
					'id' => 'address_id'
				),
				'#default_value' => isset($values['address_id']) ?
					$values['address_id'] : null
			);

			$form['client']['contact'] = array(
				'#theme' => 'table',
				'#attributes' => array(
					'id' => 'contact_display'
				),
				'#rows' => empty($values['contact']) ?
					array(array('&nbsp;', '')) : $values['contact'],
				'#header' => array(t('Contact Type'), t('Contact Value'))
			);


			$form['client']['worker_id'] = $this->get_worker_field();
			$form['client']['worker_id']['#default_value'] =
				isset($values['worker_id']) ? $values['worker_id'] : null;

			$form['home_visit'] = array(
				'#type' => 'fieldset',
				'#title' => t('Home Visit Details'),
				'#collapsible' => true,
				'#attributes' => array(
					'class' => array(
						'home_visit_fieldset',
					)
				)
			);

			$form['home_visit']['date_visited'] = array(
				'#type' => 'date_popup',
				'#title' => t('Date/time of Visit'),
				'#date_format' => \CVWO\Base\DATE_FORMAT_DATETIME,
				'#required' => true,
				'#default_value' => isset($values['date_visited']) ?
					$values['date_visited'] : null
			);

			$form['home_visit']['requires_attention'] = array(
				'#type' => 'cvwocodem2_select',
				'#title' => t('Requires Attention'),
				'#module' => MODULE_NAME,
				'#question' => HOME_VISIT_REQUIRES_ATTENTION_QUESTION,
				'#default_value' => isset($values['requires_attention']) ?
					$values['requires_attention'] : null,
				'#required' => true
			);

			$form['home_visit']['is_successful'] = array(
				'#type' => 'checkbox',
				'#title' => t('Successful Visit'),
				'#default_value' => isset($values['is_successful']) ?
					$values['is_successful'] : null
			);

			$home_cond_options = cvwocodem2_getoptions2(MODULE_NAME,
				HOME_VISIT_HOME_CONDITION_QUESTION, $enabled_only = 0, false,
				null);
			$form['home_visit']['home_condition'] = array(
				'#type' => 'checkboxes',
				'#title' => t('Home Conditions'),
				'#options' => $home_cond_options,
				'#default_value' => ($this->home_visit_report_id ?
					$values['home_condition'] : array()),
			);

			$form['home_visit']['items_brought'] = array(
				'#type' => 'textarea',
				'#title' => t('Items Brought'),
				'#default_value' => isset($values['items_brought']) ?
					$values['items_brought'] : null
			);

			$form['home_visit']['remarks'] = array(
				'#type' => 'textarea',
				'#title' => t('Remarks'),
				'#default_value' => isset($values['remarks']) ?
					$values['remarks'] : null
			);

			$this->workers_list_form = $this->get_worker_list_field();
			$this->add_child($this->workers_list_form);
			$form['workers'] = array_merge(
				array(
					'#type' => 'fieldset',
					'#title' => t('Volunteer Workers'),
					'#collapsible' => true,
					'#attributes' => array(
						'class' => array(
							'workers_fieldset',
						),
					),
				),
				$this->workers_list_form->build($form_state)
			);

			$form['submit'] = array(
				'#type' => 'submit',
				'#value' => t('Save')
			);

			$this->entries = array_merge($this->entries, $form);
		}

		/**
		 * This get_worker_field function is called by do_build to generate
		 * a Person Selection field. This can be any form field, so long a
		 * Worker ID is submitted as $form_state['values']['worker_id'].
		 * 
		 * The default value will be filled in for you.
		 */
		protected abstract function get_worker_field();
		
		/**
		 * This get_worker_list_field function is called by do_build to generate
		 * a worker list sub form. The return value should be a subclass of
		 * HomeVisitWorkersForm.
		 */
		protected abstract function get_worker_list_field();

		protected function do_process(&$form_state)
		{
			$values = $form_state['values'];
			$now = new \DateObject();

			if (empty($this->home_visit_report_id))
			{
				$transaction = db\transaction();

				$this->add($form_state);

				//Then handle our subforms: workers form
				$this->process_children($form_state);

				$transaction->commit(HOME_VISIT_ADDED_MESSAGE, HOME_VISIT_TAG,
					MODULE_NAME, array(
						'@person' => 'name',
						'@time' => $now->format(\CVWO\Base\DATE_FORMAT_DATETIME)
					));

				drupal_set_message(t('Home Visit report added successfully!'));
			}
			else
			{
				$transaction = db\transaction();

				$this->update($this->home_visit_report_id, $form_state);

				//Then handle our subforms.
				$this->process_children($form_state);

				$transaction->commit(HOME_VISIT_UPDATED_MESSAGE, HOME_VISIT_TAG,
					MODULE_NAME, array(
						'@person' => 'name',
						'@time' => $now->format(\CVWO\Base\DATE_FORMAT_DATETIME)
					));

				drupal_set_message(t('Home Visit report updated successfully!'));
			}
		}

		/**
		 * The do_validate function in subclass of HomeVisitEditForm need to
		 * implement the validation for $form_state['values']['worker_id'] field
		 * 
		 * @param type $form
		 * @param type $form_state
		 */
		protected function do_validate(&$form, &$form_state)
		{
			$values = &$form_state['values'];

			//Make sure that we have valid person IDs.
			if (!Person::exists($values['person_id']))
			{
				form_set_error('name', t('The client selected does not exist, ' .
					'or is not a client.'));
			}

			//Selected date should be before now.
			if (empty($values['date_visited']) || is_array($values['date_visited']) ||
				new \DateTime($values['date_visited']) > new \DateTime())
			{
				form_set_error('date_visited', t('The date of visited has to ' .
					'be before the present.'));
			}

			parent::do_validate($form, $form_state);
		}

		/**
		 * Sets the ID of this form object
		 * 
		 * @param integer $home_visit_report_id The database ID of the home_visit_report.
		 */
		public function set_id($home_visit_report_id)
		{
			if (empty($home_visit_report_id))
			{
				throw new \InvalidArgumentException('home_visit_report_id');
			}

			$this->home_visit_report_id = $home_visit_report_id;
		}

		/**
		 * Gets the ID of this form object. If this returns null, the object
		 * does not yet exist in the database.
		 *
		 * @return int
		 */
		public function get_id()
		{
			return $this->home_visit_report_id;
		}

		private function load(&$values)
		{
			if (empty($this->home_visit_report_id))
			{
				return;
			}

			$query = db\select(HOME_VISIT_REPORT_TABLE, 'home_visit')
				->fields('home_visit', array('person_id', 'address_id', 'worker_id',
					'date_visited', 'items_brought', 'remarks',
					'requires_attention', 'is_successful'))
				->condition('home_visit_report_id', $this->home_visit_report_id)
				->execute();

			$values = $query->fetchAssoc();

			// load home conditions using form adapter
			$hc_adapter = new HomeConditionFormAdapter($this->home_visit_report_id);
			$values['home_condition'] = parent::load_one_to_many($hc_adapter);
		}

		private function load_address_display_text($address_id)
		{
			// we are not checking "is_deleted" condition for address
			// so that even if the address is soft deleted, 
			// the address_id in the report is still valid
			$query = db\select(CVWO_ADDRESS_TABLE, 'a')
				->fields('a')
				->condition('address_id', $address_id)
				->execute();
			$rows = $query->fetchAssoc();
			return Person::format_address($rows);
		}

		private function add(&$form_state)
		{
			global $user;
			$current_user_id = $user->uid;

			$values = &$form_state['values'];

			$fields_in_forms = array('person_id', 'address_id', 'worker_id',
				'date_visited', 'items_brought', 'remarks', 'requires_attention',
				'is_successful');
			$fields_in_tables = array('person_id', 'address_id', 'worker_id',
				'date_visited', 'items_brought', 'remarks', 'requires_attention',
				'is_successful', 'user_created', 'date_created', 'user_modified');

			$db_values = array();
			foreach ($fields_in_forms as $field)
			{
				$db_values[] = &$values[$field];
			}

			$db_values[] = $current_user_id;
			$db_values[] = null;
			$db_values[] = $current_user_id;

			$last_id = db\insert(HOME_VISIT_REPORT_TABLE)
				->fields($fields_in_tables, $db_values)
				->execute();

			$this->set_id($last_id);
			parent::save_one_to_many(array_filter($values['home_condition']), new HomeConditionFormAdapter($this->get_id()));
		}

		/**
		 * Updates the current home visit in the database.
		 *
		 * @param integer $id The ID of the home visit to update.
		 * @param array $form_state The Drupal form state.
		 * @throws \Exception Error during profile photo handling.
		 */
		private function update($id, &$form_state)
		{
			$values = $form_state['values'];

			global $user;
			$current_user_id = $user->uid;

			//Copy the directly-mapped fields first.
			$fields = array('person_id', 'address_id', 'worker_id',
				'date_visited', 'items_brought', 'remarks',
				'requires_attention', 'is_successful',);
			$fields_value = drupal_map_assoc($fields, function($key) use (&$values)
				{
					return $values[$key];
				});

			$fields_value['user_modified'] = $current_user_id;

			db\update(HOME_VISIT_REPORT_TABLE)
				->fields($fields_value)
				->condition('home_visit_report_id', $id)
				->execute();

			// update home_condition using form adapter framework
			parent::save_one_to_many(array_filter($values['home_condition']), new HomeConditionFormAdapter($id));
		}
	}

	class HomeConditionFormAdapter implements \CVWO\Base\FormMultipleInputAdapter
	{
		private $home_visit_report_id;

		public function __construct($home_visit_report_id)
		{
			$this->home_visit_report_id = $home_visit_report_id;
		}

		public function get_current_entry_ids()
		{
			$query = db\select(HOME_VISIT_REPORT_HOME_CONDITION_TABLE)
				->condition('home_visit_report_id', $this->home_visit_report_id)
				->fields(HOME_VISIT_REPORT_HOME_CONDITION_TABLE, array('home_visit_report_home_condition_id'))
				->execute();

			return $query->fetchCol(0);
		}

		public function get_first_entry(&$handle)
		{
			$query = db\select(HOME_VISIT_REPORT_HOME_CONDITION_TABLE)
				->condition('home_visit_report_id', $this->home_visit_report_id)
				->fields(HOME_VISIT_REPORT_HOME_CONDITION_TABLE, array('home_condition'))
				->execute();

			$handle = $query;
			$row = $query->fetchAssoc();
			if ($row === false)
			{
				return false;
			}

			return $row['home_condition'];
		}

		public function get_next_entry(&$handle)
		{
			$row = $handle->fetchAssoc();
			if ($row === false)
			{
				return false;
			}

			return $row['home_condition'];
		}

		public function insert($value)
		{
			global $user;
			$current_user_id = $user->uid;
			$entry = array(
				'home_visit_report_id' => $this->home_visit_report_id,
				'home_condition' => $value,
				'user_created' => $current_user_id,
				'user_modified' => $current_user_id
			);

			db\insert(HOME_VISIT_REPORT_HOME_CONDITION_TABLE)
				->fields($entry)
				->execute();
		}

		public function update($id, $value)
		{
			$entry = array(
				'home_visit_report_id' => $this->home_visit_report_id,
				'home_condition' => $value,
			);

			db\update(HOME_VISIT_REPORT_HOME_CONDITION_TABLE)
				->fields($entry)
				->condition('home_visit_report_home_condition_id', $id)
				->execute();
		}

		public function delete($id)
		{
			db\delete(HOME_VISIT_REPORT_HOME_CONDITION_TABLE)
				->condition('home_visit_report_home_condition_id', $id)
				->execute();
		}

	}
	
	abstract class HomeVisitWorkersForm extends \CVWO\Base\FormMultipleInput
	                                    implements \CVWO\Base\FormMultipleInputFormAdapter
	{
		protected function do_build(&$form_state)
		{
			$form = array();

			//Load the records from database
			if (empty($form_state['input']) && $this->get_parent()->get_id() !== null)
			{
				$this->load($form_state, $this);
			}

			$form['volunteer_worker_id'] = $this->get_worker_field();

			$this->entries = array_merge($this->entries, $form);
			parent::do_build($form_state);
		}
		
		/**
		 * This get_worker_field function is called by do_build to generate
		 * a Person Selection field. This can be any form field, so long a
		 * Worker ID is submitted as $form_state['values']['volunteer_worker_id'].
		 * 
		 * The default value will be filled in for you.
		 */
		protected abstract function get_worker_field();

		protected function do_validate(&$form, &$form_state)
		{
			parent::do_validate($form, $form_state, $this);

			// validation1: empty entries should not be inserted into DB
			$worker_ids = $form_state['values']['volunteer_worker_id'];
			foreach ($worker_ids as $value)
			{
				if (empty($value))
				{
					form_set_error('workers', t('Please remove empty volunteer worker entries'));
					return;
				}
			}

			// validation2: no duplicate entries
			if (count(array_unique($worker_ids)) !== count($worker_ids))
			{
				form_set_error('workers', t('Please remove duplicate volunteer worker entries'));
			}

			// validation3: check whether the volunteer exists
		}

		protected function do_process(&$form_state)
		{
			$this->save($form_state, $this);
		}

		public function get_fields_mapping()
		{
			return array('worker_id' => 'volunteer_worker_id');
		}

		public function get_current_entry_ids()
		{
			$id = $this->get_parent()->get_id();
			$query = db\select(HOME_VISIT_REPORT_WORKER_TABLE, 'hv_workers')
				->condition('home_visit_report_id', $id)
				->condition('is_deleted', 1, '<>');
			$query->addField('hv_workers', 'home_visit_report_worker_id');
			$query = $query->execute();

			return $query->fetchCol(0);
		}

		public function get_first_entry(&$handle)
		{
			$id = $this->get_parent()->get_id();
			if (empty($id))
			{
				return false;
			}

			$query = db\select(HOME_VISIT_REPORT_WORKER_TABLE)
				->condition('home_visit_report_id', $id)
				->condition('is_deleted', 1, '<>')
				->fields(HOME_VISIT_REPORT_WORKER_TABLE,
					array_keys($this->get_fields_mapping()))
				->execute();
			$handle = $query;

			return $handle->fetchAssoc();
		}

		public function get_next_entry(&$handle)
		{
			return $handle->fetchAssoc();
		}

		public function insert($entry)
		{
			$id = $this->get_parent()->get_id();
			$current_user_id = $GLOBALS['user']->uid;

			$entry['home_visit_report_id'] = $id;
			$entry['user_created'] = $current_user_id;
			$entry['user_modified'] = $current_user_id;
			$entry['date_created'] = null;

			db\insert(HOME_VISIT_REPORT_WORKER_TABLE)
				->fields($entry)
				->execute();
		}

		public function update($id, $entry)
		{
			$current_user_id = $GLOBALS['user']->uid;
			$entry['user_modified'] = $current_user_id;

			db\update(HOME_VISIT_REPORT_WORKER_TABLE)
				->condition('home_visit_report_worker_id', $id)
				->fields($entry)
				->execute();
		}

		public function delete($id)
		{
			$current_user_id = $GLOBALS['user']->uid;
			db\update(HOME_VISIT_REPORT_WORKER_TABLE)
				->fields(array('is_deleted' => 1, 'user_modified' => $current_user_id))
				->condition('home_visit_report_worker_id', $id)
				->execute();
		}
	}
	
	function delete_community_home_visit_report($report_id)
	{
		global $user;
		$current_user_id = $user->uid;

		$transaction = db\transaction();

		$delete_fields = array('is_deleted' => 1,
			'user_modified' => $current_user_id);

		$affected_rows = db\update(HOME_VISIT_REPORT_TABLE)
			->fields($delete_fields)
			->condition('home_visit_report_id', $report_id)
			->execute();

		// delete from home cond table
		db\update(HOME_VISIT_REPORT_HOME_CONDITION_TABLE)
			->fields($delete_fields)
			->condition('home_visit_report_id', $report_id)
			->execute();

		if ($affected_rows === 1)
		{
			$now = new \DateObject();
			$transaction->commit(HOME_VISIT_DELETED_MESSAGE, HOME_VISIT_TAG,
				MODULE_NAME, array(
					'@person' => 'name',
					'@time' => $now->format(\CVWO\Base\DATE_FORMAT_DATETIME)));

			drupal_set_message(t('Home Visit report deleted successfully!'));
		}
		else
		{
			drupal_set_message(t('Home Visit report does not exist! Deletion failed'),
				'error');
			return false;
		}
	}
}
