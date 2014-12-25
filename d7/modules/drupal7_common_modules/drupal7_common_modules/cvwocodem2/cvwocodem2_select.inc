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
 * The CodeM2 select field.
 *
 * This will create a Drupal select field, backed by the CodeM answers.
 *
 * For the form field, use #type = 'cvwocodem2_select'.
 *
 * Properties:
 * #question_id: The QID to use for the question. Specify this, or use
 *               #question and #module
 * #question: These are used in concert to find the QID for display.
 * #module
 * #value: The AID the user selected.
 * #default_value: The AID to show when no values are used.
 * #options: Do not use: we will trash these.
 *
 * All other text_field properties can also be used.
 */

function theme_cvwocodem2_select($variables)
{
	return theme_select($variables);
}

/**
 * Normalises the autocomplete element.
 *
 * @param array $element The element to normalise.
 */
function cvwocodem2_select_normalise_element(&$element)
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

	//Get the #options
	$element['#options'] = cvwocodem2_getoptions_for_qid($element['#question_id'],
		0, !$element['#required']);

	//Set the default if we are using option 0 (see the install script) or null
	//and the field is required (graceful fallback)
	if ((!isset($element['#default_value']) && $element['#required']) ||
		(isset($element['#default_value']) && $element['#default_value'] === 0))
	{
		$element['#default_value'] = cvwocodem2_getdefaultopt($element['#question_id']);
	}
}

/**
 * Processes the Select element for display.
 */
function cvwocodem2_select_element_process($element, &$form_state, $form)
{
	cvwocodem2_select_normalise_element($element);

	return $element;
}

/**
 * Processes the form input. Converts the input text into an Answer ID.
 */
function cvwocodem2_select_element_value_callback($element, $input = false, &$form_state)
{
	if ($input === false)
	{
		return null;
	}

	cvwocodem2_select_normalise_element($element);
	return empty($input) ? null : $input;
}
