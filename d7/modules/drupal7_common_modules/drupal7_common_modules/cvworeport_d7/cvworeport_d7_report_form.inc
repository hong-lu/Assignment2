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

function cvwo_report_form($form, &$form_state, $mode = 'add', $special = FALSE, $report_id = 0) {

	if (!empty($form_state['storage']['values'])) {
		$form_state['values'] = $form_state['storage']['values'];
	} else if ($mode != 'add' && $report_id != 0) {
		$values = cvwo_report_load($report_id);
		// reorder a bit
		$values['user_permission']['users'] = $values['user_permission'];
		if (isset($values['queries'])) {
			$values['query']['queries'] = $values['queries'];
			foreach($values['query']['queries'] as $query_id => $query) {
				$values['query']['queries'][$query_id]['value']['query_name'] = $query['query_name'];
				$values['query']['queries'][$query_id]['value']['special'] = $query['special'];
				$values['query']['queries'][$query_id]['value']['sql_statement'] = $query['sql_statement'];
			}
		}
		$form_state['values'] = $values;
		if ($form_state['values']['special'] != $special || $form_state['values']['status'] == STATUS_DISABLED) {
			drupal_set_message(t('Report was not found'), 'error');
			drupal_goto(CVWOREPORT_MODULE);
		}
	}	

	if ($report_id != 0) {
		$form['report_id'] = array(
			'#type' => 'value',
			'#value' => $report_id,
		);
	}
		
	$form['report_name'] = array(
		'#type' => 'textfield',
		'#title' => t('Report Name'),
		'#description' => t('Name of report'),
		'#size' => 30,
		'#required' => TRUE,
		'#default_value' => isset($form_state['values']['report_name']) ? $form_state['values']['report_name'] : '',
	);
	
	$form['description'] = array(
		'#type' => 'textfield',
		'#title' => t('Description of report'),
		'#description' => t('Describes what the report does (Optional)'),
		'#default_value' => isset($form_state['values']['description']) ? $form_state['values']['description'] : '',
	);
	
	$form['category_id'] = array(
		'#type' => 'select',
		'#title' => t('Category'),
		'#required' => TRUE,
		'#empty_value' => '',
		'#options' => get_report_categories(),
	);
	if (isset($form_state['values']['category_id'])) {
		$form['category_id']['#default_value'] = $form_state['values']['category_id'];
	};	
	
	$form['special'] = array(
		'#type' => 'value',
		'#value' => isset($form_state['values']['special']) ? $form_state['values']['special'] : $special,
	);
	
	$access_function = isset($form_state['values']['access_function']) ? $form_state['values']['access_function'] : '';
	// TODO: check here
        $access_function_options = cvwocodem2_getoptions(CVWOREPORT_MODULE, CVWO_REPORTS_ACCESS_QUESTION, $access_function);
	$form['access_function'] = array(
		'#type' => 'select',
		'#title' => t('Access function to use when accessing this report'),
		'#required' => TRUE,
		'#empty_value' => '',
		'#options' => $access_function_options,
	);
	if (isset($access_function))
		$form['access_function']['#default_value'] = $access_function;
	
	if ($special) {
		$form['report_text'] = array(
			'#type' => 'textarea',
			'#title' => t('Report text'),
			'#rows' => 20,
			'#description' => t('The text can be of any form, for example, a PHP code chunk, a HTML mix or a plain text string.'),
			'#default_value' => isset($form_state['values']['report_text']) ? $form_state['values']['report_text'] : '',
			'#required' => TRUE,
		);
		$form['drupal'] = array(
			'#type' => 'checkbox',
			'#title' => t('Please tick if you are intending to use drupal form'),
			'#default_value' => isset($form_state['values']['drupal']) ? $form_state['values']['drupal'] : '',
		);
	} else {
		$form['query'] = array(
			'#type' => 'fieldset',
			'#title' => t('Queries OR Special Elements'),
			'#tree' => TRUE,
			'#theme' => 'cvwo_report_query_form',
		);
		$form['query']['query_name'] = array(
			'#type' => 'textfield',
			'#title' => t('Query OR Special Element Name'),
			'#default_value' => isset($form_state['values']['query_name']) ? $form_state['values']['query_name'] : '',
		);
		$form['query']['special'] = array(
			'#type' => 'checkbox',
			'#title' => t('Please tick if it is a special element'),
			'#default_value' => isset($form_state['values']['special']) ? $form_state['values']['special'] : '',
		);
		$form['query']['sql_statement'] = array(
			'#type' => 'textarea',
			'#title' => t('SQL statement (Please enter it in Drupal format) OR Special Element Text'),
			'#default_value' => isset($form_state['values']['sql_statement']) ? $form_state['values']['sql_statement'] : '',
		);
		$form['query']['add'] = array(
			'#type' => 'submit',
			'#value' => t('Add'),
			'#submit' => array('add_query_to_report'),
		);
		
		if (isset($form_state['values']['query']['queries'])) {
			foreach($form_state['values']['query']['queries'] as $id => $query) {	
				$form['query']['queries'][$id]['query_name'] = array(
					'#type' => 'markup',
					'#markup' => $query['value']['query_name'],
				);
				$form['query']['queries'][$id]['sql_statement'] = array(
					'#type' => 'markup',
					'#markup' => $query['value']['sql_statement'],
				);
				$form['query']['queries'][$id]['special'] = array(
					'#type' => 'markup',
					'#markup' => ($query['value']['special'] == 1) ? 'Yes' : 'No',
				);
				$form['query']['queries'][$id]['remove'] = array(
					'#type' => 'submit', 
					'#value' => t('Delete'), 
					'#name' => $id, 
					'#submit' => array('delete_query_from_report')
				);
				$form['query']['queries'][$id]['up'] = array(
					'#type' => 'submit',
					'#value' => t('Up'),
					'#name' => $id,
					'#submit' => array('move_query_up'),
				);
				$form['query']['queries'][$id]['down'] = array(
					'#type' => 'submit',
					'#value' => t('Down'),
					'#name' => $id,
					'#submit' => array('move_query_down'),
				);
				$form['query']['queries'][$id]['value'] = array(
					'#type' => 'value',
					'#value' => $query['value'],
				);
			}
		}
	
	}
	
	$form['role_permission'] = array(
		'#type' => 'fieldset',
		'#title' => t('Role Permission'),
		'#tree' => TRUE,
		'#theme' => 'cvwo_report_roles_form',
	);
	
	$role_permission_options = array(
		CVWO_REPORT_NO_PERMISSION => t('None'),
		CVWO_REPORT_VIEW_ONLY_PERM => t('Show Only'),
		CVWO_REPORT_VIEW_AND_EDIT_PERM => t('Show & Edit'),
		CVWO_REPORT_VIEW_EDIT_AND_DELETE_PERM => t('Show, Edit & Delete')
	);
	$roles = get_user_roles();
	foreach($roles as $rid => $role_name) {
		$form['role_permission'][$rid]['rid'] = array(
			'#type' => 'markup',
			'#markup' => $rid,
		);
		$form['role_permission'][$rid]['role_name'] = array(
			'#type' => 'markup',
			'#markup' => $role_name,
		);

		$form['role_permission'][$rid]['permission'] = array(
			'#type' => 'radios',
			'#options' => $role_permission_options,
			'#default_value' => isset($form_state['values']['role_permission'][$rid]['permission']) ? $form_state['values']['role_permission'][$rid]['permission'] : CVWO_REPORT_NO_PERMISSION,
		);	
	}
	$form['role_permission']['reset'] = array(
		'#type' => 'submit',
		'#value' => t('Reset Role Permissions'),
		'#submit' => array('reset_role_permissions'),
	);
	
	$user_permission_options = array(
		CVWO_REPORT_VIEW_ONLY_PERM => t('Show Only'),
		CVWO_REPORT_VIEW_AND_EDIT_PERM => t('Show & Edit'),
		CVWO_REPORT_VIEW_EDIT_AND_DELETE_PERM => t('Show, Edit & Delete')
	);
	$form['user_permission'] = array(
		'#type' => 'fieldset',
		'#title' => t('User Permission'),
		'#tree' => TRUE,
		'#theme' => 'cvwo_report_users_form',
		'#description' => t('User permissions are intended for a situation in which the report needs not be handled by the whole set of users in the role, except some particular users in this role. The permission given in this section will override those given in the role in case duplicate permissions occur.
			The user who is now adding the report will automatically be given full permission.'),
	);
	
	$form['user_permission']['search'] = array(
		'#type' => 'textfield',
		'#title' => t('Search staff'),
		'#default_value' => isset($form_state['values']['user_permission']['search']) ? $form_state['values']['user_permission']['search'] : '',
		'#autocomplete_path' => CVWOREPORT_MODULE . '/cvwo_report_staff_nric_autocomplete',
	);
	
	$form['user_permission']['search_permission'] = array(
		'#type' => 'radios',
		'#options' => $user_permission_options,
		'#default_value' => isset($form_state['values']['user_permission']['search_permission']) ? $form_state['values']['user_permission']['search_permission'] : CVWO_REPORT_VIEW_ONLY_PERM,
	);
	
	$form['user_permission']['search_add'] = array(
		'#type' => 'submit',
		'#value' => t('Add staff'),
		'#submit' => array('add_staff_to_report'),
	);
	
	if (isset($form_state['values']['user_permission']['users'])) {
		foreach ($form_state['values']['user_permission']['users'] as $uid => $key) {
			$report_user = user_load($uid);
			$form['user_permission']['users'][$uid]['uid'] = array(
				'#type' => 'value',
				'#value' => $uid,
			);
			$form['user_permission']['users'][$uid]['username'] = array(
				'#type' => 'markup',
				'#markup' => get_drupal_user_name($uid),
			);
			$form['user_permission']['users'][$uid]['name'] = array(
				'#type' => 'markup',
				'#markup' => get_name_by_drupal_id($uid),
			);
//			$form['user_permission']['users'][$uid]['nric'] = array(
//				'#type' => 'markup',
//				'#markup' => $report_user->nric_fin,
//			);
                        
                        // take the super user's situation into consideration
                        $default_p = '';
                        if (isset($form_state['values']['user_permission']['users'][$uid]['permission'])) {
                                $default_p = $form_state['values']['user_permission']['users'][$uid]['permission'];
                        } else if (isset($form_state['values']['user_permission']['users'][$uid])){
                                $default_p = $form_state['values']['user_permission']['users'][$uid];
                        }
                        
			$form['user_permission']['users'][$uid]['permission'] = array(
				'#type' => 'radios',
				'#options' => $user_permission_options,
				'#default_value' => $default_p,
			);
			$form['user_permission']['users'][$uid]['delete'] = array(
				'#type' => 'submit',
				'#value' => t('Delete'),
				'#user' => $uid,
				'#submit' => array('delete_user_from_report'),
			);
		}
	}
	
	if ($mode == 'add') {
		$form['add_report'] = array(
			'#type' => 'submit',
			'#value' => t('Add new report'),
		);
	} else {
		$form['edit_report'] = array(
			'#type' => 'submit',
			'#value' => t('Edit report'),
		);
		if (cvwo_report_access(CVWO_REPORT_VIEW_EDIT_AND_DELETE_PERM, $report_id))
			$form['delete_report'] = array(
				'#type' => 'submit',
				'#value' => t('Delete report'),
			);
	}
	
	return $form;
}

function add_query_to_report($form, &$form_state) {
	$total_query = isset($form_state['values']['query']['queries']) ? count($form_state['values']['query']['queries']) : 0;
	$form_state['values']['query']['queries'][$total_query + 1] = array('value' => array(
		'query_name' => $form_state['values']['query']['query_name'],
		'special' => $form_state['values']['query']['special'],
		'sql_statement' => $form_state['values']['query']['sql_statement'],
	));
	unset($form_state['input']['query']['query_name']);
	unset($form_state['input']['query']['special']);
	unset($form_state['input']['query']['sql_statement']);
	
	$form_state['storage']['values'] = $form_state['values'];		
	$form_state['rebuild'] = TRUE;
}

function delete_query_from_report($form, &$form_state) {
	$query_to_delete = $form_state['triggering_element']['#name'];
	$total_query = count($form_state['values']['query']['queries']);
	
	for ($i = $query_to_delete; $i < $total_query; $i++) {
		$form_state['values']['query']['queries'][$i] = $form_state['values']['query']['queries'][$i+1];
	}
	unset($form_state['values']['query']['queries'][$total_query]);
	
	$form_state['storage']['values'] = $form_state['values'];
	$form_state['rebuild'] = TRUE;
}

function move_query_up($form, &$form_state) {
	$query_to_move_up = $form_state['triggering_element']['#name'];
	
	if ($query_to_move_up != 1) {
		$query = $form_state['values']['query']['queries'][$query_to_move_up];
		$form_state['values']['query']['queries'][$query_to_move_up] = $form_state['values']['query']['queries'][$query_to_move_up - 1];
		$form_state['values']['query']['queries'][$query_to_move_up - 1] = $query;
	}
	
	$form_state['storage']['values'] = $form_state['values'];
	$form_state['rebuild'] = TRUE;
}

function move_query_down($form, &$form_state) {
	$query_to_move_down = $form_state['triggering_element']['#name'];
	$total_queries = count($form_state['values']['query']['queries']);
	
	if ($query_to_move_down != $total_queries) {
		$query = $form_state['values']['query']['queries'][$query_to_move_down];
		$form_state['values']['query']['queries'][$query_to_move_down] = $form_state['values']['query']['queries'][$query_to_move_down + 1];
		$form_state['values']['query']['queries'][$query_to_move_down + 1] = $query;
	}
	
	
	$form_state['storage']['values'] = $form_state['values'];
	$form_state['rebuild'] = TRUE;
}

function reset_role_permissions($form, &$form_state) {
	foreach ($form_state['values']['role_permission'] as $rid => $key) {
		if ($rid != 'reset') {
			unset($form_state['input']['role_permission'][$rid]['permission']);
			unset($form_state['values']['role_permission'][$rid]['permission']);
		}
	}
	
	$form_state['storage']['values'] = $form_state['values'];
	$form_state['rebuild'] = TRUE;
}

function add_staff_to_report($form, &$form_state) {
	$report_user = get_user_profile_details_by_nric($form_state['values']['user_permission']['search']);
	
	$form_state['input']['user_permission']['users'][$report_user->uid]['permission'] = $form_state['input']['user_permission']['search_permission'];
	$form_state['input']['user_permission']['search_permission'] = '';
	$form_state['input']['user_permission']['search'] = '';
	$form_state['values']['user_permission']['users'][$report_user->uid]['permission'] = $form_state['values']['user_permission']['search_permission'];
	$form_state['values']['user_permission']['search'] = '';
	$form_state['values']['user_permission']['search_permission'] = '';
	
	$form_state['storage']['values'] = $form_state['values'];		
	$form_state['rebuild'] = TRUE;
}

function delete_user_from_report($form, &$form_state) {
	$button = $form_state['clicked_button'];
	$uid = $button['#user'];

	unset($form_state['values']['user_permission']['users'][$uid]);
	unset($form_state['input']['user_permission']['users'][$uid]);

	$form_state['storage']['values'] = $form_state['values'];		
	$form_state['rebuild'] = TRUE;	
}

function cvwo_report_form_validate($form, &$form_state) {
	$button = $form_state['clicked_button'];

	switch ($button['#value']) {
		case t('Add staff'):
			$report_user = get_user_profile_details_by_nric($form_state['values']['user_permission']['search']);
			if (!$report_user) {
				form_set_error('user_permission][search', t('User with NRIC was not found'));
			}
			if ($form_state['values']['user_permission']['search_permission'] == '') {
				form_set_error('user_permission][search_permission', t('Please indicate the permission for the user'));
			}
			break;
		case t('Add'):
			if ($form_state['values']['query']['query_name'] == '') {
				form_set_error('query][query_name', t('Please add query name'));
			}
			if ($form_state['values']['query']['sql_statement'] == '') {
				form_set_error('query][sql_statement', t('Please add SQL statement OR Special Element Text'));
			}
			break;
	}
}

function cvwo_report_form_submit($form, &$form_state) {
	$button = $form_state['clicked_button'];
	$error = FALSE;
	
	// Slight rearrangement of values due to tree-like structure
	$users = (isset($form_state['values']['user_permission']['users']) ? $form_state['values']['user_permission']['users'] : array());
	$values = $form_state['values'];
	$values['user_permission'] = $users;
	unset($values['role_permission']['reset']);
	if ($form_state['values']['special'] == STATUS_DISABLED) {
		$queries = (isset($form_state['values']['query']['queries']) ? $form_state['values']['query']['queries'] : array());
		$values['queries'] = $queries;
		foreach($values['queries'] as $query_id => $query) {
			$values['queries'][$query_id]['query_name'] = $query['value']['query_name'];
			$values['queries'][$query_id]['special'] = $query['value']['special'];
			$values['queries'][$query_id]['sql_statement'] = $query['value']['sql_statement'];
		}
	}
	switch ($button['#value']) {
		case t('Add new report'):
			if (cvwo_report_save($values)) {
				drupal_set_message(t('A new report has been successfully added'));
				$form_state['redirect'] = CVWOREPORT_MODULE;
			} else {
				drupal_set_message(t('Something went wrong with adding the report'), 'error');
				$error = TRUE;
			}
			break;
		case t('Edit report'):
			if (cvwo_report_save($values, $values['report_id'])) {
				drupal_set_message(t('The report has been saved successfully'));
				$form_state['redirect'] = CVWOREPORT_MODULE;
			} else {
				drupal_set_message(t('Something went wrong with updating the report'), 'error');
				$error = TRUE;
			} 
			break;
		case t('Delete report'): 
			$form_state['redirect'] = CVWOREPORT_MODULE.'/delete_report/' . $values['report_id'];
			break;
	}
	
	if ($error) {
		$form_state['rebuild'] = TRUE;
	} 
}

function theme_cvwo_report_roles_form($variables = NULL) {
	$form = $variables['form'];
	
	$header = array(
		array('data' => t('Role id')),
		array('data' => t('Role Name')),
		array('data' => t('Permission')),
	);
	$rows = array();
	foreach (element_children($form) as $id) {
		if ($id != 'reset') {
			$rows[] = array(
				 drupal_render($form[$id]['rid']),
				 drupal_render($form[$id]['role_name']),
				 drupal_render($form[$id]['permission']),
			);
		}
	}
	
	$output = theme('table', array('header' => $header,
									'rows' => $rows,
									'empty' => 'No roles found'));
	return $output . drupal_render_children($form);
}

function theme_cvwo_report_query_form($variables = NULL) {
	$form = $variables['form'];
	
	$output = '<table>';
	$output .= '<tr>';
	$output .= '<td>' . drupal_render($form['query_name']) . '</td>';
	$output .= '<td>' . drupal_render($form['special']) . '</td>';
	$output .= '</tr>';
	$output .= '<tr>';
	$output .= '<td colspan="2">' . drupal_render($form['sql_statement']) . '</td>';
	$output .= '</tr>';
	$output .= '<tr>';
	$output .= '<td colspan="2">'. drupal_render($form['add']) . '</td>';
	$output .= '</tr>';
	$output .= '</table>';
	
	$headers = array(
		array('data' => t('Query Name')),
		array('data' => t('Sql statement')),
		array('data' => t('Special')),
		array('data' => t('Options')),
	);
	$rows = array();
	if (!empty($form['queries'])) {
		foreach(element_children($form['queries']) as $id) {
			$query = &$form['queries'][$id];
			$row = array(
				drupal_render($query['query_name']),
				drupal_render($query['sql_statement']),
				drupal_render($query['special']),
				drupal_render($query['up']) . drupal_render($query['down']) . drupal_render($query['remove']),
			);
			$rows[] = $row;
		}
	}
	return $output . theme('table', array('header' => $headers,
										  'rows' => $rows,
										  'empty' => 'No queries')) . drupal_render_children($form);
}

function theme_cvwo_report_users_form($variables = NULL) {
	$form = $variables['form'];

	$output = '<table>';
	$output .= '<tr>';
	$output .= '<td>' . drupal_render($form['search']) . '</td>';
	$output .= '<td>' . drupal_render($form['search_permission']) . '</td>';
	$output .= '<td>' . drupal_render($form['search_add']) . '</td>';
	$output .= '</tr>';
	$output .= '</table>';
	
	if (isset($form['users'])) {	
		$header = array(
			array('data' => t('Username')),
			array('data' => t('Name')),
//			array('data' => t('NRIC')),
			array('data' => t('Permission')),
			array('data' => t('Option')),
		);
		
		$rows = array();
		foreach(element_children($form['users']) as $id) {
			$rows[] = array(
				drupal_render($form['users'][$id]['username']),
				drupal_render($form['users'][$id]['name']),
//				drupal_render($form['users'][$id]['nric']),
				drupal_render($form['users'][$id]['permission']),
				drupal_render($form['users'][$id]['delete']),
			);		
		}
		$output .= theme('table', array('header' => $header,
										'rows' => $rows,
										'empty' => 'No user found'));
	}
	
	return $output . drupal_render_children($form);
}
