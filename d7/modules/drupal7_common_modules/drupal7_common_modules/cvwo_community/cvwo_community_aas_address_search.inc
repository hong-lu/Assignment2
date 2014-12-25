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
	use \CVWO\Base\Database as db;

	function aas_address_search_callback($form, &$form_state)
	{
		$form['#attached']['js'][] =
			drupal_get_path('module', MODULE_NAME) . '/cvwo_community_aas.js';

		$search = new \CVWOBaseSearch(__NAMESPACE__ . '\aas_address_search', $form);
		
		$query = db\select(\CVWO\Base\PERSON_TABLE, 'p')
			->fields('p', array(
				'person_id',
				'name'
			))
			->condition('p.is_deleted', 0, '=')
			->condition('p.name', '', '<>');
		$query->innerJoin(PERSON_TABLE, 'cp', 'p.person_id = cp.person_id');
		$query->innerJoin(\CVWO\Base\PERSONAL_PARTICULARS_PERSON_ADDRESS_TABLE, 'pa', 'p.person_id = pa.person_id');
		$query->innerJoin(CVWO_ADDRESS_TABLE, 'a', 'pa.address_id = a.address_id');
		$query->fields('a', array(
			'address_id',
			'blk_no',
			'street_name',
			'building_name',
			'floor_no',
			'unit_no',
			'postal_code'
		));
		$query->condition('a.postal_code', '', '<>');
		
		$header = array(
			array(
				'data'	 => t('Name'),
				'field'	 => 'name',
				'table'  => 'p'
			),
			array(
				'data'	 => t('Block'),
				'field'	 => 'blk_no',
				'table'	 => 'a'
			),
			array(
				'data'	 => t('Street Name'),
				'field'	 => 'street_name',
				'table'	 => 'a'
			),
			array(
				'data'	 => t('Floor'),
				'field'	 => 'floor_no',
				'table'	 => 'a'
			),
			array(
				'data'	 => t('Unit'),
				'field'	 => 'unit_no',
				'table'	 => 'a'
			),
			array(
				'data'	 => t('Postal Code'),
				'field'	 => 'postal_code',
				'table'	 => 'a',
				'sort'	 => 'asc'
			),
			array(
				'data'	 => t('Building Name'),
				'field'	 => 'building_name',
				'table'	 => 'a'
			)
		);
		
		$search->basicSearchWithoutFields($query, $header);
		
		$floor_no_formatter = array(
			'display'	 => function($row)
			{
				if (empty($row['floor_no']))
				{
					return '';
				}

				return '#' . $row['floor_no'];
			},
			'sortable'	 => true
		);
		$search->defineCustomColumns(array(
			//Hidden column to get person ID from the database
			'floor_no' => $floor_no_formatter,
			'a_floor_no' => $floor_no_formatter
		));
		
		aas_augment_search_for_address($query, $search);
		$search->setAdvancedSearchQuery($query);
		$search->defineAdvancedSearch($header, array(), 'person_id');
		
		$search->setOptionCallback(function($row) {
			$person = Person::from_id($row['person_id']);
			$data = array(
				'person_id'	 => $person->get_id(),
				'nric_fin'	 => $person->get_nric_fin(),
				'name'		 => $person->get_name(),
				'address'	 => $person->get_addresses(),
				'contact'	 => $person->get_contacts()
			);
			
			return l(t('Select User'), sprintf(
					'javascript: CVWO.Community.AAS.selectPerson(\'%s\')',
					\CVWO\Base\js_escape(drupal_json_encode($data))),
				array('external' => true));
		});

		return array_merge_recursive($form,
			$search->getForm($form_state));
	}
	
	/**
	 * Augments the given query and search object for AAS address lookup.
	 * 
	 * Condition: CVWO_ADDRESS_TABLE must be aliased to 'a'.
	 * @param SelectQuery $query
	 * @param \CVWOBaseSearch $search
	 */
	function aas_augment_search_for_address(&$query, &$search)
	{
		$search->addCustomFilters(array(
			'blk_no' => array(
				'#type'			 => 'textfield',
				'#size'			 => 4,
				'#maxlength'	 => 16,
				'#title'		 => t('Block'),
				'#query_callback'	 => function($query, $value)
				{
					$query->condition('a.blk_no', $value);
					return $query;
				}
			),

			'street_name' => array(
				'#type'			 => 'textfield',
				'#size'			 => 30,
				'#maxlength'	 => 255,
				'#title'		 => t('Street'),
				'#query_callback'	 => function($query, $value)
				{
					$query->condition('a.street_name', sprintf('%%%s%%', $value), 'LIKE');
					return $query;
				}
			),

			'floor_no' => array(
				'#type'			 => 'textfield',
				'#size'			 => 3,
				'#maxlength'	 => 5,
				'#title'		 => t('Floor'),
				'#field_prefix'	 => '#',
				'#query_callback'	 => function($query, $value)
				{
					$query->condition('a.floor_no', $value);
					return $query;
				}
			),

			'unit_no' => array(
				'#type'			 => 'textfield',
				'#size'			 => 5,
				'#maxlength'	 => 16,
				'#title'		 => t('Unit'),
				'#field_prefix'	 => '-',
				'#query_callback'	 => function($query, $value)
				{
					$query->condition('a.unit_no', $value);
					return $query;
				}
			),

			'postal_code' => array(
				'#type'			 => 'textfield',
				'#size'			 => 6,
				'#maxlength'	 => 6,
				'#title'		 => t('Postal Code'),
				'#field_prefix'	 => 'S(',
				'#field_suffix'	 => ')',
				'#query_callback'	 => function($query, $value)
				{
					$query->condition('a.postal_code', $value);
					return $query;
				}
			),

			'building_name' => array(
				'#type'			 => 'textfield',
				'#size'			 => 20,
				'#maxlength'	 => 255,
				'#title'		 => t('Building Name'),
				'#query_callback'	 => function($query, $value)
				{
					$query->condition('a.building_name', sprintf('%%%s%%', $value), 'LIKE');
					return $query;
				}
			)
		));
	}
}
