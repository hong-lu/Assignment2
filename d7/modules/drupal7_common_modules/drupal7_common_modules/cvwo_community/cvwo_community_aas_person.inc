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
	use \CVWO\Base\Database as db;

	/**
	 * Implements form_hook_alter for the Community Person edit form.
	 */
	function aas_person_form_hook(&$form, &$form_state, $form_id, \CVWO\Base\Form $form_object)
	{
		if ($form_id !== 'CVWO\Community\PersonEditForm')
		{
			return;
		}

		//Retain the submit button
		$submit = &$form['submit'];
		unset($form['submit']);

		$aas_form = new AASItemForm();
		$form_object->add_child($aas_form);
		$form['aas_item'] = array_merge(
			array(
				'#type'			 => 'fieldset',
				'#title'		 => t('AAS Item'),
				'#collapsible'	 => true
			),
			$aas_form->build($form_state)
		);

		//Reinstate the submit button
		$form['submit'] = &$submit;
		unset($submit);
	}

	class AASItemForm extends \CVWO\Base\FormMultipleInput
	                  implements \CVWO\Base\FormMultipleInputFormAdapter
	{
		protected function do_build(&$form_state)
		{
			$form = array();
			if (empty($form_state['input']) && $this->get_parent()->get_id() !== null)
			{
				$this->load($form_state, $this);
			}

			$form['item'] = array(
				'#type'				 => 'cvwocodem2_select',
				'#module'			 => MODULE_NAME,
				'#question'			 => AAS_ITEM_QUESTION,
				'#title'			 => t(AAS_ITEM_QUESTION),
				'#title_display'	 => 'none',
				'#required'			 => true
			);

			$this->entries = $form;
			parent::do_build($form_state);
		}

		protected function do_validate(&$form, &$form_state)
		{
			parent::do_validate($form, $form_state, $this);
		}

		protected function do_process(&$form_state)
		{
			$this->save($form_state, $this);
		}

		public function get_fields_mapping()
		{
			return array('aas_item' => 'item');
		}

		public function get_current_entry_ids()
		{
			return db\select(AAS_PERSON_TABLE)
				->condition('person_id', $this->get_parent()->get_id())
				->fields(AAS_PERSON_TABLE, array('person_aas_id'))
				->execute()
				->fetchCol(0);
		}

		public function get_first_entry(&$handle)
		{
			$query = db\select(AAS_PERSON_TABLE)
				->condition('person_id', $this->get_parent()->get_id())
				->fields(AAS_PERSON_TABLE, array('aas_item'))
				->execute();

			$handle = $query;
			return $query->fetchAssoc();
		}

		public function get_next_entry(&$handle)
		{
			return $handle->fetchAssoc();
		}

		public function insert($entry)
		{
			$entry['person_id'] = $this->get_parent()->get_id();
			$query = db\insert(AAS_PERSON_TABLE)
				->fields($entry)
				->execute();
		}

		public function update($id, $entry)
		{
			$entry['person_id'] = $this->get_parent()->get_id();
			$query = db\update(AAS_PERSON_TABLE)
				->condition('person_aas_id', $id)
				->fields($entry)
				->execute();
		}

		public function delete($id)
		{
			db\delete(AAS_PERSON_TABLE)
				->condition('person_aas_id', $id)
				->execute();
		}
	}
}
