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

namespace CVWO\Community {
	require_once drupal_get_path('module', MODULE_NAME) . '/cvwo_community_person.inc';
	require_once drupal_get_path('module', MODULE_NAME) . '/cvwo_community_api.inc';

use \CVWO\Base\Database as db;

	function info_ref_list($form, &$form_state) {
		$form = array();

		//create search object
		$search = new \CVWOBaseSearch('Info/Referral', $form);

		/*
		 * Basic Search Configuration
		 */
		//define header for basic search result, ignore options
		$header_basic = array(
			array('data' => t('Date/Time Requested'), 'field' => 'date_requested', 'sort' => 'desc'),
			array('data' => t('Name'), 'field' => 'name'),
			array('data' => 'Address', 'field' => 'address_id'),
			array('data' => t('Info or Referral'), 'field' => 'info_ref'),
			array('data' => t('Status'), 'field' => 'status'),
		);

		//define basic search form element
		$form_element_basic = array(
			'#type' => 'fieldset',
			'#title' => t('Basic Search'),
			'#collapsible' => true,
			'search_field' => array(
				'#type' => 'textfield',
				'#title' => t('Search Person or NRIC/FIN')
			)
		);

		//define basic search query
		$query = db\select(INFO_REF_REPORT_TABLE, 'info_ref');
		$query->fields('info_ref');
		$query->condition('info_ref.is_deleted', 0, '=');
		$query->join(\CVWO\Base\PERSON_TABLE, 'p', 'info_ref.person_id=p.person_id');
		$query->fields('p', array('name'));
		$query->leftJoin(CVWO_ADDRESS_TABLE, 'a', 'info_ref.address_id=a.address_id');

		$query_where =
			'(p.name LIKE :str OR p.nric_fin LIKE :str2)';

		//define custom columns, for formating result
		$custom_columns = array(
			'name' => '',
			'follow_up' => function($values) {
				return cvwocodem2_getanstext($values['follow_up']);
			},
			'type' => function($values) {
				return cvwocodem2_getanstext($values['type']);
			},
			'info_ref' => function($values) {
				return cvwocodem2_getanstext($values['info_ref']);
			},
			'status' => function($values) {
				return cvwocodem2_getanstext($values['status']);
			},
			'address_id' => '\CVWO\Community\community_format_address',
			'date_requested' => function($value) {
				return community_format_date_from_database($value['date_requested']);
			},
			'worker_id' => '\CVWO\Community\community_format_worker',
		);

		//register custom columns
		$search->defineCustomColumns($custom_columns);

		//define basic search
		$search->defineBasicSearch($form_element_basic, $query, $header_basic, $query_where);

		/*
		 * Advanced Search Configuration
		 */

		//define search filters, format: array('db_table' => array('db_field' , 'module_name', 'codem_question')
		$filters = array(
			'info_ref' => array(
				array('info_ref', MODULE_NAME, INFO_REF_INFO_OR_REF_QUESTION),
				array('type', MODULE_NAME, INFO_REF_TYPE_QUESTION),
				array('status', MODULE_NAME, INFO_REF_STATUS_QUESTION),
				array('follow_up', MODULE_NAME, INFO_REF_FOLLOW_UP_QUESTION),
			),
		);
		//register codem filter
		$search->registerCodemFilter($filters);


		//define display fields, format: array('db_table' => array('db_field' => 'form_element_title')
		//cannot contain the default header
		$display_fields = array(
			'info_ref' => array(
				'address_id' => t('Address'),
				'details' => t('Details'),
				'action_taken' => t('Action Taken'),
				'worker_id' => t('Attended By'),
				'follow_up' => t('Follow Up'),
				'type' => INFO_REF_TYPE_QUESTION,
			),
		);
		//register display fields
		$search->registerDisplayFields($display_fields);

		//define default header for advanced search
		$header_advanced = array(
			array('data' => 'Date Time', 'field' => 'date_requested', 'table' => 'info_ref', 'sort' => 'desc'),
			array('data' => 'Name', 'field' => 'name', 'table' => 'p'),
			array('data' => 'NRIC/FIN', 'field' => 'nric_fin', 'table' => 'p'),
			array('data' => INFO_REF_INFO_OR_REF_QUESTION, 'field' => 'info_ref', 'table' => 'info_ref'),
			array('data' => INFO_REF_STATUS_QUESTION, 'field' => 'status', 'table' => 'info_ref'),
		);
		//fields to skip when constructing the query, format: array('display_field_key' => 'corresponding_db_table')
		$skipped_fields = array();

		//define, table column to be joined. IMPT: check schema to enforce this :)
		$join_column = 'person_id';

		//define advanced search
		$search->setAdvancedSearchQuery($query);
		$search->defineAdvancedSearch($header_advanced, $skipped_fields, $join_column);
		/*
		 * Additional Filters that cannot covered by code mainteanance 
		 * format : normal form element plus '#query_callback'
		 * query_callback is a callback function where you can alter the 'where' clause of sql
		 */

		$filter_form['date_from'] = array(
			'#type' => 'date_popup',
			'#title' => 'Date From',
			'#date_format' => \CVWO\Base\DATE_FORMAT_DATE,
			'#date_year_range' => '-100:0',
			'#date_label_position' => 'none',
			'#size' => 10,
			'#query_callback' => function(&$query, $value) {
				$value = new \DateTime($value);
				$query = $query->condition('info_ref.date_requested', $value->format(\CVWO\Base\DATE_FORMAT_DATABASE), '>=');
				return $query;
			}
		);

		$filter_form['date_to'] = array(
			'#type' => 'date_popup',
			'#title' => 'Date To',
			'#date_format' => \CVWO\Base\DATE_FORMAT_DATE,
			'#date_year_range' => '-100:0',
			'#date_label_position' => 'none',
			'#size' => 10,
			'#query_callback' => function(&$query, $value) {
				$value = new \DateTime($value);
				$value->setTime(23, 59, 59);
				$query = $query->condition('info_ref.date_requested', $value->format(\CVWO\Base\DATE_FORMAT_DATABASE), '<=');
				return $query;
			}
		);

		//set the additional filters
		$search->addCustomFilters($filter_form);
		aas_augment_search_for_address($query, $search);

		return $search;
	}

	function info_ref_list_submit($form, &$form_state)
	{
		$form_state['rebuild'] = true;
	}
}
