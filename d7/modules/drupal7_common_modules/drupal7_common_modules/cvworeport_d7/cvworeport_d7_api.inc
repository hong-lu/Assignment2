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
 * Retrieve the roles in the system
 *
 * @return
 * 		An array of ID-Name value pairs for roles
 */
function get_user_roles() {
        $roles = array();
        $result = cvwo_select('role', 'r')
                ->fields('r', array('rid', 'name'))
                ->orderBy('r.weight', 'asc')
                ->condition('r.rid', DRUPAL_AUTHENTICATED_RID, '<>')
                ->condition('r.rid', DRUPAL_ANONYMOUS_RID, '<>')
                ->execute();

        // Create an array of role ID, name pairs and return
        while ($data = $result->fetchAssoc())
                $roles[$data['rid']] = $data['name'];

        return $roles;
}

/**
 * Retrieve role permissions related to the report
 *
 * @param object $report_id
 * 		ID of report to be matched
 * @return
 * 		An array of role IDs and permissions pairs
 */
function get_report_role_permissions($report_id) {

        $result = cvwo_select(CVWO_REPORT_ROLES_TABLE, 'ro')
                ->fields('ro', array('rid', 'permission'))
                ->condition('ro.report_id', $report_id, '=')
                ->execute();

        $role_permissions = array();
        while ($data = $result->fetchAssoc())
                $role_permissions[$data['rid']] = $data['permission'];

        return $role_permissions;
}

/**
 * Retrieve user permissions related to the report
 * 
 * @param object $report_id
 * 		ID of report to be matched
 * @return
 * 		An array of user IDs and permissions pairs
 */
function get_report_user_permissions($report_id) {

        $result = cvwo_select(CVWO_REPORT_USERS_TABLE, 'u')
                ->fields('u', array('uid', 'permission'))
                ->condition('u.report_id', $report_id, '=')
                ->execute();

        $user_permissions = array();
        while ($data = $result->fetchAssoc())
                $user_permissions[$data['uid']] = $data['permission'];

        return $user_permissions;
}

/**
 * 	Get report details
 */
function cvwo_report_load($report_id) {
        $result = cvwo_select(CVWO_REPORT_TABLE, 'r')
                ->fields('r', array('report_id', 'report_name', 'weight', 'category_id', 'access_function', 'description', 'special', 'status'))
                ->condition('r.report_id', $report_id, '=')
                ->execute()
                ->fetchAssoc();
        if (!$result)
                return FALSE;

        if ($result['special'] == STATUS_ENABLED) {
                $special_result = cvwo_select(CVWO_REPORT_SPECIAL_TABLE, 's')
                        ->fields('s', array('report_text', 'drupal'))
                        ->condition('s.report_id', $report_id)
                        ->execute()
                        ->fetchAssoc();
                $result['report_text'] = $special_result['report_text'];
                $result['drupal'] = $special_result['drupal'];
        } else {
                // load queries
                $query_results = cvwo_select(CVWO_REPORT_QUERIES_TABLE, 'q')
                        ->fields('q', array('query_id', 'query_name', 'sql_statement', 'special'))
                        ->condition('q.report_id', $report_id)
                        ->execute()
                        ->fetchAllAssoc('query_id', PDO::FETCH_ASSOC);
                $result['queries'] = $query_results;
        }

        // load roles
        $roles_data = cvwo_select(CVWO_REPORT_ROLES_TABLE, 'ro')
                ->fields('ro', array('rid', 'permission'))
                ->condition('ro.report_id', $report_id)
                ->execute();
        while ($role = $roles_data->fetchAssoc()) {
                $result['role_permission'][$role['rid']] = $role['permission'];
        }

        // load users
        $users_data = cvwo_select(CVWO_REPORT_USERS_TABLE, 'u')
                ->fields('u', array('uid', 'permission'))
                ->condition('u.report_id', $report_id)
                ->execute();
        while ($user = $users_data->fetchAssoc()) {
                $result['user_permission'][$user['uid']] = $user['permission'];
        }

        return $result;
}

/**
 * 	Save report details
 */
function cvwo_report_save($values, $report_id = NULL) {
        global $user;
        if (!$report_id) {
                $message = '%user_name has added a report: %report_name';
                $tag = CVWO_REPORT_TAG_ADD;
        } else {
                $message = '%user_name has edited the report: %report_name';
                $tag = CVWO_REPORT_TAG_EDIT;
        }
        $transaction = cvwo_transaction($message, $tag, CVWOREPORT_MODULE, array('%user_name' => format_username($user), '%report_name' => $values['report_name']));

        try {
                if (!$report_id) {
                        // add the report
                        $report_id = cvwo_insert(CVWO_REPORT_TABLE)
                                ->fields(array(
                                        'report_name' => $values['report_name'],
                                        'category_id' => $values['category_id'],
                                        'access_function' => $values['access_function'],
                                        'description' => $values['description'],
                                        'special' => ($values['special'] ? STATUS_ENABLED : STATUS_DISABLED),
                                ))
                                ->execute();

                        // also, add user who is adding the report to have absolute permission
                        $values['user_permission'][$user->uid] = CVWO_REPORT_VIEW_EDIT_AND_DELETE_PERM;
                } else {
                        // update report
                        cvwo_update(CVWO_REPORT_TABLE)
                                ->fields(array(
                                        'report_name' => $values['report_name'],
                                        'category_id' => $values['category_id'],
                                        'access_function' => $values['access_function'],
                                        'description' => $values['description'],
                                        'special' => $values['special'],
                                ))
                                ->condition('report_id', $report_id, '=')
                                ->execute();
                }
                if ($values['special']) {
                        // insert/update report text
                        cvwo_merge(CVWO_REPORT_SPECIAL_TABLE)
                                ->key(array('report_id' => $report_id))
                                ->fields(array(
                                        'report_id' => $report_id,
                                        'report_text' => $values['report_text'],
                                        'drupal' => $values['drupal'],
                                ))
                                ->execute();
                } else {
                        // insert/update queries
                        if (isset($values['queries']) && !empty($values['queries'])) {
                                foreach ($values['queries'] as $query_id => $query) {
                                        cvwo_merge(CVWO_REPORT_QUERIES_TABLE)
                                                ->key(array('report_id' => $report_id, 'query_id' => $query_id))
                                                ->fields(array(
                                                        'query_name' => $query['query_name'],
                                                        'sql_statement' => $query['sql_statement'],
                                                        'special' => $query['special'],
                                                ))
                                                ->execute();
                                }
                                cvwo_delete(CVWO_REPORT_QUERIES_TABLE)
                                        ->condition('report_id', $report_id)
                                        ->condition('query_id', count($values['queries']), '>')
                                        ->execute();
                        }
                }

                // update roles
                foreach ($values['role_permission'] as $rid => $key) {
                        if ($key['permission'] != CVWO_REPORT_NO_PERMISSION) {
                                cvwo_merge(CVWO_REPORT_ROLES_TABLE)
                                        ->key(array('report_id' => $report_id, 'rid' => $rid))
                                        ->fields(array(
                                                'report_id' => $report_id,
                                                'rid' => $rid,
                                                'permission' => $key['permission'],
                                        ))
                                        ->execute();
                        } else {
                                cvwo_delete(CVWO_REPORT_ROLES_TABLE)
                                        ->condition('report_id', $report_id)
                                        ->condition('rid', $rid)
                                        ->execute();
                        }
                }

                // update permissions
                foreach ($values['user_permission'] as $uid => $key) {
                        // taking super user's into consideration (no association for super user)
                        if (!isset($key['permission']) && isset($uid) ||
                                isset($key['permission']) && $key['permission'] != '') {
                                cvwo_merge(CVWO_REPORT_USERS_TABLE)
                                        ->key(array('report_id' => $report_id, 'uid' => $uid))
                                        ->fields(array(
                                                'report_id' => $report_id,
                                                'uid' => $uid,
                                                'permission' => isset($key['permission']) ? $key['permission'] : $key,
                                        ))
                                        ->execute();
                        }
                }


                $query = cvwo_delete(CVWO_REPORT_USERS_TABLE)
                        ->condition('report_id', $report_id);
                if (!empty($values['user_permission']))
                        $query->condition('uid', array_keys($values['user_permission']), 'NOT IN');
                $query->execute();
        } catch (Exception $e) {
                $transaction->rollback();
                watchdog(CVWOREPORT_MODULE, $e);
                return FALSE;
        }

        return TRUE;
}

function cvwo_report_delete($report_id) {
        global $user;
        $details = cvwo_report_load($report_id);

        $message = '%user_name deleted a report: %report_name';
        $tag = CVWO_REPORT_TAG_DELETE;
        return cvwo_update(CVWO_REPORT_TABLE)
                        ->fields(array(
                                'status' => STATUS_DISABLED,
                        ))
                        ->condition('report_id', $report_id, '=')
                        ->execute_logged($message, $tag, CVWOREPORT_MODULE, array('%user_name' => format_username($user), '%report_name' => $details['report_name']));
}

function cvwo_report_save_weights_and_categories($values) {
        global $user;
        $message = '%user_name reordered the weights and categories';
        $tag = CVWO_REPORT_TAG_EDIT;

        $transaction = cvwo_transaction($message, $tag, CVWOREPORT_MODULE, array('%user_name' => format_username($user)));

        try {
                foreach ($values as $cat_id => $reports) {
                        foreach ($reports as $report_id => $report) {
                                cvwo_update(CVWO_REPORT_TABLE)
                                        ->fields(array(
                                                'category_id' => $report['region-' . $report_id],
                                                'weight' => $report['weight-' . $report_id],
                                        ))
                                        ->condition('report_id', $report_id)
                                        ->execute();
                        }
                }
        } catch (Exception $e) {
                $transaction->rollback();
                watchdog(CVWOREPORT_MODULE, $e);
                return FALSE;
        }
        return TRUE;
}

/**
 * Retrieve a report
 *
 * @param object $report_id
 * 		ID of report to be matched
 * @return
 * 		Report DB result set
 */
function get_report($report_id) {
        return cvwo_select(CVWO_REPORT_TABLE, 'r')
                        ->fields('r', array('report_id', 'report_name', 'weight', 'category_id', 'description', 'special', 'status'))
                        ->condition('r.report_id', $report_id, '=')
                        ->execute()
                        ->fetchAssoc();
}

/**
 * Retrieve the special report text
 *
 * @param object $report_id
 * 		ID of report to be matched
 * @return
 * 		Report DB result set
 */
function get_special_report_text($report_id) {
        return cvwo_select(CVWO_REPORT_SPECIAL_TABLE, 's')
                        ->fields('s', array('report_text', 'drupal'))
                        ->condition('report_id', $report_id, '=')
                        ->execute()
                        ->fetchAssoc();
}

// /** 
// * Retrieves queries related to the report
// *
// * @param object $report_id
// *		ID of report to be matched
// * @return
// *		Queries DB result set
// */
// function get_report_queries($report_id){
// $get_report_queries_sql = "SELECT query_name, sql_statement, special FROM {".CVWO_REPORT_QUERIES_TABLE."} WHERE report_id='%d'";
// return cvwo_select_query($get_report_queries_sql, $report_id);
// }

/**
 * Add report text temporarily to the special report table
 * To be used in testing special reports without adding actually adding the report
 * 
 * @param object $report_text
 * 		Code to be added in the form of text
 *
 */
function add_edit_temp_special_report_text($report_text) {
        return cvwo_merge(CVWO_REPORT_SPECIAL_TABLE)
                        ->key(array('report_id' => CVWO_REPORT_DEFAULT_REPORT_ID))
                        ->fields(array(
                                'report_id' => CVWO_REPORT_DEFAULT_REPORT_ID,
                                'report_text' => $report_text,
                        ))
                        ->execute();
}

/**
 *  Returns the localisation setup details
 */
function get_report_setup() {
        return cvwo_select(CVWO_REPORT_SETUP_TABLE, 's')
                        ->fields('s', array('setup_id', 'menu_name', 'nric_autocomplete'))
                        ->execute()
                        ->fetchAssoc();
}

/**
 * 
 * Gets the real name of the user from the drupal id.
 * @param $drupal_id, the drupal system user id.
 * 
 */
function get_name_by_drupal_id($uid) {
        return format_username(user_load($uid));
}

/**
 * Get drupal user name by drupal ID
 *
 * @param object $uid
 * 		Drupal user ID
 * @return
 * 		Drupal user name
 */
function get_drupal_user_name($uid) {
        $drupal_user = user_load($uid);
        return $drupal_user->name;
}

/**
 * Returns all the details stored in the cvwo_person of a user according
 * to a given User ID.
 * 
 * @return user object
 *      An user object that contains the values of all the columns of the user profile
 *      table.
 */
function get_user_profile_details_by_id($uid) {
        return user_load($uid);
}

/**
 * Returns all the details stored in the cvwo_person of a user according
 * to a given NRIC FIN.
 * 
 * @return user object
 *      A user object that contains the values of all the columns of the user profile
 *      table.
 */
function get_user_profile_details_by_nric($nric) {
        $result = cvwo_select(CVWO_PERSON_TABLE, 'p')
                ->fields('p', array('uid'))
                ->condition('p.nric_fin', $nric, 'LIKE')
                ->execute()
                ->fetchField();
        if ($result)
                return user_load($result);
        return FALSE;
}

/**
 * function that fetches the report category information
 * @return array sorted by weights
 *         with key as the category id and value as the category name
 */
function get_report_categories() {
        $sql_data = cvwo_select(CVWO_REPORT_CATEGORY_TABLE, 'c')
                ->fields('c', array('category_id', 'category_name', 'weight', 'deletable'))
                ->orderBy('c.weight', 'asc')
                ->execute();

        while ($data = $sql_data->fetchAssoc()) {
                $result[$data['category_id']] = $data['category_name'];
        }

        return $result;
}

/**
 * function that fetches the report category information
 * @return array sorted by weights
 *         with key as the category id and value as the category details
 */
function get_report_categories_detail() {

        $result = cvwo_select(CVWO_REPORT_CATEGORY_TABLE, 'c')
                ->fields('c', array('category_id', 'category_name', 'weight', 'deletable'))
                ->orderBy('c.weight', 'asc')
                ->execute()
                ->fetchAllAssoc('category_id', PDO::FETCH_ASSOC);

        usort($result, function($a, $b) {
            return $a['weight'] - $b['weight'];
        });

        return $result;
}

/**
 * function that fetches details about a particular category id
 * @param $cid
 *        the category id of the category
 * @return object
 *         containing the information needed
 */
function get_report_category($cid) {
        return cvwo_select(CVWO_REPORT_CATEGORY_TABLE, 'c')
                        ->condition('category_id', $cid)
                        ->fields('c', array('category_id', 'category_name', 'weight', 'deletable'))
                        ->execute()
                        ->fetchAssoc();
}

/**
 * Format special element strings
 */
function format_statement($statement) {
        $arr = array("&" => "&amp;", "<" => "&lt;", ">" => "&gt;");
        return strtr($statement, $arr);
}

/**
 * 	Default user_access function to check whether a user has access to the report
 */
function cvwo_report_default_access($report_id) {
        global $user;
        $query = cvwo_select(CVWO_REPORT_USERS_TABLE, 'u');
        $query->condition('u.report_id', $report_id);
        $query->condition('u.uid', $user->uid);
        $query->addField('u', 'permission');
        $user_perm = $query->execute()->fetchField();
        if ($user_perm)
                return $user_perm;

        $query = cvwo_select(CVWO_REPORT_ROLES_TABLE, 'ro');
        $query->condition('ro.report_id', $report_id);
        $query->condition('ro.rid', array_keys($user->roles));
        $query->addExpression('MAX(ro.permission)', 'permission');
        $role_perm = $query->execute()->fetchField();

        if ($role_perm)
                return $role_perm;
        return CVWO_REPORT_NO_PERMISSION;
}

/**
 * Paging non-SQL data
 * 
 * @param type $data
 * @param type $results_per_table
 * @param type $headers
 */
function pager_array($data, $results_per_table, $headers) {
        // Gets number of records
        $total = count($data);
        // Define items per page for paging
        $num_per_page = $results_per_table;

        // Initialize pager and gets current page
        $page = pager_default_initialize($total, $num_per_page);
        // Gets first record and last record to show        
        $offset = ($page) * $num_per_page;
        $last = ($offset + $num_per_page < $total) ? $offset + $num_per_page : $total;
        // Calls table theme
        $output = theme('table', array(
                'header' => $headers,
                'rows' => pager_array_splice($data, $num_per_page),
                'empty' => t('There are no records to be displayed'),
                'offset' => $offset, 'last' => $last));
        // Calls Drupal standard pager theme
        return $output .= theme('pager', array('quantity' => $total));
}

/**
 * Paging non-SQL data (sub-function)
 * 
 * @global type $pager_page_array
 * @global array $pager_total
 * @global array $pager_total_items
 * @param type $data
 * @param type $limit
 * @param type $element
 * @return type
 */
function pager_array_splice($data, $limit = 9, $element = 0) {
        global $pager_page_array, $pager_total, $pager_total_items;
        $page = isset($_GET['page']) ? $_GET['page'] : '';

        // Convert comma-separated $page to an array, used by other functions.
        $pager_page_array = explode(',', $page);

        // We calculate the total of pages as ceil(items / limit).
        $pager_total_items[$element] = count($data);
        $pager_total[$element] = ceil($pager_total_items[$element] / $limit);
        $pager_page_array[$element] = max(0, min((int) $pager_page_array[$element], ((int) $pager_total[$element]) - 1));
        return array_slice($data, $pager_page_array[$element] * $limit, $limit, TRUE);
}

/**
 * callback function that handles export to excel
 * @param type $form
 * @param type $form_state
 */
function export_to_excel($form, &$form_state) {
        $report_id = $form_state['report_id'];
        $report_name = $form_state['report_name'];
        $report_desc = $form_state['report_desc'];
        $excel_export_data = $form_state['excel_export_data'];

        // print to excel
        header('Content-type: application/vnd.ms-excel');
        header('Content-Disposition: attachment; filename="' . $report_name . '.xls"');
        $output = '<html>';
        $output .= '<head><meta http-equiv=Content-Type content="text/html; charset=utf-8"></head>';
        $output .= '<body>';
        $output .= '<h1>' . $report_name . '</h1>';
        $output .= '<p>' . $report_desc . '</p>';
        foreach ($excel_export_data as $data)
                $output .= $data;
        $output .= '</body></html>';
        print $output;
        exit;
}

/**
 * export function for lions report module
 * @param $report_name: this name will be used to name the xls file
 * @param $data: main info of this report
 * @param $title: optional, set the title of xls file
 * @param $extra_info:optional, extra information attached to the end of this report
 */
function drupal_special_report_export_to_excel($report_name, $header = NULL, $data, $extra_info = NULL) {

	$table = '<style type="text/css">' . "\n";
	$styles = file_get_contents(
		drupal_get_path('module', \CVWO\SAC_NL\Activity\MODULE_NAME) .
		'/sac_nl_activity_session_attendance_print.css');
	if (is_array($styles)) {
		$table .= implode("\n", $styles);
	} else {
		$table .= $styles;
	}
	$table .= '</style>' . "\n";

	if (isset($header))
		$header = '<p>' . $header . '</p>';

	$data = $table . $header . $data;

	$date = date("d_m_Y");

	// attached the date information to the report
	$file_name = $report_name . "_" . $date;

	cvwobase_download_as_excel_self_defined_content($data, $file_name . '.xls');
}

/**
 *  convert Singapore date format to standard date-time format
 *  @param: $date, $flag (default: start)
 *  flag: "start"  time: 00:00:00
 *  flag: "end"    time: 23:59:59
 */
function to_standard_date($date, $flag = "start") {
        // $date format DD-MM-YYYY
        $output = substr($date, 6, 4) . substr($date, 2, 3) . "-" . substr($date, 0, 2);
        if ($flag == "start")
                $output.= " 00:00:00";
        else
                $output.= " 23:59:59";
        return $output;
}

/**
 *  abstract function to process the queries
 *  @param 1: MySQL query 
 * 	@param 2: return-value_name in AS syntax 
 *  
 */
function query_to_data($return_value_name, $sql, array $options=null) {
        $args = func_get_args();

        $result = call_user_func_array('cvwo_query', array_slice($args, 1));
        $data = $result->fetchAssoc();
        if (isset($data[$return_value_name]))
                return $data[$return_value_name];
        else
                return "Not Applicable";
}

/**
 * header
 * Generate date info and title
 * @param date values, title
 * @return html
 */
function report_header_info($values, $title) {

        $output = '';
        $start_date = isset($values['start_date']) ? new DateTime($values['start_date']) : null;
        $end_date = isset($values['end_date']) ? new DateTime($values['end_date']) : null;

        $output.= '<br /><h3>' . $title . '</h3>';
        if (!empty($start_date)) {
                $output.= '<h4>From Date: ' . $start_date->format(\CVWO\Base\DATE_FORMAT_DATETIME) . '</h4>';
        }
        if (!empty($end_date)) {
                $output.= '<h4>Until Date: ' . $end_date->format(\CVWO\Base\DATE_FORMAT_DATETIME) . '</h4>';
        }

        return $output;
}

/**
 * check whether the client stays in the service boundary
 * @param type $client_postal_code
 * @return boolean: true if the client stays in the service boundary
 */
function is_within_service_boundary($client_postal_code) {
	$centre_boundary = \CVWO\SAC_NL\Base\Config::get_service_boundary();
	
	return true;
}
