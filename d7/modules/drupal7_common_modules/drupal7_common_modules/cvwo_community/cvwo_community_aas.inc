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

namespace CVWO\Community
{
	require_once drupal_get_path('module', MODULE_NAME) . '/cvwo_community_person.inc';
	require_once drupal_get_path('module', MODULE_NAME) . '/cvwo_community_worker.inc';
	require_once drupal_get_path('module', MODULE_NAME) . '/cvwo_community_aas_person.inc';

	use \CVWO\Base\Database as db;

	abstract class AASReportEditForm extends \CVWO\Base\Form
	{
		/**
		 * Stores the report ID to show. If we're adding, this would be null.
		 *
		 * @var integer
		 */
		private $report_id;

		public function __construct($report_id = null)
		{
			parent::__construct();

			$this->report_id = $report_id;
		}
		
		protected function do_build(&$form_state)
		{
			$form = array(
				'#attached' => array(
					'js' => array(
						'//cdnjs.cloudflare.com/ajax/libs/fancybox/2.1.4/jquery.fancybox.pack.min.js',
						drupal_get_path('module', MODULE_NAME) . '/cvwo_community.js',
						drupal_get_path('module', MODULE_NAME) . '/cvwo_community_aas.js',
					),
					'css' => array(
						'//cdnjs.cloudflare.com/ajax/libs/fancybox/2.1.4/jquery.fancybox.min.css' => array(
							'type' => 'external'
						),
						drupal_get_path('module', MODULE_NAME) . '/cvwo_community_aas.css',
					)
				),
				'#attributes' => array(
					'class' => array(
						namespace\MODULE_NAME . '_aas_report_edit_form',
					)
				)
			);
			$values = &$form_state['values'];

			if (empty($form_state['input']) && $this->report_id !== null)
			{
				$this->load($values);

				//Retrieve the person's name, contact information, address, and
				//attendant name.
				$person = Person::from_id($values['person_id']);
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
						'client_fieldset',
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
				'#default_value' => $values['person_id'],
				'#required' => true
			);
			
			$form['client']['search_by_address'] = array(
				'#type' => 'button',
				'#value' => t('Search by Address'),
				'#attributes' => array(
					'class' => array(
						'search_by_address'
					)
				)
			);

			$form['client']['address'] = array(
				'#type' => 'textarea',
				'#id' => 'address_display',
				'#title' => t('Place of Activation'),
				'#resizable' => false,
				'#rows' => 4,
				'#disabled' => true,
				'#default_value' => $values['address']
			);

			$form['client']['address_id'] = array(
				'#type' => 'hidden',
				'#attributes' => array(
					'id' => 'address_id'
				),
				'#default_value' => $values['address_id']
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

			$form['incident'] = array(
				'#type' => 'fieldset',
				'#title' => t('Activation Details'),
				'#collapsible' => true,
				'#attributes' => array(
					'class' => array(
						'incident_fieldset',
					),
				)
			);

			$form['incident']['device'] = array(
				'#type' => 'cvwocodem2_select',
				'#title' => t('Device'),
				'#module' => MODULE_NAME,
				'#question' => AAS_ITEM_QUESTION,
				'#default_value' => $values['device']
			);

			$form['incident']['activation_timestamp'] = array(
				'#type' => 'date_popup',
				'#title' => t('Date/time of Activation'),
				'#date_format' => \CVWO\Base\DATE_FORMAT_DATETIME,
				'#default_value' => $values['activation_timestamp'],
				'#required' => true
			);

			$form['incident']['emergency'] = array(
				'#type' => 'checkbox',
				'#title' => t('Emergency'),
				'#default_value' => $values['emergency']
			);

			$form['incident']['after_office_hours'] = array(
				'#type' => 'checkbox',
				'#title' => t('After office hours'),
				'#default_value' => $values['after_office_hours']
			);

			$form['client']['worker_id'] = $this->get_worker_field();
			$form['client']['worker_id']['#default_value'] = $values['worker_id'];

			$form['incident']['details'] = array(
				'#type' => 'textarea',
				'#title' => t('Details'),
				'#default_value' => $values['details']
			);

			$form['follow_up'] = array(
				'#type' => 'fieldset',
				'#title' => t('Follow-up')
			);

			$form['follow_up']['follow_up'] = array(
				'#type' => 'textarea',
				'#default_value' => $values['follow_up']
			);

			$form['submit'] = array(
				'#type' => 'submit',
				'#value' => t('Save')
			);

			$this->entries = array_merge_recursive($this->entries, $form);
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
		 * The do_validate function in subclass of AASReportEditForm need to
		 * implement the validation for $form_state['values']['worker_id'] field
		 * 
		 * @param array $form
		 * @param array $form_state
		 */
		protected function do_validate(&$form, &$form_state)
		{
			$values = &$form_state['values'];
			
			//Make sure that we have valid person IDs.
			if (!Person::exists($values['person_id']))
			{
				form_set_error('name', t('The person selected does not '
					. 'exist, or is not a Community Person.'));
			}

			//Selected date should be no larger than current date
			if (empty($values['activation_timestamp']) ||
				is_array($values['activation_timestamp']) ||
				new \DateTime($values['activation_timestamp']) > new \DateTime())
			{
				form_set_error('activation_timestamp',
					t('The date of activation has to be before now.'));
			}

			parent::do_validate($form, $form_state);
		}

		protected function do_process(&$form_state)
		{
			$values = &$form_state['values'];
			$activation_timestamp = new \DateTime($values['activation_timestamp']);

			if (empty($this->report_id))
			{
				$transaction = db\transaction();

				$this->add($form_state);

				$transaction->commit(AAS_REPORT_ADDED_MESSAGE, 
					AAS_REPORT_TAG, MODULE_NAME,  array(
						'@person' => Person::from_id($values['person_id'])->get_name(),
						'@time' => $activation_timestamp->format(
							\CVWO\Base\DATE_FORMAT_DATETIME)));
				drupal_set_message(t('AAS report added successfully!'));
			}
			else
			{
				$transaction = db\transaction();

				$this->update($this->report_id, $form_state);

				$transaction->commit(AAS_REPORT_UPDATED_MESSAGE, 
					AAS_REPORT_TAG, MODULE_NAME, array(
						'@person' => Person::from_id($values['person_id'])->get_name(),
						'@time' => $activation_timestamp->format(
							\CVWO\Base\DATE_FORMAT_DATETIME)));
				drupal_set_message(t('AAS report updated successfully!'));
			}
		}

		private function load(&$values)
		{
			if ($this->report_id === null)
			{
				return;
			}

			$query = db\select(AAS_REPORTS_TABLE)
				->fields(AAS_REPORTS_TABLE, array('person_id', 'address_id',
					'worker_id', 'device', 'activation_timestamp', 'emergency',
					'after_office_hours', 'details', 'follow_up'))
				->condition('aas_report_id', $this->report_id)
				->execute();

			$values = $query->fetchAssoc();
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
			$values = &$form_state['values'];

			$fields = array('person_id', 'address_id', 'worker_id',
				'device', 'activation_timestamp', 'emergency',
				'after_office_hours', 'details', 'follow_up');
			$fields = drupal_map_assoc($fields, function($field) use (&$values)
			{
				return $values[$field];
			});
			//Set to null if we do not have any device
			$nullable_field = function(&$value)
			{
				if (empty($value))
				{
					$value = null;
				}
			};
			$nullable_field($fields['device']);
			
			$user_id = $GLOBALS['user']->uid;
			$fields['user_modified'] = $fields['user_created'] = $user_id;
			$fields['date_created'] = null;

			$this->report_id = db\insert(AAS_REPORTS_TABLE)
				->fields($fields)
				->execute();
		}

		/**
		 * Updates the current report in the database.
		 *
		 * @param integer $id The ID of the report to update.
		 * @param array $form_state The Drupal form state.
		 * @throws \Exception Error during profile photo handling.
		 */
		private function update($id, &$form_state)
		{
			$values = $form_state['values'];

			//Copy the directly-mapped fields first.
			$fields = array('person_id', 'address_id', 'device', 'worker_id',
				'activation_timestamp', 'emergency', 'after_office_hours',
				'details', 'follow_up');
			$fields = drupal_map_assoc($fields, function($key) use (&$values)
				{
					return $values[$key];
				});

			//Set to null if we do not have any device
			$nullable_field = function(&$value)
			{
				if (empty($value))
				{
					$value = null;
				}
			};
			$nullable_field($fields['device']);
			
			$fields['user_modified'] = $GLOBALS['user']->uid;

			db\update(AAS_REPORTS_TABLE)
				->fields($fields)
				->condition('aas_report_id', $id)
				->execute();
		}
		
		/**
		 * Gets the ID of the report we are editing.
		 * 
		 * @return integer
		 */
		public function get_report_id()
		{
			return $this->report_id;
		}
	}

	/**
	 * Mark deleted the cvwo_community_aas_report according to the id indicated
	 * 
	 * @global type $user
	 * @param integer $report_id: aas report primary key: aas_report_id
	 * @return boolean True if the report was deleted successfully.
	 */
	function delete_community_aas_report($report_id)
	{
		global $user;
		$current_user_id = $user->uid;
		
		$transaction = db\transaction();

		$affected_rows = db\update(AAS_REPORTS_TABLE)
			->fields(array(
				'is_deleted' => 1,
				'user_modified' => $current_user_id))
			->condition('aas_report_id', $report_id)
			->execute();
		
		if ($affected_rows === 1)
		{
			$now = new \DateObject();
			$transaction->commit(AAS_REPORT_DELETED_MESSAGE,  AAS_REPORT_TAG,
				MODULE_NAME,  array(
					'@person' => 'name',
					'@time' => $now->format(\CVWO\Base\DATE_FORMAT_DATETIME)));
			drupal_set_message(t('AAS report deleted successfully!'));
			return true;
		}
		else
		{
			drupal_set_message(t('AAS report does not exist! Deletion failed'), 'error'); 
			return false;
		}
	}
}
