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
 * Handles the NOK information for the abstract CVWO Person object.
 */

namespace CVWO\Base {
	use \CVWO\Base\Database as db;

	require_once drupal_get_path('module', MODULE_NAME) . '/cvwobase_d7_person_relation.inc';

	/**
	 * Builds the person NOK form.
	 *
	 * @param array $form The form to build into.
	 * @param array $form_state The current state of the form.
	 * @param integer $person_id The ID of the person to edit.
	 * @param integer $person_nok_id The ID of the Person/NOK relation to edit. Can
	 *                               be null to indicate not editing anyone; can be
	 *                               'add' to indicate a new relationship is needed.
	 * @param integer $link_person_id The ID of the person to link to. This is
	 *                                only used if $nok_id is 'add' and we want to
	 *                                link an existing person as a NOK.
	 * @return array The compiled Drupal form.
	 */
	function person_nok_form($form, &$form_state, $person_id, $person_nok_id = null,
		$link_person_id = null)
	{
		try
		{
			$instance = new PersonNokEditForm($person_id, $person_nok_id, $link_person_id);
			return $instance->build($form_state);
		}
		catch (\InvalidArgumentException $e)
		{
			drupal_not_found();
			drupal_exit();
		}
	}

	/**
	 * Validates the person NOK form.
	 *
	 * @param array $form The Drupal form
	 * @param array $form_state The Drupal state of the form.
	 */
	function person_nok_form_validate(&$form, &$form_state)
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
	function person_nok_form_submit($form, &$form_state)
	{
		$instance = $form['#instance'];
		$instance->process($form_state);
	}

	class PersonNokEditForm extends PersonRelationshipEditForm
	{
		public function __construct($person_id, $person_nok_id = null,
			$link_person_id = null)
		{
			//If the user specified person/<id>/nok/add<id> then he wants to
			//link two users together. Make sure it is valid (i.e. there won't
			//be two relationships with the same two people. If we do have such
			//a situation, we resolve it by degenerating it to the "edit existing
			//relationship" case.
			//
			//After this branch is done, we are down to the normal two cases:
			//where the user wants to add a new relation, or we are editing
			//an existing relation.
			if ($person_nok_id === 'add' && !empty($link_person_id))
			{
				$row = $query = db\select(PERSONAL_PARTICULARS_PERSON_NOK_TABLE)
					->condition('person_id', $person_id)
					->condition('nok_id', $link_person_id)
					->fields(PERSONAL_PARTICULARS_PERSON_NOK_TABLE, array(
						'person_nok_id'
					))
					->execute()
					->fetchAssoc();

				if ($row !== false)
				{
					//We have an existing relation between these two people.
					$person_nok_id = $row['person_nok_id'];
				}
			}

			//Resolve the relationship if the user specified person/<id>/nok/<id>
			//This means we want to edit an existing relation.
			if (!empty($person_nok_id) && $person_nok_id !== 'add')
			{
				//Check that the NOK specified exists
				$query = db\select(namespace\PERSONAL_PARTICULARS_PERSON_NOK_TABLE)
					->condition('person_nok_id', $person_nok_id)
					->condition('person_id', $person_id)
					->fields(namespace\PERSONAL_PARTICULARS_PERSON_NOK_TABLE,
						array('person_id', 'nok_id'))
					->execute();
				if ($query->rowCount() !== 1)
				{
					throw new \InvalidArgumentException(t('The next-of-kin relation'
						. 'for editing does not exist.'));
				}

				$row = $query->fetchAssoc();
				$this->set_main_supplement_id($person_nok_id);
				$this->set_main_id($row['person_id']);
				$nok_id = $row['nok_id'];
			}
			else
			{
				//Check that the person specified exists
				$query = db\select(\CVWO\Base\PERSON_TABLE)
					->condition('person_id', $person_id)
					->countQuery()
					->execute();

				if (intval($query->fetchField()) !== 1)
				{
					throw new \InvalidArgumentException(t('The person selected '
						. 'for editing does not exist.'));
				}

				$this->set_main_id($person_id);
				$nok_id = null;

				if ($person_nok_id === 'add')
				{
					$this->set_main_supplement_id(parent::NEW_SUPPLEMENT_ID);
					if (!empty($link_person_id))
					{
						$nok_id = $link_person_id;
					}
				}
			}

			$this->set_supplement_id($nok_id);
			parent::__construct();
		}

		protected function do_build(&$form_state)
		{
			parent::do_build($form_state);
			if ($this->get_main_supplement_id() === self::NEW_SUPPLEMENT_ID && (

					//Make sure that we are not saving NOK relations.
					empty($form_state['triggering_element']['#id']) ||
					$form_state['triggering_element']['#id'] !== 'supplement-relations-save'
				))
			{
				$this->entries['person']['personal_particulars']['relationship'] = array(
					'#type'			 => 'cvwocodem2_select',
					'#module'		 => MODULE_NAME,
					'#question'		 => PERSONAL_PARTICULARS_NOK_RELATIONSHIP_QUESTION,
					'#title'		 => t('Relationship'),
					'#required'		 => true
				);
			}
		}

		/**
		 * Override this function to return a relationship List table. Return a subclass of
		 * \CVWO\Base\PersonRelationshipListForm.
		 *
		 * @return \CVWO\Base\PersonRelationshipListForm
		 */
		protected function get_supplement_list_form()
		{
			return $this->get_nok_list_form();
		}

		/**
		 * Extenders should override this function to return its own listform
		 *
		 * @return type
		 */
		protected function get_nok_list_form()
		{
			return new PersonNokListForm($this->get_person_id());
		}

		protected function do_validate(&$form, &$form_state)
		{
			if ($form_state['triggering_element']['#id'] === 'supplement-relations-save')
			{
				//If we are only submitted the relations table, just validate that.
				$this->supplement_list_form->validate($form, $form_state);
			}
			else if ($form_state['triggering_element']['#id'] === 'Save')
			{
				//Make sure that the user is not adding a NOK to the himself.
				if ($form_state['values']['person_id'] == $this->get_main_id())
				{
					form_set_error('person', t('Cannot add a person as his own Next-of-Kin'));
				}

				parent::do_validate($form, $form_state);
			}
			else
			{
				//Otherwise the entire form needs validation.
				parent::do_validate($form, $form_state);
			}
		}

		protected function do_process(&$form_state)
		{
			if ($form_state['triggering_element']['#id'] === 'supplement-relations-add')
			{
				//Just redirect to the Add New NOK page.
				drupal_goto(sprintf(namespace\MODULE_NAME . '/person/%d/nok/add',
					$this->get_main_id()));
			}
			else if ($form_state['triggering_element']['#id'] === 'supplement-relations-save')
			{
				$this->supplement_list_form->process($form_state);
			}
			else
			{
				$transaction = db\transaction();
				parent::do_process($form_state);

				$transaction->commit(
					PERSONAL_PARTICULARS_NOK_ADD_MESSAGE,
					PERSONAL_PARTICULARS_NOK_TAG,
					MODULE_NAME,
					array(
						'%nok' => Person::from_id($this->get_nok_id())->get_name(),
						'%person' => Person::from_id($this->get_person_id())->get_name()));
			}
		}

		/**
		 * Gets the Person ID of the person we are currently editing.
		 *
		 * @return integer
		 */
		public function get_person_id()
		{
			return $this->get_main_id();
		}

		/**
		 * Gets the Person ID of the NOK we are currently editing.
		 *
		 * @return integer
		 */
		public function get_nok_id()
		{
			return $this->get_supplement_id();
		}
	}

	class PersonNokListForm extends PersonRelationshipListForm
	{
		/**
		 * Constructor.
		 * @param integer $person_id The Person ID of the person we are editing
		 *                           NOK information for.
		 */
		public function __construct($person_id)
		{
			$this->set_main_id($person_id);

			//Check that the person specified exists; and that if a NOK is
			//specified, the record also exists.
			$query = db\select(CVWO_PERSON_TABLE)
				->condition('person_id', $this->get_main_id())
				->countQuery()
				->execute();
			if (intval($query->fetchField()) !== 1)
			{
				throw new \InvalidArgumentException(t('The person selected'
					. 'for editing does not exist.'));
			}

			parent::__construct();
		}

		protected function do_build(&$form_state)
		{
			//Get the Supplements of this person if we did not submit any information
			//or if we did not pick our Save button.
			if (empty($form_state['input']) || (
				!empty($form_state['triggering_element']['#id']) &&
				$form_state['triggering_element']['#id'] !== 'supplement-relations-save'))
			{
				$this->load($form_state, $this);
			}

			$this->entries = array_merge_recursive($this->entries,
				array(
					'#theme'	 => 'table',
					'#header'	 => array(
						t('Primary'), //Primary
						t('NRIC'),
						t('Name'),
						t('Relationship'),
						'',	//Edit
						''	//Delete
					),
					'#rows'		 => array(),
					'#empty'	 => t('No registered next-of-kin')
				));

			//Build one entry first.
			$entries = &$this->entries;
			$entries['supplement_primary']		 = array(
				'#type'			 => 'radio'
			);
			$entries['supplement_nric']			 = array(
				'#type'			 => 'item'
			);
			$entries['supplement_name']			 = array(
				'#type'			 => 'item'
			);
			$entries['supplement_relationship']	 = array(
				'#type'			 => 'cvwocodem2_select',
				'#module'		 => MODULE_NAME,
				'#question'		 => PERSONAL_PARTICULARS_NOK_RELATIONSHIP_QUESTION,
			);
			$entries['supplement_link']			 = array(
				'#type'			 => 'link',
				'#href'			 => null,
				'#title'		 => t('View/Edit'),
				'#attributes'	 => array(
					'class'		 => array(
						'form_multiple_input_entry'
					)
				)
			);

			//Merge
			parent::do_build($form_state);

			//Regroup the entire multiple input form; then insert the Save button
			$this->entries = array_merge_recursive(
				$this->entries, array(
					'save'	 => array(
						'#type'		 => 'submit',
						'#value'	 => t('Save Relations'),
						'#id'		 => 'supplement-relations-save',

						//And restrict our validation errors to the relationship
						//that we submit. We only submit and process this part
						//of the form.
						'#limit_validation_errors' => array(
							array('supplement_relationship'),
							array('supplement_primary'),
							array($this->get_primary_key_field_name())
						)
					),
					'add'	 => $this->entries['form']['add_more_btn']
				)
			);

			//Remove the onclick functionality for the Add More button. We can't
			//just insert a new row, we need to generate the person form.
			unset($this->entries['add']['#attributes']['onclick']);
			$this->entries['add']['#name'] = 'supplement-relations-add';
			$this->entries['add']['#id'] = 'supplement-relations-add';
		}

		protected function do_process(&$form_state)
		{
			$transaction = db\transaction();
			parent::do_process($form_state);

			$transaction->commit(
				PERSONAL_PARTICULARS_NOK_RELATION_UPDATE_MESSAGE,
				PERSONAL_PARTICULARS_NOK_TAG,
				MODULE_NAME,
				array('%person' => Person::from_id($this->get_main_id())->get_name()));

			drupal_set_message(t('The Next-of-Kin list for @person has been updated.',
				array(
					'@person' => Person::from_id($this->get_main_id())->get_name()
				)));
		}

		/**
		 * Gets the View/Edit URI to view the given $person_id as a NOK.
		 *
		 * @param integer $person_id The Person ID to view/edit.
		 * @return string The URI to edit the given Person ID as a form.
		 */
		protected function get_edit_link($person_id)
		{
			return sprintf(namespace\MODULE_NAME . '/person/%d/nok/%d',
				$this->get_parent()->get_person_id(), $person_id);
		}

		public function get_fields_mapping()
		{
			return array(
				'person_nok_id'	 => '#primary',
				'nric_fin'		 => '##supplement_nric',
				'name'			 => '##supplement_name',
				'link'			 => '##supplement_link',
				'primary_nok'	 => 'supplement_primary',
				'relationship'	 => 'supplement_relationship'
			);
		}

		public function get_current_entry_ids()
		{
			$person_id = $this->get_parent()->get_person_id();
			$query = db\select(namespace\PERSONAL_PARTICULARS_PERSON_NOK_TABLE)
				->condition('person_id', $person_id);
			$query->addField(namespace\PERSONAL_PARTICULARS_PERSON_NOK_TABLE, 'person_nok_id');
			$query = $query->execute();

			return $query->fetchCol(0);
		}

		public function get_first_entry(&$handle)
		{
			$person_id = $this->get_parent()->get_person_id();
			$query = db\select(namespace\PERSONAL_PARTICULARS_PERSON_NOK_TABLE, 'nok')
				->condition('nok.person_id', $person_id);
			$person_table_name = $query->addJoin('INNER', namespace\PERSON_TABLE,
				'person', 'nok.nok_id=person.person_id');
			$query->fields($person_table_name, array('nric_fin', 'name'));
			$query->fields('nok', array('person_nok_id', 'primary_nok', 'relationship'));
			$query = $query->execute();
			$handle = $query;

			return $this->augment_entry($handle->fetchAssoc());
		}

		public function get_next_entry(&$handle)
		{
			return $this->augment_entry($handle->fetchAssoc());
		}

		private function augment_entry($row)
		{
			if ($row === false)
			{
				return $row;
			}

			$row['link'] = $this->get_edit_link($row['person_nok_id']);
			return $row;
		}

		public function insert($entry)
		{
			//Remove unused values
			unset($entry['person_nok_id'], $entry['nric_fin'], $entry['name']);

			//Link the people we are editing
			$entry['person_id'] = $this->get_parent()->get_person_id();
			$entry['nok_id'] = $this->get_parent()->get_nok_id();
			$entry['primary_nok'] = $entry['primary_nok'] ? 1 : 0;

			db\insert(namespace\PERSONAL_PARTICULARS_PERSON_NOK_TABLE)
				->fields($entry)
				->execute();
		}

		public function update($id, $entry)
		{
			//Normalise the 'primary_nok' value. SQL expects an integer.
			$entry['primary_nok'] = $entry['primary_nok'] ? 1 : 0;

			db\update(namespace\PERSONAL_PARTICULARS_PERSON_NOK_TABLE)
					->condition('person_nok_id', $id)
					->fields($entry)
					->execute();
		}

		public function delete($id)
		{
			db\delete(namespace\PERSONAL_PARTICULARS_PERSON_NOK_TABLE)
					->condition('person_nok_id', $id)
					->execute();
		}

		public function get_person_id() {
			return $this->get_parent()->get_person_id();
		}
	}
}
