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
 * Implementation of hook_form
 */
function cvwo_report_list_form($form, &$form_state, $requires_editing = FALSE){
	//drupal_add_js(drupal_get_path("module", CVWOREPORT_MODULE)."/cvworeport_d7.js");
	
	// get the report name
	$report_name = isset($form_state['values']['report_name']) ? $form_state['values']['report_name'] : '';
	$form = array();

	if (!get_report_setup()) {
		drupal_set_message(nl2br(t('The CVWO Report Module has not been localised.' . "\n" .
								   'Enter the localisation configuration in the !url.',
								   array(
									   '!url' => l(t('configuration page'), 'admin/config/cvwo/' . CVWOREPORT_MODULE)
								   ))), 'error');
	}
	$form['#theme'] = 'cvwo_report_list_form';
	$form['#requires_editing'] = $requires_editing;
	
	if (!$requires_editing) {
		// search box -------------------------------------------------------------
		$form['search']['report_name'] = array (
			'#id' => 'report_name',
			'#type' => 'textfield',
			'#title' => t('Search Report'),
			'#description' => t('Enter the name of the report to search for '),
			'#size' => 40,
			//'#autocomplete_path' => CVWOREPORT_MODULE.'/cvwo_report_name_autocomplete',
			'#default_value' => $report_name,
		);

		$form['search']['search_button'] = array (
			'#type' => 'submit',
			'#value' => t('Search'),
		);
		
		// now we generate the actual report list
		// Get the user ID (drupal ID) first
		global $user;
		
		$accessible_reports = cvworeport_get_accessible_reports($user->uid);
		$categorized_results = array();
		
		if (!empty($accessible_reports)) {
			$report_result = cvwo_select(CVWO_REPORT_TABLE, 'r')
					->condition('r.report_id', array_keys($accessible_reports))
					->condition('r.report_name', '%' . $report_name . '%', 'LIKE')
					->fields('r', array('report_id', 'report_name', 'weight', 'category_id', 'access_function', 'description', 'special', 'status'))
					->orderBy('weight', 'asc')
					->orderBy('category_id', 'asc')
					->execute();
			while ($data = $report_result->fetchAssoc()){
				$categorized_results[$data['category_id']][] = $data;
			}
		}
	} else {
		// just list all reports
		$categorized_results = array();
		$report_result = cvwo_select(CVWO_REPORT_TABLE, 'r')
				->condition('r.status', STATUS_ENABLED)
				->fields('r', array('report_id', 'report_name', 'weight', 'category_id', 'access_function', 'description', 'special', 'status'))
				->orderBy('weight', 'asc')
				->orderBy('category_id', 'asc')
				->execute();
		while ($data = $report_result->fetchAssoc()) {
			$categorized_results[$data['category_id']][] = $data;
		}
	}
	$form['rows']['#tree'] = TRUE;
	
	$categories = get_report_categories();
	
	foreach($categories as $cat_id => $cat){
		$form['rows'][$cat_id] = array();
		if (isset($categorized_results[$cat_id])){
			foreach($categorized_results[$cat_id] as $result){
				$data = array(
					'report_id' => $result['report_id'],
					'weight' => $result['weight'],
					'category_id' => $result['category_id'],
					'report_name' => $result['report_name'],
					'description' => $result['description'],
					'special' => $result['special'],
					'permission' => isset($accessible_reports[$result['report_id']]) ? $accessible_reports[$result['report_id']] : CVWO_REPORT_NO_PERMISSION,
				);

				$form['rows'][$cat_id][$result['report_id']]['data'] = array(
					'#type' => 'value',
					'#value' => $data,
				);
				if (!$requires_editing) {
					$form['rows'][$cat_id][$result['report_id']]['options-'.$result['report_id']] = array(
						'#type' => 'value',
						'#value' => format_permissions(isset($accessible_reports[$result['report_id']]) ? $accessible_reports[$result['report_id']] : CVWO_REPORT_NO_PERMISSION, $result['report_id'], $result['special']),
					);
				} else {
					// create the weight form element
					$form['rows'][$cat_id][$result['report_id']]['weight-'.$result['report_id']] = array(
						'#type' => 'weight',
						'#size' => 5,
						'#default_value' => $result['weight'],
						'#attributes' => array('class' => array('cat-weight cat-weight-'.$cat_id)),
					);

					// create the region form element
					$form['rows'][$cat_id][$result['report_id']]['region-'.$result['report_id']] = array(
						'#type' => 'select',
						'#default_value' => $result['category_id'],
						'#options' => $categories,
						'#attributes' => array('class' => array('cat-region cat-region-'.$cat_id)),
					);
				}
			}
		} else {
			$form['rows'][$cat_id]['empty'] = array(
				'#type' => 'value',
				'#value' => TRUE,
			);
		}
	}
	if ($requires_editing)
		$form['reorder'] = array(
			'#type' => 'submit',
			'#value' => t('Save Changes'),	
		);


	return $form;
}

/**
 * hook submit for cvwo_report_list_form
 */
function cvwo_report_list_form_submit($form, &$form_state){
	$button = $form_state['clicked_button'];
	$values = $form_state['values'];
	
	switch ($button['#value']) {
		case t('Save Changes'):
			if (cvwo_report_save_weights_and_categories($form_state['values']['rows'])) {
				drupal_set_message(t('Weights and categories have been saved'));
			} else {
				drupal_set_message(t('Error in saving weights and categories'), 'error');
				$form_state['rebuild'] = TRUE;
			}
			break;
		case t('Search'):
			$form_state['rebuild'] = TRUE;
			break;
	}
}

/**
 * theme function for listing
 */
function theme_cvwo_report_list_form($variables){
	$form = $variables['form'];

	$output = '';
	if (isset($form['search'])) $output .= theme('table', array('rows' => array(array(drupal_render($form['search']['report_name']), drupal_render($form['search']['search_button'])))));
	
	$header = array(
		array('data' => t('Report Name')),
		array('data' => t('Description')),
	);
	
	if ($form['#requires_editing']) {
		$header[] = array('data' => t('Weight'));
		$header[] = array('data' => t('Category'));
	} else {
		$header[] = array('data' => t('Options'));
	}
		
	$table_rows = array();
	$categories = get_report_categories_detail();
	foreach($categories as $category) {
		$category_row = array(
			'<b><em>'. $category['category_name'] . '</em></b>',
			'',
			'',
		);
		if ($form['#requires_editing'])
			$category_row[] = '';
		$table_rows[] = $category_row;
		foreach(element_children($form['rows'][$category['category_id']]) as $report_id) {
			if ($report_id != 'empty') {
				$report = $form['rows'][$category['category_id']][$report_id];
				$report_id = $report['data']['#value']['report_id'];
				$report_row = array(
					$report['data']['#value']['report_name'],
					$report['data']['#value']['description'],
				);
				if (!$form['#requires_editing']) {
					$report_row[] = $form['rows'][$category['category_id']][$report_id]['options-'.$report_id]['#value'];
				} else {
					$report_row[] = drupal_render($form['rows'][$category['category_id']][$report_id]['weight-'.$report_id]);
					$report_row[] = drupal_render($form['rows'][$category['category_id']][$report_id]['region-'.$report_id]);
				}
				$table_rows[] = $report_row;
			}
		}
	}
	$output .= theme('table', array('header' => $header,
									'rows' => $table_rows,
									'empty' => 'No reports found'));
									
	return $output.drupal_render_children($form);
}

/**
 * Define the permssions according to the data
 * in the table and returns
 */
function format_permissions($permission, $report_id, $special = FALSE){
	$options = array();
	$permission_array = array(
		array('text' => 'Show', 'link' => CVWOREPORT_MODULE . '/display/' . $report_id, 'options' => $options),
		array('text' => 'Edit', 'link' => CVWOREPORT_MODULE . ($special ? '/editspecial/' : '/edit/') . $report_id, 'options' => $options),
		array('text' => 'Delete', 'link' => CVWOREPORT_MODULE . '/delete_report/' . $report_id, 'options' => $options),
	);
	
	$checked_permission = array_map(create_function('$permission', 'return la($permission["text"], $permission["link"], $permission["options"], TRUE);'), $permission_array);
	$filtered_permission = array_filter($checked_permission, "check_not_empty_string"); 
	return implode(' - ', $filtered_permission);
}

function check_not_empty_string($string) {
	return $string != '';
}

/**
 * hook_form for deleting a particular report
 */
function cvwo_report_delete_report_form($form, &$form_state, $report_id){

	$report = cvwo_report_load($report_id);
	
	$form['report_id'] = array(
		'#type' => 'hidden',
		'#value' => $report_id,
	);
	
	$form['report_name'] = array(
		'#type' => 'hidden',
		'#value' => $report['report_name'],
	);

	return confirm_form(
		$form,
		'Are you sure you want to delete the report '.$report['report_name'].'?',
		CVWOREPORT_MODULE.'/list',
		'',
		'Delete',
		'Cancel',
		'cvwo_report_delete_report_form');
}


/**
 * hook_submit for deleting a particular report
 */
function cvwo_report_delete_report_form_submit($form, &$form_state){
	if (cvwo_report_delete($form_state['values']['report_id'])) {
		drupal_set_message(t('Report was successfully deleted'));
		$form_state['redirect'] = CVWOREPORT_MODULE;
	} else {
		drupal_set_message(t('Report could not be deleted'), 'error');
		$form_state['rebuild'] = TRUE;
	}
}