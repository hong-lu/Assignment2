<?php
/*
  Copyright (c) 2011-2012 Computing for Volunteer Welfare Organisations (CVWO)
  National University of Singapore
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation
  files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy,
  modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:
   
  1. The above copyright notice and this permission notice shall be included in all copies or substantial portions of the
  Software.
   
  2. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
  WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
// $Id$

/**
 * Localisation forms
 */
function cvwo_report_setup_form($form, &$form_state){
	
	$result = get_report_setup();

	$form['menu_name'] = array(
  		'#type' => 'textfield', 
  		'#title' => t('Customized Menu Name'), 
  		'#default_value' => $result ? $result['menu_name'] : 'Reports',
  		'#size' => 32, 
  		'#maxlength' => 255,
		'#required' => TRUE,
  		'#description' => 'You can customize the name of the module displayed in the main navigation menu. You will need to clear cache after changing this value',
	);
	
	$form['localisation_functions'] = array(
		'#type' => 'fieldset',
		'#title' => t('Localisation Functions'),
		'#collapsible' => FALSE,
	);

	$form['localisation_functions']['nric_autocomplete'] = array(
		'#type' => 'textarea',
		'#id' => 'nric_autocomplete',
		'#name' => 'nric_autocomplete',
		'#title' => t('Autocomplete function for staff\'s nric number'),
		'#rows' => 10,
		'#required' => TRUE,
		'#description' => t("Input texts will be used to create a function of the form: function nric_autocomplete(\$string)" .
							" ,should return an indexed array A of arrays B[] with B exactly of the form ['nric', 'name']"),
		'#default_value' => $result ? $result['nric_autocomplete'] : '',
	);
	
	$form['submit'] = array(
		'#type' => 'submit',
		'#id' => '_add_edit',
		'#name' => '_add_edit',
		'#value' => ($result ? t('Edit') : t('Add')),
	);
	
	return $form;
}

/**
 * Submit hook for the localisation forms
 * 
 */
function cvwo_report_setup_form_submit($form, &$form_state){
	$result = get_report_setup();
	$message = $result ? 'You have successfully updated the localisation information.' 
							   : 'You have successfully added the localisation information.';
	drupal_set_message($message);
	if ($result){
		cvwo_update(CVWO_REPORT_SETUP_TABLE)
			->fields(array(
				'menu_name' => $form_state['values']['menu_name'],
				'nric_autocomplete' => $form_state['values']['nric_autocomplete'],
			))
			->condition('setup_id', CVWO_REPORT_DEFAULT_SETUP_ID, '=')
			->execute();
	} else {
		cvwo_insert(CVWO_REPORT_SETUP_TABLE)
			->fields(array(
				'setup_id' => CVWO_REPORT_DEFAULT_SETUP_ID,
				'menu_name' => $form_state['values']['menu_name'],
				'nric_autocomplete' => $form_state['values']['nric_autocomplete'],
			))
			->execute();
	}
}
