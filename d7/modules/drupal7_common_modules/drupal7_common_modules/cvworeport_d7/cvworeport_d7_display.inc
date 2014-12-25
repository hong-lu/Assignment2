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
 * Generate the result of the report
 */
function cvwo_report_display_form($form, &$form_state, $report_id) {
        $setup = get_report_setup();
        $breadcrumb = drupal_set_breadcrumb(null);
        $breadcrumb[] = l(t('Home'), '<front>');
        $breadcrumb[] = l($setup['menu_name'], CVWOREPORT_MODULE);
        drupal_set_breadcrumb($breadcrumb);

        $report = cvwo_report_load($report_id);
        $form_state['report_id'] = $report_id;
        $form_state['report_name'] = $report['report_name'];
        $form_state['report_desc'] = $report['description'];
        drupal_set_title($report['report_name']);

        $form['report_id'] = array(
                '#type' => 'value',
                '#value' => $report_id,
        );

        if ($report['special'] == STATUS_ENABLED) {
                // do eval
                if ($report['drupal'] == STATUS_ENABLED) {
                        //This crazy nesting is to make sure that we do not expose our
                        //variables to the included file. We only want $form, $form_state
                        //and other harmless variables in.
                        $function = function($form, &$form_state) use ($report_id) {
                                $included_forms = &drupal_static(__FUNCTION__);
                                if (empty($included_forms)) {
                                        $included_forms = array();
                                }
                                if (!in_array(intval($report_id), $included_forms, true)) {
                                        $included_forms[] = intval($report_id);
                                        if (ini_get('allow_url_include')) {
                                                include('cvworeport://private/' . $report_id);
                                        } else {
                                                $tmp_file_path = tempnam(null, CVWOREPORT_MODULE);
                                                copy('cvworeport://private/' . $report_id,
                                                        $tmp_file_path);
                                                include($tmp_file_path);
                                        }
                                }

                                return form_main($form, $form_state);
                        };

                        $form = $function($form, $form_state);
                } else {
                        $form['special'] = array(
                                '#type' => 'markup',
                                '#markup' => php_eval($report['report_text']),
                        );
                }
        } else {
                // do queries
                $results_per_table = isset($_GET['results_per_table']) ? $_GET['results_per_table'] : 10;

                // Build tables for each query
                $i = 1;
                $excel_export_data = array();
                foreach ($report['queries'] as $query_id => $query) {
                        $query_name = $query['query_name'];
                        $sql_statement = $query['sql_statement'];
                        $special = $query['special'];

                        // if the statement is a special element		
                        if ($special) {
                                $eval_value = php_eval($sql_statement);
                                $form['query_result' . $i] = array(
                                        '#markup' => $eval_value,
                                );

                                // add to excel data
                                $excel_export_data['query_' . $i] = $eval_value;
                        } else { // if the statement is a sql statement
                                try {
                                        $sql_result = cvwo_query($sql_statement);
                                        $sql_query = $sql_result->fetchAll();

                                        // save sql results in $cols and $rows
                                        // $cols: database_column_name => data_in_column
                                        // $rows: row_number => data_in_each_row
                                        $cols = array();
                                        $rows = array();
                                        foreach ($sql_query as $row) {
                                                $row = (array) $row;
                                                $rows[] = array_values($row);
                                                foreach (array_keys($row) as $key) {
                                                        $cols[$key][] = $row[$key];
                                                }
                                        }

                                        $col_count = count($cols);
                                        $row_count = count($rows);
                                        $query_name_formatted = '<br><br><strong><em>' . $query_name . ' : </em></strong>';
                                        $form['query_' . $i] = array(
                                                '#markup' => $query_name_formatted,
                                        );

//                    if ($col_count == 1 && $row_count == 1 && $sql_result) { // if one column and one row
//                        $single_value = array_pop($cols);
//                        $form['query_result' . $i] = array(
//                            '#value' => $single_value, // there is only one key, value pair in the array
//                        );
//
//                        // add to excel data
//                        $excel_export_data['query_' . $i] = $query_name_formatted . $single_value;
//                    } else 
                                        if ($sql_result) {

                                                $output = pager_array($rows, $results_per_table, array_keys($cols)) . '<br/>';

                                                $form['query_result' . $i] = array(
                                                        '#markup' => $output,
                                                );

                                                // add to excel data
                                                $excel_export_data['query_' . $i] = $query_name_formatted . $output;
                                        } else {
                                                $form['query_result' . $i] = array(
                                                        '#markup' => 'There is an error in this query. Click ' .
                                                        l('here', CVWOREPORT_MODULE . '/edit/' . $report_id)
                                                                . ' to check the report.',
                                                );
                                        }
                                } catch (Exception $e) {
                                        $form['query_' . $i] = array(
                                                '#markup' => '<br><br><strong><em>' . $query_name . ' : </em></strong>There is an error'.$e.' in this query. Click ' .
                                                l('here', CVWOREPORT_MODULE . '/edit/' . $report_id)
                                                       . ' to check the report.',
                                        );
                                }
                        }
                        $i++;
                }

                $form_state['excel_export_data'] = $excel_export_data;

                $form['export_excel'] = array(
                        '#type' => 'submit',
                        '#id' => 'export_excel',
                        '#name' => 'export_excel',
                        '#value' => 'Export to Excel',
                        '#submit' => array('export_to_excel'),
                );
        }
        return $form;
}

function cvwo_report_display_form_submit($form, &$form_state) {
        $options = array();
        foreach (element_children($form_state['values']) as $id) {
                $options[$id] = $form_state['values'][$id];
        }
        $form_state['redirect'] = array(CVWOREPORT_MODULE . '/display/' . $form_state['values']['report_id'], array('query' => $options));
}

/*
  function cvwo_report_display_form(&$form_state){
  drupal_add_js(drupal_get_path("module",CVWOREPORT_MODULE)."/cvwo_report.js");

  // Get the user ID (drupal ID) first
  global $user;
  $user_id = $user->uid;
  // Get report ID
  $report_id = $_GET['report_id'];
  $results_per_table = $_GET['results_per_table'];
  if(!$results_per_table) $results_per_table = 10;
  $form_state['report_id'] = $report_id;
  // extract report name and description
  $basic_report_info = get_report($report_id);
  $report_name = $basic_report_info->report_name;
  $form_state['report_name'] = $report_name;
  $report_desc = $basic_report_info->description;
  $form_state['report_desc'] = $report_desc;

  drupal_set_title(t($report_name));

  $form = array();

  // hidden input to store results per table
  $form['results_per_table'] = array(
  '#type' => 'hidden',
  '#id' => 'results_per_table',
  '#value' => '',
  );

  $form['report_name'] = array(
  '#value' => t('<br><h2>'.$report_name.'</h2><br>')
  );

  $form['report_description'] = array(
  '#value' => t($report_desc),
  );

  // get queries related to the report
  $report_queries = get_report_queries($report_id);

  // Build tables for each query and provide permissions accordingly
  $i = 1;
  $excel_export_data = array();
  while($query = db_fetch_object($report_queries)){
  $query_name = $query->query_name;
  $query_sql = $query->sql_statement;
  $special = $query->special;

  if($special){ // if the statement is a special element
  $eval_value = '<br><br>'.drupal_eval($query_sql);
  $form['query_result'.$i] = array(
  '#value' => $eval_value,
  );

  // add to excel data
  $excel_export_data['query_'.$i] = $eval_value;
  }else{ // if the statement is a sql statement
  try{
  $sql_result = cvwo_select_query($query_sql);

  // get the columns
  $cols = db_fetch_array($sql_result);
  $col_count = count($cols);
  $row_count = db_affected_rows($sql_result); // pg_num_rows doesn't work. Why?

  $query_name_formatted = '<br><br><strong><em>'.$query_name.' : </em></strong>';
  $form['query_'.$i] = array(
  '#value' => $query_name_formatted,
  );

  if($col_count==1 && $row_count==1 && $sql_result){ // if one column and one row
  $single_value = array_pop($cols);
  $form['query_result'.$i] = array(
  '#value' => $single_value, // there is only one key, value pair in the array
  );

  // add to excel data
  $excel_export_data['query_'.$i] = $query_name_formatted . $single_value;
  }else if($sql_result){
  $table = new CVWOTable($query_sql,array_keys($cols),array_keys($cols),$results_per_table);
  $output = $table->getOutput(false);

  $form['query_result'.$i] = array(
  '#value' => $output,
  );

  // add to excel data
  $excel_export_data['query_'.$i] = $query_name_formatted . $table->getOutput(false);
  }else{
  $form['query_result'.$i] = array(
  '#value' => 'There is an error in this query. Click '.
  l('here', CVWOREPORT_MODULE.'/edit', array('attributes' => array('target' => '_blank'),
  'query' => array('report_id' => $report_id))).' to check the report.',
  );
  }
  }catch(Exception $e){
  $form['query_'.$i] = array(
  '#value' => '<br><br><strong><em>'.$query_name.' : </em></strong>There is an error in this query. Click '.
  l('here', CVWOREPORT_MODULE.'/edit', array('attributes' => array('target' => '_blank'),
  'query' => array('report_id' => $report_id))).' to check the report.',
  );
  }
  }
  $i++;
  }

  $form_state['excel_export_data'] = $excel_export_data;

  $form['results_per_table_option'] = array(
  '#value' => '<br>Results per table: '.
  ($results_per_table!=10?'<a href="javascript: void(0);" id="10" onclick="set_results_per_table(this)"> 10 </a>':' 10 ').
  ($results_per_table!=20?'<a href="javascript: void(0);" id="20" onclick="set_results_per_table(this)"> 20 </a>':' 20 ').
  ($results_per_table!=30?'<a href="javascript: void(0);" id="30" onclick="set_results_per_table(this)"> 30 </a>':' 30 ').
  ($results_per_table!=50?'<a href="javascript: void(0);" id="50" onclick="set_results_per_table(this)"> 50 </a>':' 50 '),
  );

  // it is here to enable links to submit onclick
  // otherwise link onclicks do not work at all
  // this div is hidden by javascript from the js file
  $form['dummy_button'] = array(
  '#id' => 'dummy_button',
  '#type' => 'submit',
  '#value' => 'New report',
  '#prefix' => '<div id="to-hide">',
  '#suffix' => '</div>',
  );
  $form['back'] = array(
  '#type' => 'submit',
  '#id' => '_back',
  '#name' => '_back',
  '#value' => t('Back to List'),
  '#prefix' => '<br><hr>',

  );

  $form['export_excel'] = array(
  '#type' => 'submit',
  '#id' => 'export_excel',
  '#name' => 'export_excel',
  '#value' => 'Export to Excel',
  '#submit' => array('export_to_excel'),
  //'#prefix' => '<br><hr>',
  );

  return $form;
  }
 */

/**
 * Implementation of hook_submit
 */
/*
  function cvwo_report_display_form_submit($form, &$form_state){
  if($form_state['clicked_button']['#id'] == '_back'){
  $form_state['rebuild'] = FALSE;
  unset($form_state['storage']); // redirect wouldn't work otherwise
  $form_state['redirect'] = CVWOREPORT_MODULE;
  }
  if(!empty($_POST['results_per_table'])){
  $report_id = $form_state['report_id'];
  $results_per_table = $_POST['results_per_table'];
  $form_state['rebuild'] = FALSE;
  unset($form_state['storage']); // redirect wouldn't work otherwise
  $form_state['redirect'] = array(CVWOREPORT_MODULE.'/display', array('report_id' => $report_id, 'results_per_table' => $results_per_table));
  }
  }
  ?> */

