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
 * The CodeM2 tag field.
 *
 * This will create a Drupal text field which will be used together with jQuery
 * tag-it, to allow adding tags. This will degrade gracefully when the browser
 * has JavaScript turned off; users can still input using commas as separators.
 *
 * For the form field, use #type = 'cvwocodem2_tag'.
 *
 * Properties:
 * #question_id: The QID to use for the question. Specify this, or use
 *               #question and #module
 * #question: These are used in concert to find the QID for display.
 * #module
 * #value: An array of AIDs the user selected.
 * #default_value: An array of AIDs to show when no values are used.
 *
 * All other textfield properties can also be used.
 */

function theme_cvwocodem2_tag($variables)
{
	return theme_textfield($variables);
}

/**
 * Normalises the tag element.
 * 
 * @param array $element The element to normalise.
 */
function cvwocodem2_tag_normalise_element(&$element)
{
	//We need the tagit library
	drupal_add_library('tag_it', 'tag_it');

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
	
	//Define our element class so we can convert the element to a tag selector.
	if (!isset($element['#attributes']))
	{
		$element['#attributes'] = array();
	}
	$element['#attributes'] = array_merge_recursive(
		$element['#attributes'],
		array(
			'class' => array(
				'form-item-cvwocodem2-tag'
			)
		));
	
	//No Drupal autocomplete.
	$element['#autocomplete_path'] = null;
	
	//Prepare the settings to be spit out for this element.
	$tag_list = cvwocodem2_getoptions_for_qid($element['#question_id'], true, false);
	$settings = array('cvwocodem2_tag' => array(
		$element['#id'] => array(
			'availableTags' => array_values($tag_list)
		)
	));
	drupal_add_js($settings, 'setting');

	//Convert our answer IDs into answer texts
	if (isset($element['#value']))
	{
		$element['#value']	= (array)$element['#value'];
		$element['#value'] = implode(', ',
			array_map(function($tag) use (&$tag_list) {
				return $tag_list[$tag];
			}, $element['#value']));
	}
}

/**
 * Processes the tag element for display.
 */
function cvwocodem2_tag_element_process($element, &$form_state, $form)
{
	cvwocodem2_tag_normalise_element($element);

	if (isset($element['#element_validate']))
	{
		array_push($element['#element_validate'], 'cvwocodem2_tag_element_value_validate');
	}
	else
	{
		$element['#element_validate'] = array('cvwocodem2_tag_element_value_validate');
	}

	return $element;
}

/**
 * Processes the form input. Converts the input text into an array of Answer IDs.
 */
function cvwocodem2_tag_element_value_callback($element, $input = false, &$form_state)
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

	cvwocodem2_tag_normalise_element($element);
	$selected_tags = empty($input) ? array() : explode(',', $input);
	
	//Convert the tags into an array of answer IDs. First, make sure we add new answer IDs.
	$tag_list = cvwocodem2_getoptions_for_qid($element['#question_id'], true, false);

	//Add new options
	$difference = array_diff($selected_tags, $tag_list);
	$tag_list = array_flip($tag_list);
	foreach ($difference as $new)
	{
		$tag_list[$new] = cvwocodem2_addoption($element['#question_id'], $new,
			$new, 0, -1);
	}
	
	//Map all the tags to AIDs.
	$selected_tags = array_map(function($value) use (&$tag_list) {
			return $tag_list[$value];
		}, $selected_tags);

	return $selected_tags;
}

function cvwocodem2_tag_element_value_validate($element, &$form_state)
{
	$input_exists	 = NULL;
	$input			 = drupal_array_get_nested_value($form_state['values'], $element['#parents'], $input_exists);

	if (!is_array($input))
	{
		$input = cvwocodem2_tag_element_value_callback($element, $input, $form_state);
	}

	form_set_value($element, $input, $form_state);
}
