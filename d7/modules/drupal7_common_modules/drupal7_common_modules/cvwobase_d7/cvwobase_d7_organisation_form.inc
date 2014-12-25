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

namespace CVWO\Base {

	use \CVWO\Base\Database as db;
	
	/**
	 * Build the organisation add/edit form
	 * @param type $form
	 * @param type $form_state
	 * @param type $id
	 * @return type
	 */
	function organisation_edit($form, &$form_state, $id = null) {
		try {
			$instance = new OrganisationEditForm($id);
			return $instance->build($form_state);
		} catch (\InvalidArgumentException $e) {
			drupal_not_found();
			drupal_exit();
		}
	}

	function organisation_edit_validate(&$form, &$form_state) {
		$instance = $form['#instance'];
		$instance->validate($form, $form_state);
	}

	function organisation_edit_submit($form, &$form_state) {
		$instance = $form['#instance'];
		$instance->process($form_state);
	}

	Class OrganisationEditForm extends Form {

		protected $organisation_id;

		public function __construct($organisation_id = null) {
			parent::__construct();

			$this->organisation_id = $organisation_id;

			if (!empty($this->organisation_id)) {
				$query = db\select(ORGANISATION_TABLE)
					->condition('organisation_id', $this->organisation_id)
					->countQuery()
					->execute();
				if (intval($query->fetchField()) !== 1) {
					throw new \InvalidArgumentException(t('The organisation selected'
						. 'for editing does not exist.'));
				}
			}
		}

		protected function do_build(&$form_state) {
			$form = array();

			$organisationParticularsForm = new OrganisationParticularsForm();
			$this->add_child($organisationParticularsForm);
			$form['organisation_particulars'] = array_merge(
				array(
				'#type' => 'fieldset',
				'#title' => t('Organisation Particulars'),
				'#collapsible' => true
				), $organisationParticularsForm->build($form_state));

			$organisationAddressForm = new OrganisationAddressForm();
			$this->add_child($organisationAddressForm);
			$form['organisation_addresses'] = array_merge(
				array(
				'#type' => 'fieldset',
				'#title' => t('Organisation Addresses'),
				'#collapsible' => true
				), $organisationAddressForm->build($form_state));

			//Information about the organisation we are editing.
			$form['organisation_id'] = array(
				'#type' => 'hidden',
				'#value' => $this->organisation_id,
			);

			$form['submit'] = array(
				'#type' => 'submit',
				'#value' => t('Save'),
				'#id' => 'Save',
			);

			$this->entries = array_merge_recursive($this->entries, $form);
		}

		protected function do_process(&$form_state) {
			$values = $form_state['values'];
			$now = new \DateObject();

			$transaction = db\transaction();


			//Then handle our subforms: address form
			$this->process_children($form_state);

			$transaction->commit(ORGANISATION_ADD_LOG_MESSAGE, ORGANISATION_TAG, MODULE_NAME, array(
				'@org' => 'organisation_name',
				'@time' => $now->format(\CVWO\Base\DATE_FORMAT_DATETIME)
			));
			if (empty($this->organisation_id)) {
				drupal_set_message(t('Organisation added successfully!'));
			} else {
				drupal_set_message(t('Organisation updated successfully!'));
			}
		}

		public function set_id($id) {
			if ($id === null) {
				throw new \InvalidArgumentException('$organisation_id');
			}
			$this->organisation_id = $id;
		}

		public function get_id() {
			return $this->organisation_id;
		}

	}

	class OrganisationParticularsForm extends Form {

		private $disabled;

		public function __construct($disabled = false) {
			parent::__construct();
			$this->disabled = $disabled;
		}

		protected function do_build(&$form_state) {
			$form = array();

			$values = empty($form_state['values']) ? null :
				$form_state['values'];

			if (empty($form_state['input']) && $this->get_parent()->get_id() !== null) {
				$this->load($this->get_parent()->get_id(), $values);
			}

			$form['name'] = array(
				'#type' => 'textfield',
				'#title' => t('Name'),
				'#size' => 50,
				'#required' => TRUE,
				'#default_value' => isset($values['name']) ? $values['name'] : null,
				'#disabled' => $this->disabled,
			);

			$form['type'] = array(
				'#type' => 'cvwocodem2_select',
				'#title' => t('Type'),
				'#module' => MODULE_NAME,
				'#question' => ORGANISATION_TYPE_QUESTION,
				'#default_value' => isset($values['type']) ? $values['type'] : null,
				'#required' => TRUE,
				'#disabled' => $this->disabled,
			);

			$form['industry'] = array(
				'#type' => 'cvwocodem2_autocomplete',
				'#title' => t('Industry'),
				'#module' => MODULE_NAME,
				'#question' => ORGANISATION_INDUSTRY_QUESTION,
				'#default_value' => isset($values['industry']) ? $values['industry'] : null,
				'#disabled' => $this->disabled,
			);

			$form['member_count'] = array(
				'#type' => 'textfield',
				'#title' => t('Number of Members'),
				'#size' => 50,
				'#default_value' => isset($values['member_count']) ? $values['member_count'] : null,
				'#disabled' => $this->disabled,
				'#element_validate' => array(
					'element_validate_integer_positive'
				)
			);

			$form['registration_no'] = array(
				'#type' => 'textfield',
				'#title' => t('Registration Number'),
				'#size' => 50,
				'#default_value' => isset($values['registration_no']) ? $values['registration_no'] : null,
				'#disabled' => $this->disabled,
			);

			$form['description'] = array(
				'#type' => 'textarea',
				'#title' => t('Description'),
				'#size' => 50,
				'#default_value' => isset($values['description']) ? $values['description'] : null,
				'#disabled' => $this->disabled,
			);
			$this->entries = array_merge_recursive($this->entries, $form);
		}

		protected function do_validate(&$form, &$form_state) {
			parent::do_validate($form, $form_state);
		}

		protected function do_process(&$form_state) {
			$id = $this->get_parent()->get_id();
			if (empty($id)) {
				$id = $this->add($form_state);
			} else {
				$this->update($form_state, $id);
			}
		}

		private function load($id, &$values) {
			$fields = array('name', 'member_count', 'type', 'industry',
				'registration_no', 'description');

			$query = db\select(ORGANISATION_TABLE)
				->condition('organisation_id', $id);
			$query->fields(ORGANISATION_TABLE, $fields);
			$query = $query->execute();
			$row = $query->fetchAssoc();

			foreach ($fields as $field) {
				$values[$field] = $row[$field];
			}
		}

		private function add(&$form_state) {
			$current_user_id = $GLOBALS['user']->uid;
			$values = $form_state['values'];

			$fields = array('name', 'member_count', 'type', 'industry',
				'registration_no', 'description');

			$db_values = array();
			foreach ($fields as $field) {
				if (empty($values[$field]) &&
					in_array($field, array('member_count', 'industry', 'registration_no', 'description'), true)) {
					$db_values[] = null;
				} else {
					$db_values[] = &$values[$field];
				}
			}

			$fields[] = 'user_created';
			$db_values[] = $current_user_id;
			$fields[] = 'user_modified';
			$db_values[] = $current_user_id;
			$fields[] = 'date_created';
			$db_values[] = null;

			$organisation_id = db\insert(ORGANISATION_TABLE)
				->fields($fields, $db_values)
				->execute();
			$this->get_parent()->set_id($organisation_id);
		}

		private function update(&$form_state, $id) {
			$current_user_id = $GLOBALS['user']->uid;
			$values = $form_state['values'];

			$fields = array('name', 'member_count', 'type', 'industry',
				'registration_no', 'description');

			$fields = drupal_map_assoc($fields, function($key) use (&$values) {
					if (empty($values[$key]) &&
						in_array($key, array('member_count', 'industry', 'registration_no', 'description'), true)) {
						return null;
					} else {
						return $values[$key];
					}
				});

			$fields['user_modified'] = $current_user_id;

			db\update(ORGANISATION_TABLE)
				->fields($fields)
				->condition('organisation_id', $id)
				->execute();
		}

	}

	class OrganisationAddressForm extends AddressFormBase {

		public function get_fields_mapping() {
			$result = array('blk_no', 'street_name',
				'building_name', 'floor_no', 'unit_no', 'postal_code',
				'flat_type');
			return array_combine($result, $result);
		}

		public function get_current_entry_ids() {
			$organisation_id = $this->get_parent()->get_id();
			$query = db\select(ORGANISATION_ADDRESS_TABLE)
				->condition('organisation_id', $organisation_id);
			$query->addField(ORGANISATION_ADDRESS_TABLE, 'organisation_address_id');
			$query = $query->execute();

			return $query->fetchCol(0);
		}

		public function get_first_entry(&$handle) {
			$organisation_id = $this->get_parent()->get_id();
			if (empty($organisation_id)) {
				return false;
			}

			$query = db\select(ORGANISATION_ADDRESS_TABLE, 'o')
				->condition('organisation_id', $organisation_id);
			$address_table_alias = $query->addJoin('INNER', CVWO_ADDRESS_TABLE, null, 'o.address_id = %alias.address_id');

			$query->fields($address_table_alias, array_keys($this->get_fields_mapping()));
			$query = $query->execute();
			$handle = $query;

			return $handle->fetchAssoc();
		}

		public function get_next_entry(&$handle) {
			return $handle->fetchAssoc();
		}

		public function insert($entry) {
			$organisation_id = $this->get_parent()->get_id();
			$current_user_id = $GLOBALS['user']->uid;

			//Start a transaction
			$transaction = db\transaction();

			//Insert the new address record first.
			$entry['user_created'] = $current_user_id;
			$entry['user_modified'] = $current_user_id;
			$entry['date_created'] = null;

			$nullable_field = function(&$value) {
				if (empty($value))
				{
					$value = null;
				}
			};
			$nullable_field($entry['flat_type']);

			$address_id = db\insert(CVWO_ADDRESS_TABLE)
				->fields($entry)
				->execute();

			$link = array();
			$link['organisation_id'] = $organisation_id;
			$link['address_id'] = $address_id;
			$link['user_created'] = $current_user_id;
			$link['user_modified'] = $current_user_id;
			$link['date_created'] = null;
			db\insert(ORGANISATION_ADDRESS_TABLE)
				->fields($link)
				->execute();

			//This transaction commit log should not be seen. It should be wrapped
			//in a larger transaction.
			assert(Database\Transaction::is_in_transaction());
			$transaction->commit('Inserted address record', 'Organisation Particulars', null);
		}

		public function update($id, $entry) {
			$current_user_id = $GLOBALS['user']->uid;
			$entry['user_modified'] = $current_user_id;

			//Start a transaction
			$transaction = db\transaction();

			//Get the address record ID
			$query = db\select(ORGANISATION_ADDRESS_TABLE)
				->condition('organisation_address_id', $id);
			$query->addField(ORGANISATION_ADDRESS_TABLE, 'address_id');
			$query = $query->execute();
			$row = $query->fetchAssoc();

			$nullable_field = function(&$value) {
				if (empty($value))
				{
					$value = null;
				}
			};
			$nullable_field($entry['flat_type']);
			
			//Update the address record.
			db\update(CVWO_ADDRESS_TABLE)
				->condition('address_id', $row['address_id'])
				->fields($entry)
				->execute();

			//This transaction commit log should not be seen. It should be wrapped
			//in a larger transaction.
			assert(Database\Transaction::is_in_transaction());
			$transaction->commit('Updated address record', 'Organisation Particulars', null);
		}

		public function delete($id) {
			//Start a transaction
			$transaction = db\transaction();

			//Get the address record ID
			$query = db\select(ORGANISATION_ADDRESS_TABLE)
				->condition('organisation_address_id', $id);
			$query->addField(ORGANISATION_ADDRESS_TABLE, 'address_id');
			$query = $query->execute();
			$row = $query->fetchAssoc();

			//Delete the link
			db\delete(ORGANISATION_ADDRESS_TABLE)
				->condition('organisation_address_id', $id)
				->execute();

			//Then delete from the addresses table
			db\delete(CVWO_ADDRESS_TABLE)
				->condition('address_id', $row['address_id'])
				->execute();

			//This transaction commit log should not be seen. It should be wrapped
			//in a larger transaction.
			assert(Database\Transaction::is_in_transaction());
			$transaction->commit('Deleted address record', 'Organisation particulars', null);
		}
	}
}
