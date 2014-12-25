<?php
/*
 * The MIT License
 *
 * Copyright 2007-2013 Computing for Volunteer Welfare Organizations (CVWO),
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

	require_once drupal_get_path('module', MODULE_NAME) . '/cvwobase_d7_person_relation.inc';

	use \CVWO\Base\Database as db;

	function organisation_contact($form, &$form_state, $organisation_id, $organisation_person_id = null, $link_person_id = null) {
		try {
			$instance = new OrganisationContactEditForm($organisation_id, $organisation_person_id, $link_person_id);
			return $instance->build($form_state);
		} catch (\InvalidArgumentException $e) {
			drupal_not_found();
			drupal_exit();
		}
	}

	function organisation_contact_validate(&$form, &$form_state) {
		$instance = $form['#instance'];
		$instance->validate($form, $form_state);
	}

	function organisation_contact_submit($form, &$form_state) {
		$instance = $form['#instance'];
		$instance->process($form_state);
	}

	class OrganisationContactEditForm extends PersonRelationshipEditForm {

		public function __construct($organisation_id, $organisation_person_id = null, $link_person_id = null) {
			//If the user specified organisation/<id>/contact/add<id> then he wants to
			//link the person and organisation. Make sure it is valid (i.e. there won't
			//be two relationships with the same two entities. If we do have such
			//a situation, we resolve it by degenerating it to the "edit existing
			//relationship" case.
			//
			//After this branch is done, we are down to the normal two cases:
			//where the user wants to add a new relation, or we are editing
			//an existing relation.
			if ($organisation_person_id === 'add' && !empty($link_person_id)) {
				$row = $query = db\select(ORGANISATION_CONTACT_PERSON_TABLE)
					->condition('organisation_id', $organisation_id)
					->condition('person_id', $link_person_id)
					->fields(ORGANISATION_CONTACT_PERSON_TABLE, array(
						'organisation_person_id'
					))
					->execute()
					->fetchAssoc();
				if ($row !== false) {
					//We have an existing relation between the org and person.
					$organisation_person_id = $row['organisation_person_id'];
				}
			}

			//Resolve the relationship if the user specified person/<id>/nok/<id>
			//This means we want to edit an existing relation.
			if (!empty($organisation_person_id) && $organisation_person_id !== 'add') {
				//Check that the NOK specified exists
				$query = db\select(namespace\ORGANISATION_CONTACT_PERSON_TABLE)
					->condition('organisation_person_id', $organisation_person_id)
					->condition('organisation_id', $organisation_id)
					->fields(namespace\ORGANISATION_CONTACT_PERSON_TABLE, array('organisation_id', 'person_id'))
					->execute();
				if ($query->rowCount() !== 1) {
					throw new \InvalidArgumentException(t('The organisation-contact_person relation'
						. 'for editing does not exist.'));
				}

				$row = $query->fetchAssoc();
				$this->set_main_supplement_id($organisation_person_id);
				$this->set_main_id($row['organisation_id']);
				$contact_person_id = $row['person_id'];
			} else {
				//Check that the organisation specified exists
				$query = db\select(namespace\ORGANISATION_TABLE)
					->condition('organisation_id', $organisation_id)
					->countQuery()
					->execute();

				if (intval($query->fetchField()) !== 1) {
					throw new \InvalidArgumentException(t('The organisation selected '
						. 'for editing does not exist.'));
				}

				$this->set_main_id($organisation_id);
				$contact_person_id = null;

				if ($organisation_person_id === 'add') {
					$this->set_main_supplement_id(parent::NEW_SUPPLEMENT_ID); //self::NEW_NOK_ID;
					if (!empty($link_person_id)) {
						$contact_person_id = $link_person_id;
					}
				}
			}

			$this->set_supplement_id($contact_person_id);
			parent::__construct();
		}

		protected function do_build(&$form_state) {
			parent::do_build($form_state);

			if ($this->get_main_supplement_id() === self::NEW_SUPPLEMENT_ID && (

				//Make sure that we are not saving NOK relations.
				empty($form_state['triggering_element']['#id']) ||
				$form_state['triggering_element']['#id'] !== 'supplement-relations-save'
				)) {
				$this->entries['person']['personal_particulars']['relationship'] = array(
					'#type' => 'cvwocodem2_autocomplete',
					'#title' => t('Designation'),
					'#module' => MODULE_NAME,
					'#question' => ORGANISATION_PERSON_DESIGNATION_QUESTION,
				);

				$this->entries['person']['personal_particulars']['remarks'] = array(
					'#type' => 'textarea',
					'#title' => t('Remarks'),
					'#size' => 50,
				);
			}
		}

		/**
		 * Override this function to return a relationship List table. Return a subclass of
		 * \CVWO\Base\PersonRelationshipListForm.
		 * 
		 * @return \CVWO\Base\PersonRelationshipListForm
		 */
		protected function get_supplement_list_form() {
			return new OrganisationContactListForm($this->get_organisation_id());
		}

		protected function do_validate(&$form, &$form_state) {
			if ($form_state['triggering_element']['#id'] === 'supplement-relations-save') {
				//If we are only submitted the relations table, just validate that.
				$this->supplement_list_form->validate($form, $form_state);
			} else if ($form_state['triggering_element']['#id'] === 'Save') {

				parent::do_validate($form, $form_state);
			} else {
				//Otherwise the entire form needs validation.
				parent::do_validate($form, $form_state);
			}
		}

		protected function do_process(&$form_state) {
			if ($form_state['triggering_element']['#id'] === 'supplement-relations-add') {
				//Just redirect to the Add New contact person page.
				drupal_goto(sprintf(namespace\MODULE_NAME . '/organisation/%d/contact/add', $this->get_main_id()));
			} else if ($form_state['triggering_element']['#id'] === 'supplement-relations-save') {
				$this->supplement_list_form->process($form_state);
			} else {
				$transaction = db\transaction();
				parent::do_process($form_state);

				$transaction->commit(
					ORGANISATION_ADD_LOG_MESSAGE, ORGANISATION_TAG, MODULE_NAME, array(
					'%org' => Organisation::from_id($this->get_organisation_id())->get_name(),
					'%person' => Person::from_id($this->get_contact_person_id())->get_name()));
			}
		}

		protected function process_new_relation(array &$form_state) {
			$values = &$form_state['values'];
			parent::process_new_relation($form_state);

			//Transform the fields into an array. This is for the situation where
			//one relationship becomes two; the first relation is submitted to us
			//as a scalar.
			//
			//This lambda is from parent::process_new_relation
			$fixup_array = function($field) use (&$values)
			{
				if (!isset($values[$field]) || !is_array($values[$field]))
				{
					$values[$field] = empty($values[$field]) ? array() :
						array($values[$field]);
				}
			};
			$fixup_array('supplement_remarks');
			$values['supplement_remarks'][] = $values['remarks'];
		}

		/**
		 * Gets the Person ID of the person we are currently editing.
		 *
		 * @return integer
		 */
		public function get_organisation_id() {
			return $this->get_main_id();
		}

		/**
		 * Gets the Person ID of the NOK we are currently editing.
		 *
		 * @return integer
		 */
		public function get_contact_person_id() {
			return $this->get_supplement_id();
		}

	}

	class OrganisationContactListForm extends PersonRelationshipListForm {

		/**
		 * Constructor.
		 * @param integer organisation_id The organisation ID we are editing
		 */
		public function __construct($organisation_id) {
			$this->set_main_id($organisation_id);

			//Check that the person specified exists; and that if a NOK is
			//specified, the record also exists.
			$query = db\select(namespace\ORGANISATION_TABLE)
				->condition('organisation_id', $this->get_main_id())
				->countQuery()
				->execute();
			if (intval($query->fetchField()) !== 1) {
				throw new \InvalidArgumentException(t('The organisation selected '
					. 'for editing does not exist.'));
			}

			parent::__construct();
		}

		protected function do_build(&$form_state) {
			//Get the Supplements of this person if we did not submit any information
			//or if we did not pick our Save button.

			if (empty($form_state['input']) || (
				!empty($form_state['triggering_element']['#id']) &&
				$form_state['triggering_element']['#id'] !== 'supplement-relations-add'))
			{
				$this->load($form_state, $this);
			}

			$this->entries = array_merge_recursive($this->entries, array(
				'#theme' => 'table',
				'#header' => array(
					t('Primary'), //Primary
					t('NRIC'),
					t('Name'),
					t('Designation'),
					t('Remarks'),
					'', //Edit
					'' //Delete
				),
				'#rows' => array(),
				'#empty' => t('No registered contact person.')
			));

			//Build one entry first.
			$entries = &$this->entries;
			$entries['supplement_primary'] = array(
				'#type' => 'radio'
			);
			$entries['supplement_nric'] = array(
				'#type' => 'item'
			);
			$entries['supplement_name'] = array(
				'#type' => 'item'
			);
			$entries['supplement_relationship'] = array(
				'#type' => 'cvwocodem2_autocomplete',
				'#module' => MODULE_NAME,
				'#question' => ORGANISATION_PERSON_DESIGNATION_QUESTION,
			);
			$entries['supplement_remarks'] = array(
				'#type' => 'textfield',
			);
			$entries['supplement_link'] = array(
				'#type' => 'link',
				'#href' => null,
				'#title' => t('View/Edit'),
				'#attributes' => array(
					'class' => array(
						'form_multiple_input_entry'
					)
				)
			);

			//Merge
			parent::do_build($form_state);

			//Regroup the entire multiple input form; then insert the Save button
			$this->entries = array_merge_recursive(
				$this->entries, array(
					'save' => array(
						'#type' => 'submit',
						'#value' => t('Save Relations'),
						'#id' => 'supplement-relations-save',

						//And restrict our validation errors to the relationship
						//that we submit. We only submit and process this part
						//of the form.
						'#limit_validation_errors' => array(
							array('supplement_relationship'),
							array('supplement_remarks'),
							array('supplement_primary'),
							array($this->get_primary_key_field_name())
						)
					),
					'add' =>
						$this->entries['form']['add_more_btn']
				)
			);

			//Remove the onclick functionality for the Add More button. We can't
			//just insert a new row, we need to generate the person form.
			unset($this->entries['add']['#attributes']['onclick']);
			$this->entries['add']['#name'] = 'supplement-relations-add';
			$this->entries['add']['#id'] = 'supplement-relations-add';
		}

		protected function do_process(&$form_state) {
			$transaction = db\transaction();
			parent::do_process($form_state);

			$transaction->commit(
				ORGANISATION_ADD_LOG_MESSAGE, ORGANISATION_TAG, MODULE_NAME,
				array('@org' => Organisation::from_id($this->get_main_id())->get_name()));

			drupal_set_message(t('The Contact Person list for the organisation @organisation has been updated.',
				array(
					'@organisation' => Organisation::from_id($this->get_main_id())->get_name()
				)));
		}

		/**
		 * Gets the View/Edit URI to view the given $person_id as a NOK.
		 *
		 * @param integer $person_id The Person ID to view/edit.
		 * @return string The URI to edit the given Person ID as a form.
		 */
		protected function get_edit_link($organisation_id) {
			return sprintf(namespace\MODULE_NAME . '/organisation/%d/contact/%d', $this->get_parent()->get_organisation_id(), $organisation_id);
		}

		public function get_fields_mapping() {
			return array(
				'organisation_person_id' => '#primary',
				'nric_fin' => '##supplement_nric',
				'name' => '##supplement_name',
				'link' => '##supplement_link',
				'primary_contact' => 'supplement_primary',
				'designation' => 'supplement_relationship',
				'remarks' => 'supplement_remarks',
			);
		}

		public function get_current_entry_ids() {
			$organisation_id = $this->get_parent()->get_organisation_id();
			$query = db\select(namespace\ORGANISATION_CONTACT_PERSON_TABLE)
				->condition('organisation_id', $organisation_id);
			$query->addField(namespace\ORGANISATION_CONTACT_PERSON_TABLE, 'organisation_person_id');
			$query = $query->execute();

			return $query->fetchCol(0);
		}

		public function get_first_entry(&$handle) {
			$organisation_id = $this->get_parent()->get_organisation_id();
			$query = db\select(namespace\ORGANISATION_CONTACT_PERSON_TABLE, 'cp')
				->condition('cp.organisation_id', $organisation_id);
			$person_table_name = $query->addJoin('INNER', namespace\PERSON_TABLE, 'person', 'cp.person_id=person.person_id');
			$query->fields($person_table_name, array('nric_fin', 'name'));
			$query->fields('cp', array('organisation_person_id', 'primary_contact', 'designation', 'remarks'));
			$query = $query->execute();
			$handle = $query;

			return $this->augment_entry($handle->fetchAssoc());
		}

		public function get_next_entry(&$handle) {
			return $this->augment_entry($handle->fetchAssoc());
		}

		private function augment_entry($row) {
			if ($row === false) {
				return $row;
			}

			$row['link'] = $this->get_edit_link($row['organisation_person_id']);
			return $row;
		}

		public function insert($entry) {
			$current_user_id = $GLOBALS['user']->uid;
			//Remove unused values
			unset($entry['organisation_person_id'], $entry['nric_fin'], $entry['name']);

			//Link the people we are editing
			$entry['organisation_id'] = $this->get_parent()->get_organisation_id();
			$entry['person_id'] = $this->get_parent()->get_contact_person_id();
			$entry['primary_contact'] = $entry['primary_contact'] ? 1 : 0;
			$entry['user_created'] = $current_user_id;
			$entry['date_created'] = null;
			$entry['user_modified'] = $current_user_id;

			db\insert(namespace\ORGANISATION_CONTACT_PERSON_TABLE)
				->fields($entry)
				->execute();
		}

		public function update($id, $entry) {
			//Normalise the 'primary_nok' value. SQL expects an integer.
			$entry['primary_contact'] = $entry['primary_contact'] ? 1 : 0;

			db\update(namespace\ORGANISATION_CONTACT_PERSON_TABLE)
				->condition('organisation_person_id', $id)
				->fields($entry)
				->execute();
		}

		public function delete($id) {
			db\delete(namespace\ORGANISATION_CONTACT_PERSON_TABLE)
				->condition('organisation_person_id', $id)
				->execute();
		}
	}
}
