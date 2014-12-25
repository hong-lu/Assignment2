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
	 * Remember to override __construct function in PersonRelationshipEditForm's
	 * subclasses
	 */
	abstract class PersonRelationshipEditForm extends Form
	{
		/**
		 * The ID of the main/supplement (Person/NOK or Organisation/Contact Person)
		 * relation we are editing.
		 * @var integer
		 */
		private $main_supplement_id;

		/**
		 * The person we are editing.
		 * @var integer
		 */
		private $main_id;

		/**
		 * The person (who is a supplment (NOK/contact person)) we are editing.
		 * @var integer
		 */
		private $supplement_id;

		/**
		 * The ID used for the $this->main_supplement_id value if we are adding a new
		 * NOK.
		 */
		const NEW_SUPPLEMENT_ID = -1;

		/**
		 * The Person edit form for supplement (NOK/contact person)
		 *
		 * @var \CVWO\Base\PersonEditForm
		 */
		protected $person_form;

		/**
		 * The supplement (NOK/contact person) list form.
		 *
		 * @var PersonNokListForm
		 */
		protected $supplement_list_form;
		
		public function serialize()
		{
			return serialize(array(
				$this->main_supplement_id,
				$this->main_id,
				$this->supplement_id,
				parent::serialize()
			));
		}
		
		public function unserialize($data)
		{
			$unpack = unserialize($data);
			$this->main_supplement_id = $unpack[0];
			$this->main_id = $unpack[1];
			$this->supplement_id = $unpack[2];
			parent::unserialize($unpack[3]);
		}

		protected function do_build(&$form_state)
		{
			$form = array(
				'#attached'		 => array(
					'js'		 => array(
						'//cdnjs.cloudflare.com/ajax/libs/fancybox/2.1.4/jquery.fancybox.pack.min.js'
					),
					'css'		 => array(
						'//cdnjs.cloudflare.com/ajax/libs/fancybox/2.1.4/jquery.fancybox.min.css' => array(
							'type' => 'external'
						)
					)
				),
			);

			//Build the list of supplements at the top of the page.
			$this->supplement_list_form = $this->get_supplement_list_form();
			$this->add_child($this->supplement_list_form);
			$form['supplement_list'] = $this->supplement_list_form->build($form_state);

			//Build the remainder of the Person Edit form
			$this->entries = array_merge_recursive($this->entries, $form);
			if (!empty($this->main_supplement_id))
			{
				$this->person_form = $this->get_person_edit_form($form_state);
				$this->add_child($this->person_form);
				$this->entries['person'] = array(
					'#type'			 => 'container',
				);
				$this->entries['person'] += $this->person_form->build($form_state);

				//Hook the autocomplete the NRIC/Name
				$this->entries['#attached']['js'][] =
					drupal_get_path('module', MODULE_NAME) . '/cvwobase_d7_person_nok.js';

				$this->entries['person']['personal_particulars']['nric_fin']['#autocomplete_path']
					= MODULE_NAME . '/person/search';
				$this->entries['person']['personal_particulars']['name']['#autocomplete_path']
					= MODULE_NAME . '/person/search';
			}
		}

		/**
		 * Override this function to return a relationship list table. Return a subclass of
		 * \CVWO\Base\PersonRelationshipListForm.
		 *
		 * @return \CVWO\Base\PersonRelationshipListForm
		 */
		protected abstract function get_supplement_list_form();

		/**
		 * Override this function to return a different person edit form. Return a
		 * subclass of \CVWO\Base\PersonEditForm.
		 *
		 * @return \CVWO\Base\PersonEditForm
		 */
		protected function get_person_edit_form(&$form_state)
		{
			return new PersonEditForm($this->supplement_id);
		}

		/**
		 * Extenders of this class might want to hook the triggering element with
		 * id inside $this->getRelationshipFieldRelated()['save'] to handle the
		 * redirection to the add supplement page for the extension form.
		 *
		 * @param array $form_state
		 */
		protected function do_process(&$form_state)
		{
			if ($form_state['triggering_element']['#id'] === 'Save')
			{
				$transaction = db\transaction();

				//Save the person first, so that we can handle supplement adding.
				$this->person_form->process($form_state);

				//Add a new relation if this is an add operation. This alters form
				//data so that we can just use the existing frameworks to handle
				//the new relation.
				if ($this->main_supplement_id === self::NEW_SUPPLEMENT_ID)
				{
					$this->process_new_relation($form_state);
					$this->supplement_list_form->process($form_state);
				}

				$transaction->commit(null, null, null);
			}
		}

		protected function process_children(&$form_state)
		{
			//Flip the order: we need to process the personal particulars form
			//to create the person first, before we can link them.
			$children = $this->get_children();
			$children_keys = array_keys($children);
			$children_keys = array_merge(array_slice($children_keys, 1), array($children_keys[0]));
			foreach ($children_keys as $key)
			{
				$children[$key]->process($form_state);
			}
		}

		/**
		 * Handles a new supplement insertion. This modifies the form data so that we
		 * will use the Multiple Input Form framework to handle the new relation
		 * being defined.
		 */
		protected function process_new_relation(array &$form_state)
		{
			$values = &$form_state['values'];

			//Transform the fields into an array. This is for the situation where
			//one relationship becomes two; the first relation is submitted to us
			//as a scalar.
			$fixup_array = function($field) use (&$values)
			{
				if (!isset($values[$field]) || !is_array($values[$field]))
				{
					$values[$field] = empty($values[$field]) ? array() :
						array($values[$field]);
				}
			};
			$fixup_array('supplement_relationship');
			$values['supplement_relationship'][] = $values['relationship'];

			//We need to add one more row to the primary key for the NOK list form
			//so that the list knows that we added one more person. Generate
			//a new index
			$form = $this->entries['supplement_list']['#instance'];
			$indexers = &$form_state['input'][$form->get_indexer_key_field_name()];
			if (empty($indexers))
			{
				$indexers = array();
			}
			for ($i = count($indexers); in_array($i, $indexers, true); ++$i);

			//Then fill the primary key with it.
			$form_state['input'][$form->get_indexer_key_field_name()][] = $i;
			$values[$form->get_primary_key_field_name()][] = null;

			//Also, if we have no existing supplement, we have to create this new person
			//as the primary supplement.
			if (empty($values['supplement_primary']))
			{
				$values['supplement_primary'] = $this->get_supplement_id();
			}

			//Create a dummy element so our multiple input form adapter can see
			//that this field is a radio button. Although we didn't actually exist
			//in the table!
			$this->supplement_list_form->entries['supplement_primary' . $i] = array(
				'#type'				 => 'radio',
				'#return_value'		 => $this->get_supplement_id()
			);
		}

		/**
		 * Gets the id of the relationship between main and supplement.
		 *
		 * @return integer
		 */
		public function get_main_supplement_id()
		{
			return $this->main_supplement_id;
		}

		/**
		 * Sets the ID of the relationship between main and supplment.
		 *
		 * @return integer
		 */
		public function set_main_supplement_id($main_supplement_id)
		{
			$this->main_supplement_id = $main_supplement_id;
		}

		/**
		 * Gets the Person ID of the person we are currently editing.
		 *
		 * @return integer
		 */
		public function get_main_id()
		{
			return $this->main_id;
		}

		/**
		 * Sets the Person ID of the person we are currently editing.
		 *
		 * @return integer
		 */
		public function set_main_id($main_id)
		{
			$this->main_id = $main_id;
		}

		/**
		 * Gets the Person ID of the supplement we are currently editing.
		 *
		 * @return integer
		 */
		public function get_supplement_id()
		{
			if (isset($this->person_form))
			{
				if ($this->person_form->get_id() !== $this->supplement_id)
				{
					$this->supplement_id = $this->person_form->get_id();
				}
			}

			return $this->supplement_id;
		}

		/**
		 * Sets the Person ID of the NOK we are currently editing.
		 *
		 * @return integer
		 */
		public function set_supplement_id($supplement_id)
		{
			$this->supplement_id = $supplement_id;
		}
	}

	/**
	 * PersonNokListForm and OrganisationContactListForm extend this form
	 * Remember to override the following functions:
	 * __construct();
	 */
	abstract class PersonRelationshipListForm extends FormMultipleInputTableForm
	                                          implements FormMultipleInputFormAdapter
	{
		/**
		 * The person we are editing information for.
		 *
		 * @var integer
		 */
		private $main_id;

		protected function do_validate(&$form, &$form_state)
		{
			parent::do_validate($form, $form_state, $this);
		}

		protected function do_process(&$form_state)
		{
			$transaction = db\transaction();
			$this->save($form_state, $this);

			//This transaction commit log should not be seen. It should be wrapped
			//in a larger transaction.
			assert(Database\Transaction::is_in_transaction());
			$transaction->commit(null, null, null);
		}

		public function get_indexer_key_field_name()
		{
			return parent::get_indexer_key_field_name();
		}

		public function get_primary_key_field_name()
		{
			return parent::get_primary_key_field_name();
		}

		/**
		 * Gets the ID of the person whom we are modifying NOK for.
		 *
		 * @return integer
		 */
		public function get_main_id()
		{
			return $this->main_id;
		}

		public function set_main_id($main_id)
		{
			$this->main_id = $main_id;
		}
	}
}
