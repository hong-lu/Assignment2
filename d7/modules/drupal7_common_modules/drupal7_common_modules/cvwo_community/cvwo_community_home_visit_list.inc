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

	/**
	 * Builds the template for all Home Visit Advanced Search forms.
	 * 
	 * @return \CVWOBaseSearch
	 */
	function home_visit_list($form, &$form_state)
	{
		$form = array();

		//create search object
		$search = new \CVWOBaseSearch('Home Visit', $form);

		/*
		 * Basic Search Configuration
		 */
		//define header for basic search result, ignore options
		$header_basic = array(
			array('data' => t('Date/Time of Visit'), 'field' => 'date_visited', 'sort' => 'desc'),
			array('data' => t('Name'), 'field' => 'name'),
			array('data' => t('Address'), 'field' => 'address_id'),
			array('data' => t('Home Condition'), 'field' => 'home_condition'),
			array('data' => t('Visited By'), 'field' => 'worker_id'),
		);

		//define basic search form element
		$form_element_basic = array(
			'#type'			 => 'fieldset',
			'#title'		 => t('Basic Search'),
			'#collapsible'	 => true,
			'search_field' => array(
				'#type' => 'textfield',
				'#title' => t('Search Person or NRIC/FIN')
			)
		);

		//define basic search query
		$query = db\select(HOME_VISIT_REPORT_TABLE, 'home_visit');
		$query->fields('home_visit');
		$query->condition('home_visit.is_deleted', 0, '=');
		$query->join(\CVWO\Base\PERSON_TABLE, 'p', 'home_visit.person_id=p.person_id');
		$query->fields('p', array('name'));
		$query->join(\CVWO\Base\PERSON_TABLE, 'w', 'home_visit.worker_id=w.person_id');
		//$query->fields('w', array('name'));
		$query->leftJoin(CVWO_ADDRESS_TABLE, 'a', 'home_visit.address_id=a.address_id');

		$query_where = '(p.name LIKE :str OR p.nric_fin LIKE :str2)';

		//define basic search
		$search->defineBasicSearch($form_element_basic, $query, $header_basic, $query_where);

		/*
		 * Advanced Search Configuration
		 */

		//define search filters, format: array('db_table' => array('db_field' , 'module_name', 'codem_question')
		$filters = array(
			'home_visit' => array(
				array('requires_attention', MODULE_NAME, HOME_VISIT_REQUIRES_ATTENTION_QUESTION)
			),
			'p' => array(
				array('gender', BASE_MODULE_NAME, \CVWO\Base\PERSONAL_PARTICULARS_GENDER_QUESTION),
				array('race', BASE_MODULE_NAME, \CVWO\Base\PERSONAL_PARTICULARS_RACE_QUESTION),
				array('religion', BASE_MODULE_NAME, \CVWO\Base\PERSONAL_PARTICULARS_RELIGION_QUESTION),
				array('nationality', BASE_MODULE_NAME, \CVWO\Base\PERSONAL_PARTICULARS_NATIONALITY_QUESTION),
				array('ethnicity', BASE_MODULE_NAME, \CVWO\Base\PERSONAL_PARTICULARS_ETHNICITY_QUESTION),
				array('nric_colour', BASE_MODULE_NAME, \CVWO\Base\PERSONAL_PARTICULARS_NRIC_COLOUR_QUESTION),
				array('salutation', BASE_MODULE_NAME, \CVWO\Base\PERSONAL_PARTICULARS_SALUTATION_QUESTION),
				array('gender', BASE_MODULE_NAME, \CVWO\Base\PERSONAL_PARTICULARS_GENDER_QUESTION),
				array('race', BASE_MODULE_NAME, \CVWO\Base\PERSONAL_PARTICULARS_RACE_QUESTION),
				array('religion', BASE_MODULE_NAME, \CVWO\Base\PERSONAL_PARTICULARS_RELIGION_QUESTION),
			),
		);
		//register codem filter
		$search->registerCodemFilter($filters);


		//define display fields, format: array('db_table' => array('db_field' => 'form_element_title')
		//cannot contain the default header
		$display_fields = array(
			'home_visit' => array(
				'is_successful' => t('Successful Visit'),
				'requires_attention' => t('Requires Attention'),
				'items_brought' => t('Items Brought'),
				'remarks' => t('Remarks'),
			),
			'p' => array(
				'nric_colour' => t('NRIC Colour'),
				'salutation' => t('Salutation'),
				'gender' => t('Gender'),
				'date_of_birth' => t('Age'),
				'race' => t('Race'),
				'religion' => t('Religion'),
			),
			CVWO_PERSON_CONTACT_TABLE => array(
				'contact_id' => t('Contact'),
			)
		);
		//register display fields
		$search->registerDisplayFields($display_fields);
		
		//define custom columns, for formating result
		$custom_columns = array(
			'name' => '',
			'home_condition' => function($values) {
				$query = db\select(HOME_VISIT_REPORT_HOME_CONDITION_TABLE, 'c')
						->fields('c', array('home_condition'))
						->condition('c.home_visit_report_id', $values['home_visit_report_id'])
						->condition('c.is_deleted', 0)
						->execute()
						->fetchCol();

				if (empty($query))
				{
					return '';
				}

				foreach ($query as $aid)
				{
					$conditions[] = cvwocodem2_getanstext($aid);
				}

				return implode('-', $conditions);
			},
			'is_successful' => function($values) {
				return empty($values['is_successful'])? t('No') : t('Yes');
			},
			'requires_attention' => function($values) {
				return cvwocodem2_getanstext($values['requires_attention']);
			},
			'worker_id' => '\CVWO\Community\community_format_worker',
			'date_visited' => function($value) {
				return community_format_date_from_database($value['date_visited']);
			},
			'date_of_birth' => function($values) {
				if (!empty($values['date_of_birth']))
				{
					$dob = new \DateObject($values['date_of_birth']);
					return t('@date (@years years old)', array(
						'@date' => $dob->format(\CVWO\Base\DATE_FORMAT_DATE),
						'@years' => \CVWO\Base\calculate_age($dob)
					));
				}
				
				return '';
			},
			'contact_id' => '\CVWO\Community\commuinty_format_contact',
			'address_id' => '\CVWO\Community\community_format_address',
		);

		//register custom columns
		$search->defineCustomColumns($custom_columns);

		//define default header for advanced search
		$header_advanced = array(
			array('data' => 'Date Time', 'field' => 'date_visited', 'table' => 'home_visit', 'sort' => 'desc'),
			array('data' => 'Name', 'field' => 'name', 'table' => 'p'),
			array('data' => 'NRIC/FIN', 'field' => 'nric_fin', 'table' => 'p'),
			array('data' => 'Visited By', 'field' => 'worker_id', 'table' => 'home_visit'),
		);
		
		//fields to skip when constructing the query, format: array('display_field_key' => 'corresponding_db_table')
		$skipped_fields = array(
			'contact_id' => CVWO_PERSON_CONTACT_TABLE,
		);

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
			'#title' => t('Date From'),
			'#date_format' => \CVWO\Base\DATE_FORMAT_DATE,
			'#date_year_range' => '-100:0',
			'#date_label_position' => 'none',
			'#size' => 10,
			'#query_callback' => function(&$query, $value) {
				$value = new \DateTime($value);
				$query = $query->condition('home_visit.date_visited',
					$value->format(\CVWO\Base\DATE_FORMAT_DATABASE), '>=');
				return $query;
			}
		);

		$filter_form['date_to'] = array(
			'#type' => 'date_popup',
			'#title' => t('Date To'),
			'#date_format' => \CVWO\Base\DATE_FORMAT_DATE,
			'#date_year_range' => '-100:0',
			'#date_label_position' => 'none',
			'#size' => 10,
			'#query_callback' => function(&$query, $value) {
				$value = new \DateTime($value);
				$value->setTime(23, 59, 59);
				$query = $query->condition('home_visit.date_visited',
					$value->format(\CVWO\Base\DATE_FORMAT_DATABASE), '<=');
				return $query;
			}
		);

		//set the additional filters
		$search->addCustomFilters($filter_form);
		aas_augment_search_for_address($query, $search);

		return $search;
	}

	function home_visit_list_submit($form, &$form_state)
	{
		$form_state['rebuild'] = true;
	}
}
