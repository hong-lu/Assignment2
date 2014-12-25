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
	/**
	 * @file
	 * Implements the CVWO Orgaqnisation selection autocomplete form field.
	 *
	 * Attributes:
	 * #default_value: The Organisation ID to use as the value/default value for
	 * #value          this field
	 *
	 * On the client side, if you want to register another autocomplete handler,
	 * the raw JSON is stored in this.raw_value.
	 */

	/**
	 * Normalises the element for display.
	 *
	 * @param array $element The form element.
	 */
	function organisation_select_normalise(&$element)
	{
		if (empty($element['#name']))
		{
			//This shouldn't happen, but is necessary.
			trigger_error('#name must be set (by Drupal) for select_organisation elements', E_USER_ERROR);
		}

		//Get the organisation object to interface with.
		$element_current_value = isset($element['#value']) ?
			$element['#value'] : $element['#default_value'];
		$organisation = empty($element_current_value) ||
			!Organisation::exists($element_current_value) ?
				null : Organisation::from_id($element_current_value);

		//Make sure we have a valid autocomplete path.
		$element['#autocomplete_path'] = MODULE_NAME . '/organisation/search';
		
		$parents = array_merge($element['#parents'], array('text'));
		$element['text'] = array(
			'#type'				 => 'textfield',
			'#name'				 => array_shift($parents) . '['. implode('][', $parents) .']',
			'#autocomplete_path' => $element['#autocomplete_path'],
			'#description'		 => t('Enter the name of the organisation to start searching'),
			'#default_value'	 => empty($organisation) ? '' : $organisation->get_name(),
			'#attributes'		 => array(
				'class'			 => array(
					'form-select-organisation'
				),
				'id'			 => $element['#name']
			)
		);

		$parents = array_merge($element['#parents'], array('id'));
		$element['id'] = array(
			'#type'				 => 'hidden',
			'#name'				 => array_shift($parents) . '['. implode('][', $parents) .']',
			'#default_value'	 => $element_current_value,
			'#attributes'		 => array(
				'id'			 => $element['#name'] . '_id',
				'class'			 => array(
					'form-select-organisation-id'
				)
			)
		);
	}

	/**
	 * Processes the Auto Complete element for display.
	 */
	function organisation_select_process($element, &$form_state, $form)
	{
		organisation_select_normalise($element);

		return $element;
	}

	/**
	 * Processes the form input. Converts the input text into an Answer ID.
	 */
	function organisation_select_value_callback($element, $input = false, &$form_state)
	{
		if ($input === false)
		{
			//Get the value to display from the default value or the specified value
			if (isset($element['#value']))
			{
				return $element['#value'];
			}
			else if (isset($element['#default_value']))
			{
				return $element['#default_value'];
			}
			else
			{
				return null;
			}
		}

		if (is_array($input))
		{
			//We only care about the organisation ID. This branch is taken when we are
			//handling a form submittal.
			$input = $input['id'];
			if (!empty($input))
			{
				$input = intval($input);
			}
		}

		return $input;
	}

	function organisation_select_validate(&$element, &$form_state)
	{
		//Construct the organisation object.
		if (!empty($element['#value']) && !Organisation::exists($element['#value']))
		{
			form_set_error(implode('][', $element['text']['#parents']),
				t('Invalid @element selected', array('@element' => $element['#title'])));
		}
		else
		{
			//Otherwise, set the form element's value.
			$value = drupal_array_get_nested_value($form_state['input'],
				$element['#parents']);
			form_set_value($element, intval($value['id']), $form_state);
		}
	}
}
