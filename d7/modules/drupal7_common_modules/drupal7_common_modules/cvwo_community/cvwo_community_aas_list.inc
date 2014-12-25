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
	require_once drupal_get_path('module', MODULE_NAME) . '/cvwo_community_aas_address_search.inc';

	use \CVWO\Base\Database as db;

	/**
	 * Builds the template for all AAS Advanced Search forms.
	 * 
	 * @return \CVWOBaseSearch
	 */
	function aas_list($form, &$form_state)
	{
		$form = array();

		//create search object
		$search = new \CVWOBaseSearch('AAS', $form);

		/*
		 * Basic Search Configuration
		 */
		//define header for basic search result, ignore options
		$header_basic = array(
			array('data' => t('Date/Time of Activation'), 'field' => 'activation_timestamp', 'table' => 'aas', 'sort' => 'desc'),
			array('data' => t('Name'), 'field' => 'name', 'table' => 'p'),
			array('data' => t('Address'), 'field' => 'address_id'),
			array('data' => t('Emergency'), 'field' => 'emergency'),
			array('data' => t('After Office Hours'), 'field' => 'after_office_hours'),
		);

		//define basic search form element
		$form_element_basic = array(
			'#type'			 => 'fieldset',
			'#title'		 => t('Basic Search'),
			'#collapsible'	 => true,
			'search_field'	 => array(
				'#type' => 'textfield',
				'#title' => t('Search Person or NRIC/FIN')
			),
		);

		//define basic search query
		$query= db\select(AAS_REPORTS_TABLE, 'aas');
		$query->fields('aas');
		$query->condition('aas.is_deleted', 0, '=');
		$query->join(\CVWO\Base\PERSON_TABLE, 'p', 'aas.person_id=p.person_id');
		$query->fields('p', array('name'));
		$query->leftJoin(CVWO_ADDRESS_TABLE, 'a', 'aas.address_id=a.address_id');

		$query_where = '(p.name LIKE :str OR p.nric_fin LIKE :str2)';

		//define custom columns, for formating result
		$custom_columns = array(
			'name' => '',
			'emergency' => function($values) {
				return empty($values['emergency']) ? t('No') : t('Yes');
			},
			'after_office_hours' => function($values) {
				return empty($values['after_office_hours']) ? t('No') : t('Yes');
			},
			'device' => array(
				'display' => function($values) {
					return cvwocodem2_getanstext($values['device']);
				},
				'sortable' => false
			),
			'address_id' => '\CVWO\Community\community_format_address',
			'activation_timestamp' => function($value) {
				$date = new \DateTime($value['activation_timestamp']);
				return $date->format(\CVWO\Base\DATE_FORMAT_DATETIME);
			}
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
			'aas' => array(
				array('device', MODULE_NAME, AAS_ITEM_QUESTION),
			)
		);
		//register codem filter
		$search->registerCodemFilter($filters);


		//define display fields, format: array('db_table' => array('db_field' => 'form_element_title')
		//cannot contain the default header
		$display_fields = array(
			'aas' => array(
				'details' => t('Details'),
				'follow_up' => t('Follow Up'),
				'device' => AAS_ITEM_QUESTION,
			),
		);
		//register display fields
		$search->registerDisplayFields($display_fields);

		//define default header for advanced search
		$header_advanced = array(
			array('data' => t('Date/Time of Activation'), 'field' => 'activation_timestamp', 'table' => 'aas', 'sort' => 'desc'),
			array('data' => t('Name'), 'field' => 'name', 'table' => 'p'),
			array('data' => t('Address'), 'field' => 'address_id'),
			array('data' => t('Emergency'), 'field' => 'emergency', 'table' => 'aas'),
			array('data' => t('After Office Hours'), 'field' => 'after_office_hours', 'table' => 'aas'),
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
			'#title' => t('Date From'),
			'#date_format' => \CVWO\Base\DATE_FORMAT_DATE,
			'#date_year_range' => '-100:0',
			'#date_label_position' => 'none',
			'#query_callback' => function($query, $value) {
				$value = new \DateTime($value);
				$query = $query->condition('aas.activation_timestamp',
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
			'#query_callback' => function($query, $value) {
				$value = new \DateTime($value);
				$value->setTime(23, 59, 59);
				$query = $query->condition('aas.activation_timestamp',
					$value->format(\CVWO\Base\DATE_FORMAT_DATABASE), '<=');
				return $query;
			}
		);

		//set the additional filters
		$search->addCustomFilters($filter_form);
		aas_augment_search_for_address($query, $search);

		/*
		 * Common Configuration
		 */
		return $search;
	}

	function aas_list_submit($form, &$form_state)
	{
		$form_state['rebuild'] = true;
	}
}
