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

	abstract class InfoRefEditForm extends \CVWO\Base\Form
	{
		/**
		 * Stores the info_ref ID to show. If we're adding, this would be null.
		 *
		 * @var integer
		 */
		private $info_ref_report_id;

		public function __construct($info_ref_report_id = null)
		{
			parent::__construct();

			$this->info_ref_report_id = $info_ref_report_id;
		}

		protected function do_build(&$form_state)
		{
			$form = array(
				'#attached' => array(
					'js' => array(
						drupal_get_path('module', MODULE_NAME) . '/cvwo_community.js',
					),
					'css' => array(
						drupal_get_path('module', MODULE_NAME) . '/cvwo_community_info_ref.css',
					)
				),
				'#attributes' => array(
					'class' => array(
						namespace\MODULE_NAME . '_info_ref_edit_form',
					)
				)
			);

			$values = &$form_state['values'];

			if (empty($form_state['input']) && $this->info_ref_report_id !== null)
			{
				$this->load($values);

				//Retrieve the person's name, contact information, address, 
				//attendant name and organisation name
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
					),
				),
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

			$form['client']['address'] = array(
				'#type' => 'textarea',
				'#id' => 'address_display',
				'#title' => t('Client\'s Address'),
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


			$form['client']['worker_id'] = $this->get_worker_field();
			$form['client']['worker_id']['#default_value'] = $values['worker_id'];

			$form['info_referral'] = array(
				'#type' => 'fieldset',
				'#title' => t('Info/Referral Details'),
				'#collapsible' => true,
				'#attributes' => array(
					'class' => array(
						'info_referral_fieldset',
					),
				),
			);

			$form['info_referral']['date_requested'] = array(
				'#type' => 'date_popup',
				'#title' => t('Date/time of Referral'),
				'#date_format' => \CVWO\Base\DATE_FORMAT_DATETIME,
				'#required' => true,
				'#default_value' => $values['date_requested']
			);

			$form['info_referral']['details'] = array(
				'#type' => 'textarea',
				'#title' => t('Details'),
				'#default_value' => $values['details']
			);

			$form['info_referral']['action_taken'] = array(
				'#type' => 'textarea',
				'#title' => t('Action Taken'),
				'#default_value' => $values['action_taken']
			);

			$form['info_referral']['info_ref'] = array(
				'#type' => 'cvwocodem2_select',
				'#title' => t('Info or Referral'),
				'#module' => MODULE_NAME,
				'#question' => INFO_REF_INFO_OR_REF_QUESTION,
				'#default_value' => ($this->info_ref_report_id ? $values['info_ref'] : '0'),
				'#required' => true
			);

			$form['info_referral']['type'] = array(
				'#type' => 'cvwocodem2_select',
				'#title' => t('Info/Referral Type'),
				'#module' => MODULE_NAME,
				'#question' => INFO_REF_TYPE_QUESTION,
				'#required' => true,
				'#default_value' => $values['type']
			);

			$form['info_referral']['status'] = array(
				'#type' => 'cvwocodem2_select',
				'#title' => t('Info/Referral Status'),
				'#module' => MODULE_NAME,
				'#question' => INFO_REF_STATUS_QUESTION,
				'#required' => true,
				'#default_value' => ($this->info_ref_report_id ? $values['status'] : '0'),
			);

			$form['info_referral']['follow_up'] = array(
				'#type' => 'cvwocodem2_select',
				'#title' => t('Follow-up'),
				'#module' => MODULE_NAME,
				'#question' => INFO_REF_FOLLOW_UP_QUESTION,
				'#default_value' => ($this->info_ref_report_id ? $values['follow_up'] : '0'),
				'#required' => true
			);

			$form['info_referral']['organisation_id'] = array(
				'#type' => 'select_organisation',
				'#title' => t('Organisation'),
				'#default_value' => $values['organisation_id'],
				'#required'	=> true
			);

			$form['info_ref_report_id'] = array(
				'#type' => 'value',
				'#value' => $this->info_ref_report_id === null ? '' : $this->info_ref_report_id
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
		 * The do_validate function in subclass of InfoRefEditForm need to
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
				form_set_error('name', t('The person selected does not '
						. 'exist, or is not a client.'));
			}

			//The date of referral must not be after now.
			if (is_array($values['date_requested']) ||
				new \DateTime($values['date_requested']) > new \DateTime())
			{
				form_set_error('date_requested', t('The date of request has ' .
					'to be no larger than the current time'));
			}

			parent::do_validate($form, $form_state);
		}

		protected function do_process(&$form_state)
		{
			$values = $form_state['values'];
			if (empty($values['info_ref_report_id']))
			{
				$transaction = db\transaction();

				$this->add($form_state);

				$transaction->commit(INFO_REF_ADDED_MESSAGE, INFO_REF_TAG,
					MODULE_NAME, array(
						'@person' => 'name',
						'@time' => $values['date_requested']
					));

				drupal_set_message(t('Info/Referral report added successfully!'));
			}
			else
			{
				$transaction = db\transaction();

				$this->update($values['info_ref_report_id'], $form_state);

				$transaction->commit(INFO_REF_UPDATED_MESSAGE, INFO_REF_TAG,
					MODULE_NAME, array(
						'@person' => 'name',
						'@time' => $values['date_requested']
					));

				drupal_set_message(t('Info/Referral report updated successfully!'));
			}
		}

		private function load(&$values)
		{
			if ($this->info_ref_report_id === null)
			{
				return;
			}

			$query = db\select(INFO_REF_REPORT_TABLE)
				->fields(INFO_REF_REPORT_TABLE, array('person_id', 'address_id',
					'worker_id', 'organisation_id', 'date_requested', 'details',
					'action_taken', 'info_ref', 'follow_up', 'type', 'status'))
				->condition('info_ref_report_id', $this->info_ref_report_id)
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
				'date_requested', 'details', 'action_taken', 'info_ref', 'follow_up',
				'type', 'status', 'organisation_id');
			$fields = drupal_map_assoc($fields, function($field) use (&$values)
			{
				return $values[$field];
			});
			
			$nullable_field = function(&$value)
			{
				if (empty($value))
				{
					$value = null;
				}
			};
			$nullable_field($fields['organisation_id']);

			$user_id = $GLOBALS['user']->uid;
			$fields['user_modified'] = $fields['user_created'] = $user_id;
			$fields['date_created'] = null;

			$this->info_ref_report_id = db\insert(INFO_REF_REPORT_TABLE)
				->fields($fields)
				->execute();
			$form_state['values']['info_ref_report_id'] = $this->info_ref_report_id;
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
			global $user;
			$current_user_id = $user->uid;

			$values = $form_state['values'];

			$fields = array('person_id', 'address_id', 'worker_id', 'date_requested',
				'details', 'action_taken', 'info_ref', 'follow_up', 'type',
				'status', 'organisation_id');

			$fields_value = drupal_map_assoc($fields, function($key) use (&$values)
				{
					return $values[$key];
				});

			$nullable_field = function(&$value)
			{
				if (empty($value))
				{
					$value = null;
				}
			};
			$nullable_field($fields['organisation_id']);

			$fields_value['user_modified'] = $current_user_id;

			db\update(INFO_REF_REPORT_TABLE)
				->fields($fields_value)
				->condition('info_ref_report_id', $id)
				->execute();
		}
		
		public function get_id() {
			return $this->info_ref_report_id;
		}
	}

	/**
	 * soft delete the cvwo_community_info_ref_report according to the id indicated
	 * 
	 * @global type $user
	 * @param type $report_id: info_ref report primary key: info_ref_report_id
	 * @return boolean: true - deleted successfully
	 */
	function delete_community_info_ref_report($report_id)
	{
		global $user;
		$current_user_id = $user->uid;

		$transaction = db\transaction();

		$affected_rows = db\update(INFO_REF_REPORT_TABLE)
			->fields(array(
				'is_deleted' => 1,
				'user_modified' => $current_user_id))
			->condition('info_ref_report_id', $report_id)
			->execute();

		if ($affected_rows === 1)
		{
			$now = new \DateObject();
			$transaction->commit(INFO_REF_DELETED_MESSAGE, INFO_REF_TAG,
				MODULE_NAME, array(
					'@person' => 'name',
					'@time' => $now->format(\CVWO\Base\DATE_FORMAT_DATETIME)
				));

			drupal_set_message(t('Info/Referral report deleted successfully!'));
			return true;
		}
		else
		{
			drupal_set_message(t('Info/Referral report does not exist! Deletion failed'),
				'error');
			return false;
		}
	}
	
	function info_ref_delete(array $form, array &$form_state, $info_ref_report_id = null)
	{
		$form = array();
		$form['#deleted_id'] = $info_ref_report_id;
		return confirm_form($form, t('Are you sure you want to delete?'),
			'info_ref/view', t('This action cannot be undone.'), t('Delete'),
			t('Cancel'));
	}

	function info_ref_delete_submit(array $form, array &$form_state)
	{
		$report_id = $form['#deleted_id'];

		$transaction = db\transaction();

		global $user;
		$current_user_id = $user->uid;

		$count = intval(db\select(INFO_REF_REPORT_TABLE, 'info_ref')
			->condition('info_ref_report_id', $report_id)
			->countQuery()
			->execute()
			->fetchField());

		if ($count === 1)
		{
			db\update(INFO_REF_REPORT_TABLE)
				->fields(array('is_deleted' => 1, 'user_modified' => $current_user_id))
				->condition('info_ref_report_id', $report_id)
				->execute();

			$date = new \DateObject();

			$transaction->commit(INFO_REF_DELETED_MESSAGE,
				'Info/Referral Deleted', MODULE_NAME, array(
					'@person' => 'name',
					'@time' => $now->format(\CVWO\Base\DATE_FORMAT_DATETIME)
				));

			drupal_set_message(t('Info/Referral deleted successfully!'));
		}
		else
		{
			drupal_set_message(t('Info/Referral report does not exist! Deletion failed'),
				'error');
		}
	}
}
