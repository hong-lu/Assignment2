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
 * @file
 * Audit page include file for the cvwobase_d7 module.
 */

require_once drupal_get_path('module', CVWOBASE_MODULE).'/cvwobase_d7_audit_api.php';

/**
 * Form builder for the audit entries form.
 *
 * @param array $form
 * @param array &$form_state
 * @param string $module
 * @param int timestamp_from
 * @param int $timestamp_to
 * @param string $username
 * @param string $...
 *  Tags to filter by; if none specified, will include all tags
 *
 * @see cvwobase_d7_audit_form_validate()
 * @see cvwobase_d7_audit_form_submit()
 * @ingroup forms
 */
function cvwobase_d7_audit_form($form, &$form_state, $module = NULL, $timestamp_from = NULL, $timestamp_to = NULL, $username = NULL) {
  $filter_tags = array_slice(func_get_args(), 6);
  
  $modules = array_map(create_function('$info', 'return $info["name"];'), array_intersect_key(system_get_info('module'), array_flip(cvwobase_get_audit_modules())));
  $tags = cvwobase_get_audit_tags();
  
  // Delete Confirmation
  if (isset($form_state['values']) && $form_state['values']['op'] == t(CVWOBASE_AUDIT_BTN_DELETE) && count($deleting = array_filter($form_state['values']['delete_records'])) > 0) {
    return confirm_form(array(
      'cfm_deleting' => array(
        '#type' => 'value',
        '#value' => $deleting
      )),
      format_plural(count($deleting), 'Are you sure you wish to delete 1 record?', 'Are you sure you wish to delete @count records?'),
      'audit/'.implode('/', $form_state['build_info']['args'])
    );
  }
  
  $form['module_name'] = array(
    '#type' => 'select',
    '#title' => t('Module Name'),
    '#options' => $modules,
    '#empty_option' => t('All Modules'),
    '#default_value' => $module,
    '#description' => t('Search by Module Name')
  );
  $form['adv'] = array(
    '#type' => 'fieldset',
    '#title' => t('Advanced Search'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#attributes' => array(
      'class' => array(
        'no-print'	
      )
    )
  );
  $form['adv']['tag_fs'] = array(
    '#type' => 'fieldset',
    '#title' => t('Filter by Tags'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#description' => t('If none are selected, entries with any Tag will be shown')
  );
  $form['adv']['tag_fs']['tags'] = array(
    '#type' => 'checkboxes',
    '#options' => empty($tags) ? array() : array_combine($tags, $tags),
    '#default_value' => $filter_tags
  );
  $form['adv']['from_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('Only entries after and on'),
    '#date_format' => 'd/m/Y',
    '#date_year_range' => '-20:+0',
    '#default_value' => empty($timestamp_from) ? NULL : date('Y-m-d H:i:s', $timestamp_from)
  );
  $form['adv']['to_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('Only entries before and on'),
    '#date_format' => 'd/m/Y',
    '#date_year_range' => '-20:+0',
    '#default_value' => empty($timestamp_to) ? NULL : date('Y-m-d H:i:s', $timestamp_to)
  );
  $form['adv']['username'] = array(
    '#type' => 'textfield',
    '#title' => t('User Name'),
    '#autocomplete_path' => 'user/autocomplete',
    '#default_value' => $username
  );
  $form['view'] = array(
    '#type' => 'button',
    '#value' => t(CVWOBASE_AUDIT_BTN_VIEW),
  );

  // The records themselves
  $query = db_select(CVWO_AUDIT_TABLE, 'a')->extend('PagerDefault')->limit(variable_get(PAGELIMIT_CVWOAUDIT, PAGELIMIT_CVWOAUDIT_DEFAULT))
            ->fields('a', array('id', 'module_name', 'tag', 'message', 'variables', 'timestamp', 'uid'))->orderBy('timestamp', 'DESC')->orderBy('id');
  $users_alias = $query->innerJoin('users', 'u', 'a.uid = %alias.uid');
  
  if (!empty($module))
    $query->condition('module_name', $module);
  if (!empty($filter_tags)) {
    $form['adv']['#collapsed'] = $form['adv']['tag_fs']['#collapsed'] = FALSE;
    $query->condition('tag', $filter_tags);
  }
  if (!empty($timestamp_from)) {
    $form['adv']['#collapsed'] = FALSE;
    $query->condition('timestamp', $timestamp_from, '>=');
  }
  if (!empty($timestamp_to)) {
    $form['adv']['#collapsed'] = FALSE;
    $query->condition('timestamp', $timestamp_to, '<=');
  }
  if (!empty($username)) {
    $form['adv']['#collapsed'] = FALSE;
    $query->condition($users_alias.'.name', $username);
  }
  
  $records = $query->execute();
  $header = array(t('ID'), t('Module Name'), t('Tag'), t('Message'), t('Date'), t('User'));
  $options = array();
  foreach ($records as $record)
    $options[$record->id] = array($record->id, isset($modules[$record->module_name]) ? $modules[$record->module_name] : $record->module_name, $record->tag,
                                  $record->variables === 'N;' ? $record->message : t($record->message, unserialize($record->variables)),
                                  date('d/m/Y H:i', $record->timestamp), theme('username', array('account' => user_load($record->uid))));
  
  $form['delete_records'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('There are no records to be displayed'),
    '#js_select' => TRUE
  );
  $form['pager'] = array(
    '#theme' => 'pager'
  );
  if (!empty($options)) {
    $form['actions'] = array('#type' => 'actions');
    $form['actions']['delete'] = array(
      '#type' => 'button',
      '#value' => t(CVWOBASE_AUDIT_BTN_DELETE),
      '#access' => user_access(CVWOBASE_D7_ADMIN_AUDIT_PERM)
    );
    $form['actions']['export'] = array(
      '#type' => 'button',
      '#value' => t(CVWOBASE_AUDIT_BTN_EXPORT)
    );
  }
  return $form;
}

/**
 * Form validation handler for cvwobase_d7_audit_form().
 *
 * @see cvwobase_d7_audit_form_submit()
 */
function cvwobase_d7_audit_form_validate($form, &$form_state) {
  if ($form_state['triggering_element']['#value'] == t(CVWOBASE_AUDIT_BTN_VIEW)) {
    $values = &$form_state['values'];
    $timestamp_from = empty($values['from_date']) ? '' : strtotime($values['from_date']);
    $timestamp_to = empty($values['to_date']) ? '' : strtotime($values['to_date'].' 23:59:59');
    drupal_goto('audit/'.implode('/', array_merge(array($values['module_name'], $timestamp_from, $timestamp_to, $values['username']), array_filter($values['tags']))));
  }
}

/**
 * Form submission handler for cvwobase_d7_audit_form().
 *
 * @see cvwobase_d7_audit_form_validate()
 */
function cvwobase_d7_audit_form_submit($form, &$form_state) {
  $deleting = array_filter($form_state['values']['cfm_deleting']);
  if (!empty($deleting))
    db_delete(CVWO_AUDIT_TABLE)->condition('id', $deleting)->execute();
  drupal_set_message(t('The selected entries have been removed.'));
  $form_state['redirect'] = 'audit/'.implode('/', $form_state['build_info']['args']);
}