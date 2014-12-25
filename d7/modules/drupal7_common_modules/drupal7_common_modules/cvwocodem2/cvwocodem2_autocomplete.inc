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
 * The CodeM2 autocompletion text field.
 *
 * This will create a Drupal text field with autocompletion support, backed by
 * CodeM answers. If no answers match, upon submittal, a new entry will be
 * created.
 *
 * For the form field, use #type = 'cvwocodem2_autocomplete'.
 *
 * Properties:
 * #question_id: The QID to use for the question. Specify this, or use
 *               #question and #module
 * #question: These are used in concert to find the QID for display.
 * #module
 * #value: The AID the user selected.
 * #default_value: The AID to show when no values are used.
 *
 * All other text_field properties can also be used.
 */

function theme_cvwocodem2_autocomplete($variables)
{
	return theme_textfield($variables);
}

/**
 * Normalises the autocomplete element.
 * 
 * @param array $element The element to normalise.
 */
function cvwocodem2_autocomplete_normalise_element(&$element)
{
	//Get the question ID, if #question was specified instead.
	if (!isset($element['#question_id']))
	{
		if (!isset($element['#question']) || !isset($element['#module']))
		{
			//Can't do anything about it. The programmer gave us insufficient
			//information
			$element['#question_id'] = -1;
		}
		else
		{
			//Look up our database.
			$element['#question_id'] =
				cvwocodem2_getqid($element['#module'], $element['#question']);
		}
	}

	//Resolve the #value.
	if (isset($element['#value']))
	{
		$element['#value'] = cvwocodem2_getanstext($element['#value']);
	}
}

/**
 * Processes the Auto Complete element for display.
 */
function cvwocodem2_autocomplete_element_process($element, &$form_state, $form)
{
	cvwocodem2_autocomplete_normalise_element($element);

	$element['#autocomplete_path'] = sprintf('%s/autocomplete/%d',
		CVWOCODEM2_MODULE, $element['#question_id']);

	if (isset($element['#element_validate']))
	{
		array_push($element['#element_validate'], 'cvwocodem2_autocomplete_element_value_validate');
	}
	else
	{
		$element['#element_validate'] = array('cvwocodem2_autocomplete_element_value_validate');
	}

	return $element;
}

/**
 * Processes the form input. Converts the input text into an Answer ID.
 */
function cvwocodem2_autocomplete_element_value_callback($element, $input = false, &$form_state)
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
			return -1;
		}
	}

	cvwocodem2_autocomplete_normalise_element($element);
	$aid = cvwocodem2_getaid($element['#question_id'], $input);

	//See if we need to add a new option.
	if ($aid === false)
	{
		$aid = cvwocodem2_addoption($element['#question_id'], $input, $input, 0, -1);
	}

	return $aid;
}

function cvwocodem2_autocomplete_element_value_validate($element, &$form_state)
{
	$input_exists	 = NULL;
	$input			 = drupal_array_get_nested_value($form_state['values'], $element['#parents'], $input_exists);

	if (!is_numeric($input))
	{
		cvwocodem2_autocomplete_normalise_element($element);
		$input = cvwocodem2_getaid($element['#question_id'], $input);
	}

	form_set_value($element, $input, $form_state);
}

function cvwocodem2_autocomplete_callback($qid, $input)
{
	$query = db_select(CVWOCODEM2_OPTIONS_TABLE)
		->condition('qid', $qid)
		->condition('anstext', sprintf('%%%s%%', $input), 'LIKE')
		->fields(CVWOCODEM2_OPTIONS_TABLE, array('anstext'))
		->execute();
	$matches = $query->fetchCol();

	foreach ($matches as &$match)
	{
		$match = check_plain($match);
	}
	$result = empty($matches) ? array() :array_combine($matches, $matches);

	drupal_json_output($result);
}
