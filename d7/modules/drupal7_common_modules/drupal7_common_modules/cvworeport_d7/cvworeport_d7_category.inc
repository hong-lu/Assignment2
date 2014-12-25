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
 * form for managing report categories
 */
function cvwo_report_manage_category_form($form, &$form_state){
	
	$form = array();

	$categories = get_report_categories_detail();
	
	// need to set to true so that form_state['values'] do not overlap each other
	$form['category_rows']['#tree'] = TRUE;
	
	foreach($categories as $category){

		$form['category_rows'][$category['category_id']]['id'] = array(
			'#type' => 'hidden',
			'#value' => $category['category_id'],
		);

		$form['category_rows'][$category['category_id']]['name'] = array(
			'#type' => 'textfield',
			'#default_value' => $category['category_name'],
			'#size' => 30,
			'#maxlength' => 255,
		);

		$form['category_rows'][$category['category_id']]['weight'] = array(
			'#type' => 'textfield',
			'#default_value' => $category['weight'],
			'#size' => 5,
			'#maxlength' => 5,
		);
		
		$form['category_rows'][$category['category_id']]['options'] = array(
			'#type' => 'markup',
			'#markup' => $category['deletable'] == STATUS_ENABLED ? l('Delete', CVWOREPORT_MODULE.'/delete_category/' . $category['category_id'] . '/' . $category['category_name']) : 'Cannot delete default category',
		);

	}

	$form['new_category'] = array(
        '#type' => 'fieldset',
        '#title' => 'Add New Category',
		'#theme' => 'cvwo_report_add_category_form',
        '#collapsible'  => TRUE,
        '#collapsed' => TRUE,
	);

	$form['new_category']['new_name'] = array(
		'#type' => 'textfield',
		'#title' => 'Category Name',
		'#maxlength' => 255,
		'#size' => 30,
		'#default_value' => isset($form_state['values']['new_name']) ? $form_state['values']['new_name'] : '',
	);

	$form['new_category']['new_weight'] = array(
		'#type' => 'textfield',
		'#title' => 'Weight',
		'#maxlength' => 5,
		'#size' => 5,
		'#default_value' => isset($form_state['values']['new_weight']) ? $form_state['values']['new_weight'] : '',
	);
	
	$form['new_category']['add'] = array(
		'#type' => 'submit',
		'#value' => t('Add'),
	);

	$form['update'] = array(
		'#type' => 'submit',
		'#value' => t('Save Changes'),
	);

	return $form;
}

/**
 *	hook validation for cvwo_report_manage_category_form
 */
function cvwo_report_manage_category_form_validate($form, &$form_state) {
	$values = $form_state['values'];
	switch ($values['op']) {
		case t('Save Changes'):
			foreach(element_children($form['category_rows']) as $id) {
				if (!is_numeric($values['category_rows'][$id]['weight']))
					form_set_error('category_rows]['.$id.'][weight', t('Weights must be numeric.'));
			}
			break;
		case t('Add'):
			if (!is_numeric($values['new_weight'])) {
				form_set_error('new_weight', t('Weights must be numeric.'));
			}
			break;
	} 
}
/**
 * hook submit for cvwo_report_manage_category_form
 */
function cvwo_report_manage_category_form_submit($form, &$form_state){
	$values = $form_state['values'];
	$error = FALSE;
	
	switch ($values['op']) {
		case t('Save Changes'):
			$message = 'Existing categories have been edited';
			$txn = cvwo_transaction($message, CVWO_REPORT_TAG_EDIT, CVWOREPORT_MODULE);
			try {
				foreach(element_children($form['category_rows']) as $id) {
					cvwo_update(CVWO_REPORT_CATEGORY_TABLE)
						->fields(array(
						'category_name' => $values['category_rows'][$id]['name'],
						'weight' => $values['category_rows'][$id]['weight'],
						))
						->condition('category_id', $id, '=')
						->execute();
				}
			} catch (Exception $e) {
				$txn->rollback();
				watchdog_exception(CVWOREPORT_MODULE, $e);
				$error = TRUE;
			}
			if ($error) {
				drupal_set_message(t('There were some errors in updating the existing categories.'), 'error');
			} else {
				drupal_set_message(t('Changes have been successfully saved.'));
			}
			break;
		case t('Add'):
			$message = 'A new category: %name has been added';
			$result = cvwo_insert(CVWO_REPORT_CATEGORY_TABLE)
					->fields(array(
						'category_name' => $values['new_name'],
						'weight' => $values['new_weight'],
						'deletable' => STATUS_ENABLED,
					))
					->execute_logged($message, CVWO_REPORT_TAG_ADD, CVWOREPORT_MODULE, array('%name' => $values['new_name']));
			if ($result) {
				drupal_set_message(t('A category has been successfully added.'));
			} else {
				drupal_set_message(t('There was some error in adding the category.'), 'error');
				$error = TRUE;
			} 
			break;	
	}
	
	if ($error) {
		$form_state['rebuild'] = TRUE;
	} else {
		$form_state['redirect'] = CVWOREPORT_MODULE.'/manage_category';
	}

}


/**
 * theming function for cvwo_report_manage_category_form
 */
function theme_cvwo_report_manage_category_form($variables = NULL){
	$form = $variables['form'];

	$output = '';
	
	$header = array(
		array('data' => t('Name')),
		array('data' => t('Weight')),
		array('data' => t('Options')),
	);
	
	$rows = array();
	
	foreach(element_children($form['category_rows']) as $id) {
		$row = &$form['category_rows'][$id];
		$rows[] = array(
			drupal_render($row['name']),
			drupal_render($row['weight']),
			drupal_render($row['options']),
		);
	}
	
	$output .= theme('table', array('header' => $header,
									'rows' => $rows,
									'empty' => 'No categories found'));
	$output .= drupal_render($form['update']);
	$output .= drupal_render($form['new_category']);	
	$output .= theme('status_messages');
	return $output.drupal_render_children($form);
}

/**
 * theming function for add category fieldset
 */
function theme_cvwo_report_add_category_form($variables = NULL){

	$form = $variables['form'];

	$output = '';
	$output .= '<table>';
	$output .= '<tr>';
	$output .= '<td>';
	$output .= drupal_render($form['new_name']);
	$output .= '</td><td>';
	$output .= drupal_render($form['new_weight']);
	$output .= '</td><td>';
	$output .= drupal_render($form['add']);
	$output .= '</td>';
	$output .= '</tr>';
	$output .= '</table>';

	return $output.drupal_render_children($form);
}

/**
 * hook_form for deleting a particular category
 */
function cvwo_report_delete_category_form($form, &$form_state, $category_id, $category_name){

	$form = array();

	$form['category_id'] = array(
		'#type' => 'hidden',
		'#value' => $category_id,
	);
	
	$form['category_name'] = array(
		'#type' => 'hidden',
		'#value' => $category_name,
	);

	return confirm_form(
		$form,
		'Are you sure you want to delete the category '.$category_name.'?',
		CVWOREPORT_MODULE.'/manage_category',
		'All reports in this category will be put into the default category. This operation cannot be undone.',
		'Delete',
		'Cancel',
		'cvwo_report_delete_category_form');
}


/**
 * hook submit for deleting a particular category
 */
function cvwo_report_delete_category_form_submit($form, &$form_state){

	$message = 'Category %name has been successfully deleted.';
	$error = FALSE;
	$txn = cvwo_transaction($message, CVWO_REPORT_TAG_DELETE, CVWOREPORT_MODULE, array('%name' => $form_state['values']['category_name']));
	try {
		cvwo_delete(CVWO_REPORT_CATEGORY_TABLE)
			->condition('category_id', $form_state['values']['category_id'], '=')
			->execute();
		cvwo_update(CVWO_REPORT_TABLE)
			->fields(array(
				'category_id' => CVWO_REPORT_DEFAULT_CATEGORY_ID,
			))
			->condition('category_id', $form_state['values']['category_id'], '=')
			->execute();
	} catch (Exception $e) {
		$txn->rollback();
		watchdog_exception(CVWOREPORT_MODULE, $e);
		$error = TRUE;
	}
	if ($error) {
		drupal_set_message(t('There were some errors in deleting the %name.', array('%name' => $form_state['values']['category_name'])), 'error');
	} else {
		drupal_set_message(t('Changes have been successfully saved.'));
	}
	
	$form_state['redirect'] = CVWOREPORT_MODULE.'/manage_category';
}
